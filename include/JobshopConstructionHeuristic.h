#pragma once
#include <string>
#include <cfloat>
#include <random>
#include <cassert>
#include <set>
#include <algorithm>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <Eigen/Dense>
#include "ConstructionHeuristicConcept.h"
#include "JobshopData.h"
#include "FFN.h"
#include "Err.h"
#include "utils.h"
#include "Generator.h"


namespace jobshop {
    using namespace std;
    using namespace Eigen;

    /**
     * @brief Core Constructive Heuristic for the Flexible Job Shop Problem.
     * * This class iteratively builds a schedule (Gantt chart) by selecting the best
     * available job-machine pairing at each step. The selection is guided by an
     * Assessment Function (AFType, usually a Neural Network) which evaluates the
     * current state of the factory based on extracted features.
     * * @tparam AFType The Assessment Function type (e.g., nnutils::FFN).
     * @tparam GenConfType The configuration type for data generation.
     */
    template<typename AFType, typename GenConfType>
    class JobshopConstructionHeuristic {
        friend class boost::serialization::access;

    public:
        /**
         * @brief Configuration and Feature Selection for the Heuristic.
         * * Controls which pieces of information (features) are extracted from the
         * current factory state and fed into the Assessment Function.
         */
        struct ConfigType {
            friend class boost::serialization::access;

            using AFT = AFType;
            using GenConfT = GenConfType;

            string desc; ///< Human-readable description of the heuristic.

            bool noAutoScaleEval = false; ///< If true, disables auto-scaling during evaluation (not boost::serialized).
            bool autoScale; ///< Enables automatic scaling of machine-related features.
            bool autoScaleNumOperationsInfo; ///< Enables scaling for the operations-left vector.
            bool nextOperationInfo; ///< If true, includes one-hot encoding of the next operation in the job.
            bool numOperationsInfo; ///< If true, includes the remaining count of all operation types.
            bool numAllOperationsInfo; ///< If true, includes the total count of all remaining operations.

            int numM; ///< Total number of machines in the problem space.
            int numO; ///< Total number of operation types in the problem space.

            typename AFType::ConfigType AConf; ///< Topology configuration for the Neural Network.
            GenConfType GConf; ///< Data generation configuration.

            template<class Archive>
            void serialize(Archive &ar, const unsigned int version) {
                ar & desc;
                ar & autoScale;
                ar & autoScaleNumOperationsInfo;
                ar & nextOperationInfo;
                ar & numOperationsInfo;
                ar & numAllOperationsInfo;
                ar & numM;
                ar & numO;
                ar & AConf;
                ar & GConf;
            }
        };

        // ConstructionHeuristicConcept interface
        typedef JobshopData DataType;

        static bool maximize() {
            return false; // Minimizing makespan
        }

        int getParamsSize() const {
            return AF.getParamsSize() + (Conf.autoScale && !Conf.noAutoScaleEval ? Scale.size() : 0) + (
                       Conf.autoScaleNumOperationsInfo && !Conf.noAutoScaleEval ? ScaleNumOperationsInfo.size() : 0);
        }

        void getParams(vector<double> &Params) const {
            AF.getParams(Params);
            if (Conf.autoScale && !Conf.noAutoScaleEval) {
                Params.insert(Params.end(), Scale.begin(), Scale.end());
            }
            if (Conf.autoScaleNumOperationsInfo && !Conf.noAutoScaleEval) {
                Params.insert(Params.end(), ScaleNumOperationsInfo.begin(), ScaleNumOperationsInfo.end());
            }
        }

        JobshopConstructionHeuristic &setParams(const double *params, int n) {
            int s = AF.getParamsSize();
            AF.setParams(params, s);
            params += s;
            if (Conf.autoScale && !Conf.noAutoScaleEval) {
                Scale.assign(params, params + Scale.size());
                params += Scale.size();
            }
            if (Conf.autoScaleNumOperationsInfo && !Conf.noAutoScaleEval) {
                ScaleNumOperationsInfo.assign(params, params + ScaleNumOperationsInfo.size());
                params += ScaleNumOperationsInfo.size();
            }
            return *this;
        }

        DataType::SolutionType run(const DataType &Data) {
            return construct(Data);
        }

        // end of interface
        ConfigType Conf;

    protected:
        /**
         * @brief Trainable scaling parameters for machine workload normalization.
         * * These values are appended to the neural network's weight vector and optimized
         * directly by the CMA-ES algorithm. They allow the system to automatically learn
         * the optimal normalization bounds (Auto-Scaling) for machine time features
         * before feeding them into the activation function (scaleTanh).
         */
        vector<float> Scale;

