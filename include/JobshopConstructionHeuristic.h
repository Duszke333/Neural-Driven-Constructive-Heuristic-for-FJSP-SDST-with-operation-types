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
    using namespace ReadConfig;
    using namespace Eigen;

    template<typename AFType, typename GenConfType>
    class JobshopConstructionHeuristic {
        friend class boost::serialization::access;

    public:
        struct ConfigType {
            friend class boost::serialization::access;

            using AFT = AFType;
            using GenConfT = GenConfType;

            string desc;
            bool noAutoScaleEval = false; //< during evaluation, do not do autoScale nor autoScaleNumOperationsInfo
            //< this parameter is not boost::serialized
            bool autoScale;
            bool autoScaleNumOperationsInfo; //< if to auto scale vector numTaskInfo
            bool nextOperationInfo; //< if to include the information on next operation
            bool numOperationsInfo; //< if to include information on the number left of all task types
            bool numAllOperationsInfo; //< if to include information on the number of all tasks left
            int numM; //< number of machines
            int numO;
            typename AFType::ConfigType AConf;
            GenConfType GConf;

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

    public:
        // ConstructionHeuristicConcept interface
        typedef JobshopData DataType;

        static bool maximize() {
            return false;
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
    public:
        // fields
        ConfigType Conf;

    protected:
        vector<float> Scale;
        vector<float> ScaleNumOperationsInfo;

        AFType AF;

        int numAllTasksLeft;
        vector<vector<int> > JobsOperationsNumLeft; //< [job][operation] -> number left
        vector<vector<int> > JobsOperationsLeft; //< [job] -> list of operations, in reverse order
        vector<int> NumOperationsLeft; //< [task] -> number of left operations of this type
        vector<int> JobMinTime; //< [job] -> minimum time, when the next operation can start
        vector<int> MachineEndTime; //< [machine] -> schedule end time
        vector<vector<int> > OperationsMachines; //< [operation] -> list of machines permitted
        vector<vector<int> > MachinesOperations; //< [machine] -> list of operations permitted

        Matrix<float, Dynamic, 1> View1D; //< [machine] -> diff between x_max and MachineEndTime

        int x_max = 0; //< when  current schedule ends, max of MachineEndTimes
        float avgOpTime = 1.0f; //< average operation duration

        Matrix<float, Dynamic, 1> NNInput;

    public:
        template<class Archive>
        void serialize(Archive &ar, const unsigned int version) {
            ar & Conf;
            ar & Scale;
            ar & ScaleNumOperationsInfo;
            ar & AF;
        }

    public:
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
        DataType::SolutionType construct(const DataType &IOD) {
            DataType::SolutionType Solution;

            init(IOD);

            Solution.Decs.clear();
            Solution.Decs.reserve(numAllTasksLeft);

            while (numAllTasksLeft > 0) {
                float best_v = -FLT_MAX;
                int best_j = -1;
                int best_m = -1;
                int best_o = -1;
                for (int j = 0; j < IOD.numJ; j++) {
                    if (JobsOperationsLeft[j].empty()) continue;

                    int o = JobsOperationsLeft[j].back(); //< as it is in reverse order

                    for (int m: OperationsMachines[o]) {
                        prepareNNInput(IOD, j, m, o);

                        float v = AF(NNInput);


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

                JobshopData::Dec D = {
                    best_m,
                    MachineEndTime[best_m] - IOD.OMtime[best_o][best_m],
                    MachineEndTime[best_m],
                    best_j,
                    (int) (IOD.Jobs[best_j].Ops.size() - (JobsOperationsLeft[best_j].size() + 1))
                };
                Solution.Decs.push_back(D);
            }

            Solution.setObj(x_max);

            return Solution;
        }


        void init(const DataType &IOD) {
            Scale.assign(IOD.numM, 0.0f);

            //  Scale = {0.7, 0.3, 0.5, -0.4, 0.18, 0.17, 0.6, 0.18, 0.04, -0.9, 0.24}; //!!!!!!!!!!!!!111

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

            // fill OperationsMachines and MachinesOperations
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


        void updateData(const DataType &Data, int j, int m, int o) {
            numAllTasksLeft--;
            NumOperationsLeft[o]--;
            JobsOperationsNumLeft[j][o]--;
            JobsOperationsLeft[j].pop_back();

            JobMinTime[j] = MachineEndTime[m] = max(MachineEndTime[m], JobMinTime[j]) + Data.OMtime[o][m];

            int delta_x = max(0, MachineEndTime[m] - x_max);

            if (delta_x > 0) {
                View1D.array() += (float) delta_x;
                x_max += delta_x;
            }

            View1D[m] = x_max - MachineEndTime[m];
        }


        void prepareNNInput(const DataType &Data, int j, int m, int o) {
            int wasted = max(0, JobMinTime[j] - MachineEndTime[m]);
            int od = Data.OMtime[o][m]; //< operation duration on machine m
            int met = max(MachineEndTime[m], JobMinTime[j]) + od; //< machine end time, after inserting operation

            // View1D
            int idx = 0;
            float sc = avgOpTime * 2;
            for (int i = 0; i < Data.numM; i++) {
                NNInput.middleRows(idx, Data.numM)(idx + i) = scaleTanh(0.5f / (sc * (1.0 + Scale[i])) * View1D[i]);
            }
            NNInput.middleRows(idx, Data.numM)(m) = scaleTanh(0.5f / (sc * (1.0 + Scale[m])) * (met - x_max));

            idx += Data.numM;

            // current machine
            NNInput.middleRows(idx, Data.numM).array() = 0.0f;
            NNInput.middleRows(idx, Data.numM)(m) = 1.0f;
            idx += Data.numM;

            // current operation
            NNInput.middleRows(idx, Data.numO).array() = 0.0f;
            NNInput.middleRows(idx, Data.numO)(o) = 1.0f;
            idx += Data.numO;

            if (Conf.nextOperationInfo) {
                // next operation in current job
                NNInput.middleRows(idx, Data.numO).array() = 0.0f;
                if (JobsOperationsLeft[j].size() > 1) {
                    int noi = JobsOperationsLeft[j].size() - 2;
                    int no = JobsOperationsLeft[j][noi];
                    NNInput.middleRows(idx, Data.numO)(no) = 1.0f;
                }
                idx += Data.numO;
            }

            if (Conf.numOperationsInfo) {
                auto V = NNInput.middleRows(idx, Data.numO);

                for (int i = 0; i < Data.numO; i++) {
                    V(i) = scaleTanh(0.5 / (2.0 * (1.0 + ScaleNumOperationsInfo[i])) * NumOperationsLeft[i]);
                }

                idx += Data.numO;
            }

            if (Conf.numAllOperationsInfo) {
                NNInput(idx) = scaleTanh(0.5f / (2 * Conf.numM) * numAllTasksLeft);
                idx += 1;
            }

            // operations left in current job
            for (int i = 0; i < Data.numO; i++) {
                NNInput(idx + i) = (JobsOperationsNumLeft[j][i] == 0
                                        ? 0.0f
                                        : scaleTanh(0.5f / 2 * JobsOperationsNumLeft[j][i]));
            }
            idx += Data.numO;

            NNInput(idx) = scaleTanh(0.5f / (avgOpTime * (1.0 + Scale.back())) * wasted);
            idx += 1;

            if (NNInput.rows() != idx)
                INTERNAL(
                    "NNInput.rows() != idx (" + to_string(NNInput.rows()) + " != " + to_string(idx) + "), numO=" +
                    to_string(Data.numO) + " numO=" + to_string(Conf.numO) + " numM=" + to_string(Data.numM) + " numM="
                    + to_string(Conf.numM));
        }
    };

    // template<typename T>
    // inline void to_json(nlohmann::json& j, const T& c) {
    //     to_json<typename T::AF, typename T::GenConf>(j, c);
    // }

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
        //const nlohmann::json &j, typename JobshopConstructionHeuristic<AFType,GenConfType>::ConfigType &c) {
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