        /**
         * @brief Trainable scaling parameters for operation count normalization.
         * * Similar to 'Scale', these variables are optimized by CMA-ES to learn how
         * to best compress the "number of operations left" signal into the [-1, 1] range.
         */
        vector<float> ScaleNumOperationsInfo;

        AFType AF; ///< The Assessment Function (Neural Network)

        int numAllTasksLeft; ///< Total operations left to schedule.
        vector<vector<int> > JobsOperationsNumLeft; ///< [job][operation] -> number of operations left in the job.
        vector<vector<int> > JobsOperationsLeft; ///< [job] -> sequence of operations in a job (in reverse order).
        vector<int> NumOperationsLeft; ///< [operation] -> total left of this type across all jobs.
        vector<int> JobMinTime; ///< [job] -> earliest time the next operation can start.
        vector<int> MachineEndTime; ///< [machine] -> current end time of the machine's schedule.

        vector<vector<int> > OperationsMachines; ///< [operation] -> valid machines for this operation.
        vector<vector<int> > MachinesOperations; ///< [machine] -> valid operations for this machine.

        Matrix<float, Dynamic, 1> View1D; ///< [machine] -> diff between x_max and MachineEndTime.

        int x_max = 0; ///< Current global makespan (max of MachineEndTime).
        float avgOpTime = 1.0f; ///< Global average operation duration.

        Matrix<float, Dynamic, 1> NNInput; ///< Buffer for the Neural Network feature vector.

    public:
        template<class Archive>
        void serialize(Archive &ar, const unsigned int version) {
            ar & Conf;
            ar & Scale;
            ar & ScaleNumOperationsInfo;
            ar & AF;
        }

        explicit JobshopConstructionHeuristic() {
        }

        explicit JobshopConstructionHeuristic(const ConfigType &_Config)
            : Conf(_Config), AF(_Config.AConf), Scale(_Config.numM + 1, 0.0f),
              ScaleNumOperationsInfo(_Config.numO, 0.0f) {
            int dim = getParamsSize();
            vector<double> Params(dim, 0.0);
            setParams(Params.data(), dim);
        }

    protected:
        // =====================
        // CORE SCHEDULING LOGIC
        // =====================

        /**
         * @brief Main scheduling loop (Greedy Construction).
         * * Repeatedly scans all available operations and valid machines, calculates the state
         * feature vector (NNInput), and asks the Neural Network to evaluate the move.
         * The move with the highest activation value is chosen and applied to the schedule.
         * @param IOD The problem instance data.
         * @return The completed schedule and its makespan.
         */
        DataType::SolutionType construct(const DataType &IOD) {
            DataType::SolutionType Solution;

            init(IOD);

            Solution.Decs.clear();
            Solution.Decs.reserve(numAllTasksLeft);

            // Continue until all operations for all jobs are scheduled
            while (numAllTasksLeft > 0) {
                float best_v = -FLT_MAX;
                int best_j = -1;
                int best_m = -1;
                int best_o = -1;

                // Evaluate all valid (Job, Machine) pairs for the next operation
                for (int j = 0; j < IOD.numJ; j++) {
                    if (JobsOperationsLeft[j].empty()) continue;

                    int o = JobsOperationsLeft[j].back(); // Get next operation (stored in reverse)

                    for (int m: OperationsMachines[o]) {
                        // Extract factory state into a feature vector
                        prepareNNInput(IOD, j, m, o);
                        // Use Neural Network for evaluation score
                        float v = AF(NNInput);

                        // Keep track of the best move
                        if (v > best_v) {
                            best_v = v;
                            best_j = j;
                            best_m = m;
                            best_o = o;
                        }
                    }
                }

                if (best_j < 0)
                    INTERNAL("could not find assignment");

                updateData(IOD, best_j, best_m, best_o);

                // Apply the best move to the factory state
                JobshopData::Dec D = {
                    best_m,
                    MachineEndTime[best_m] - IOD.OMtime[best_o][best_m], // start time
                    MachineEndTime[best_m], // end time
                    best_j,
                    (int) (IOD.Jobs[best_j].Ops.size() - (JobsOperationsLeft[best_j].size() + 1))
                };
                Solution.Decs.push_back(D);
            }

            Solution.setObj(x_max);

            return Solution;
        }

        /**
         * @brief Initializes the factory simulation state before building a schedule.
         * @param IOD Problem instance data.
         */
        void init(const DataType &IOD) {
            Scale.assign(IOD.numM, 0.0f);
            ScaleNumOperationsInfo.assign(IOD.numO, 0.0f);

            x_max = 0;
            numAllTasksLeft = 0;
            NumOperationsLeft.assign(IOD.numO, 0);
            JobsOperationsNumLeft.assign(IOD.numJ, vector<int>(IOD.numO, 0));

            for (auto &J: IOD.Jobs) {
                for (auto o: J.Ops) {
                    JobsOperationsNumLeft[J.idx][o] += 1;
                    NumOperationsLeft[o] += 1;
                    numAllTasksLeft += 1;
                }
            }
            JobsOperationsLeft.resize(IOD.numJ);
            for (auto &J: IOD.Jobs) {
                JobsOperationsLeft[J.idx] = J.Ops;
                reverse(JobsOperationsLeft[J.idx].begin(), JobsOperationsLeft[J.idx].end());
            }

            JobMinTime.assign(IOD.numJ, 0);
            MachineEndTime.assign(IOD.numM, 0);

            // Fill OperationsMachines and MachinesOperations mappings
            OperationsMachines.assign(IOD.numO, vector<int>());
            MachinesOperations.assign(IOD.numM, vector<int>());

            vector<set<int> > OperationsMachinesSet(IOD.numO);
            vector<set<int> > MachinesOperationsSet(IOD.numM);

            assert(IOD.numO == IOD.OMtime.size());

            avgOpTime = 0.0f;
            int opTimeNum = 0;

            for (int o = 0; o < IOD.numO; o++) {
                assert(IOD.numM == IOD.OMtime[o].size());

                for (int m = 0; m < IOD.numM; m++) {
                    int t = IOD.OMtime[o][m];
                    if (t > 0) {
                        avgOpTime += t;
                        opTimeNum += 1;
                        OperationsMachinesSet[o].insert(m);
                        MachinesOperationsSet[m].insert(o);
                    }
                }
            }

            avgOpTime /= opTimeNum;

            for (int o = 0; o < OperationsMachinesSet.size(); o++) {
                for (auto m: OperationsMachinesSet[o]) {
                    OperationsMachines[o].push_back(m);
                }
            }

            for (int m = 0; m < MachinesOperationsSet.size(); m++) {
                for (auto o: MachinesOperationsSet[m]) {
                    MachinesOperations[m].push_back(o);
                }
            }

            View1D.resize(IOD.numM, 1);

            View1D.setConstant(0.0f);

            NNInput.resize(Conf.AConf.numInputs, 1);
        }

        /**
         * @brief Updates the factory state after permanently assigning an operation to a machine.
         * Advances the machine's local clock and the job's local clock.
         */
        void updateData(const DataType &Data, int j, int m, int o) {
            numAllTasksLeft--;
            NumOperationsLeft[o]--;
            JobsOperationsNumLeft[j][o]--;
            JobsOperationsLeft[j].pop_back();

            // The new end time is the max of when the machine is free and when the job is free, plus duration
            JobMinTime[j] = MachineEndTime[m] = max(MachineEndTime[m], JobMinTime[j]) + Data.OMtime[o][m];

            // Update global makespan
            int delta_x = max(0, MachineEndTime[m] - x_max);
            if (delta_x > 0) {
                View1D.array() += (float) delta_x;
                x_max += delta_x;
            }

            View1D[m] = x_max - MachineEndTime[m];
        }

        /**
         * @brief Constructs the Feature Vector (NNInput) for the Neural Network.
         * * This method translates the current simulation state and the proposed move
         * (Job j, Machine m, Operation o) into a normalized float vector.
         */
        void prepareNNInput(const DataType &Data, int j, int m, int o) {
            // Calculate idle time (wasted time) if machine has to wait for the job to arrive
            int wasted = max(0, JobMinTime[j] - MachineEndTime[m]);
            int od = Data.OMtime[o][m]; //< operation duration on machine m
            int met = max(MachineEndTime[m], JobMinTime[j]) + od; //< machine end time, after inserting operation

            int idx = 0;
            float sc = avgOpTime * 2;

            // Segment 1: Machine workloads (Diff between global makespan and machine end time)
            for (int i = 0; i < Data.numM; i++) {
                NNInput.middleRows(idx, Data.numM)(idx + i) = scaleTanh(0.5f / (sc * (1.0 + Scale[i])) * View1D[i]);
            }
            // Temporarily simulate the move for the evaluated machine 'm'
            NNInput.middleRows(idx, Data.numM)(m) = scaleTanh(0.5f / (sc * (1.0 + Scale[m])) * (met - x_max));
            idx += Data.numM;

            // Segment 2: One-hot encoding of the evaluated Machine
            NNInput.middleRows(idx, Data.numM).array() = 0.0f;
            NNInput.middleRows(idx, Data.numM)(m) = 1.0f;
            idx += Data.numM;

            // Segment 3: One-hot encoding of the evaluated Operation
            NNInput.middleRows(idx, Data.numO).array() = 0.0f;
            NNInput.middleRows(idx, Data.numO)(o) = 1.0f;
            idx += Data.numO;

            // Segment 4 [Optional]: One-hot encoding of the NEXT operation in the job sequence
            if (Conf.nextOperationInfo) {
                NNInput.middleRows(idx, Data.numO).array() = 0.0f;
                if (JobsOperationsLeft[j].size() > 1) {
                    int noi = JobsOperationsLeft[j].size() - 2;
                    int no = JobsOperationsLeft[j][noi];
                    NNInput.middleRows(idx, Data.numO)(no) = 1.0f;
                }
                idx += Data.numO;
            }

            // Segment 5 [Optional]: Number of remaining operations of each type globally
            if (Conf.numOperationsInfo) {
                auto V = NNInput.middleRows(idx, Data.numO);
                for (int i = 0; i < Data.numO; i++) {
                    V(i) = scaleTanh(0.5 / (2.0 * (1.0 + ScaleNumOperationsInfo[i])) * NumOperationsLeft[i]);
                }
                idx += Data.numO;
            }

            // Segment 6 [Optional]: Total number of remaining operations overall
            if (Conf.numAllOperationsInfo) {
                NNInput(idx) = scaleTanh(0.5f / (2 * Conf.numM) * numAllTasksLeft);
                idx += 1;
            }

            // Segment 7: Remaining operations of each type specifically in the CURRENT Job
            for (int i = 0; i < Data.numO; i++) {
                NNInput(idx + i) = (JobsOperationsNumLeft[j][i] == 0
                                        ? 0.0f
                                        : scaleTanh(0.5f / 2 * JobsOperationsNumLeft[j][i]));
            }
            idx += Data.numO;

            // Segment 8: Wasted (idle) time introduced by this assignment
            NNInput(idx) = scaleTanh(0.5f / (avgOpTime * (1.0 + Scale.back())) * wasted);
            idx += 1;

            if (NNInput.rows() != idx)
                INTERNAL(
                "NNInput.rows() != idx (" + to_string(NNInput.rows()) + " != " + to_string(idx) + "), numO=" +
                to_string(Data.numO) + " numO=" + to_string(Conf.numO) + " numM=" + to_string(Data.numM) + " numM="
                + to_string(Conf.numM));
        }
    };

    // ==================
    // JSON SERIALIZATION
    // ==================

    template<typename T>
    auto to_json(nlohmann::json &j, const T &c)
        -> std::enable_if_t<std::is_same_v<T, typename JobshopConstructionHeuristic<typename T::AFT, typename
            T::GenConfT>::ConfigType> > {
        j = nlohmann::json{};
        j.emplace("desc", c.desc);
        j.emplace("noAutoScaleEval", c.noAutoScaleEval);
        j.emplace("autoScale", c.autoScale);
        j.emplace("autoScaleNumTaskInfo", c.autoScaleNumOperationsInfo);
        j.emplace("nextTaskInfo", c.nextOperationInfo);
        j.emplace("numTasksInfo", c.numOperationsInfo);
        j.emplace("numAllTasksInfo", c.numAllOperationsInfo);
        j.emplace("numM", c.numM);
        j.emplace("numO", c.numO);
        j.emplace("AConf", c.AConf);
        j.emplace("GConf", c.GConf);
    }

    template<typename T>
    auto from_json(nlohmann::json &j, T &c)
        -> std::enable_if_t<std::is_same_v<T, typename JobshopConstructionHeuristic<typename T::AFT, typename
            T::GenConfT>::ConfigType> > {
        j.at("desc").get_to(c.desc);
        j.at("noAutoScaleEval").get_to(c.noAutoScaleEval);
        j.at("autoScale").get_to(c.autoScale);
        j.at("autoScaleNumTaskInfo").get_to(c.autoScaleNumOperationsInfo);
        j.at("nextTaskInfo").get_to(c.nextOperationInfo);
        j.at("numTasksInfo").get_to(c.numOperationsInfo);
        j.at("numAllTasksInfo").get_to(c.numAllOperationsInfo);
        j.at("numM").get_to(c.numM);
        j.at("numO").get_to(c.numO);
        j.at("AConf").get_to(c.AConf);
        j.at("GConf").get_to(c.GConf);
    }


    //  static_assert(chof::ConstructionHeuristicConcept<JobshopConstructionHeuristic<nnutils::FFN, GenConfigType>>);
};
