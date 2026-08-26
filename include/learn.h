#pragma once
#include <random>
#include <iomanip>
#include <set>
#include "DataConcept.h"
#include "ConstructionHeuristicConcept.h"
#include "DataSetEvaluator.h"
#include "ParallelEvaluator.h"
#include "ParallelDataSetEvaluator.h"
#include "Err.h"
#include "float.h"
#include "libcmaes/cmaes.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include "utils.h"


namespace chof {
    using namespace std;
    using namespace libcmaes;

    // ========================
    // CONFIGURATION STRUCTURES
    // ========================

    /**
     * @brief Core configuration for the CMA-ES optimizer.
     * Contains essential parameters steering the evolutionary process.
     */
    struct OptConfig {
        int seed = 1; ///< Random seed for reproducible runs.
        int population = 192; ///< Number of candidates generated in each generation (lambda).
        vector<int> NumEvals = {1}; ///< Maximum number of objective function evaluations.
        vector<double> Sigmas = {0.1}; ///< Initial step size (standard deviation) for CMA-ES.
        bool mt_feval = false; ///< Enable multi-threaded objective function evaluation.
        bool quiet = true; ///< Suppress libcmaes standard console output.
        bool zero = false; ///< Initialize parameters with zeros.
        bool storeProgressInfo = false; ///< Write optimization progress to a file.
        string progressFile = ""; ///< Path to the progress CSV file.
        int timeLimit = 0; ///< Execution time limit in seconds (0 = no time limit).

        OptConfig() {
        }

        OptConfig(int numEvals, double sigma) : NumEvals({numEvals}), Sigmas({sigma}), mt_feval(true) {
        }
    };

    inline void to_json(nlohmann::json &j, const OptConfig &config) {
        j = nlohmann::json{};
        j.emplace("seed", config.seed);
        j.emplace("population", config.population);
        j.emplace("NumEvals", config.NumEvals);
        j.emplace("Sigmas", config.Sigmas);
        j.emplace("mt_feval", config.mt_feval);
        j.emplace("quiet", config.quiet);
        j.emplace("zero", config.zero);
        j.emplace("storeProgressInfo", config.storeProgressInfo);
        j.emplace("progressFile", config.progressFile);
        j.emplace("timeLimit", config.timeLimit);
    }

    inline void from_json(const nlohmann::json &j, OptConfig &p) {
        j.at("seed").get_to(p.seed);
        j.at("population").get_to(p.population);
        j.at("NumEvals").get_to(p.NumEvals);
        j.at("Sigmas").get_to(p.Sigmas);
        j.at("mt_feval").get_to(p.mt_feval);
        j.at("quiet").get_to(p.quiet);
        j.at("zero").get_to(p.zero);
        j.at("storeProgressInfo").get_to(p.storeProgressInfo);
        j.at("progressFile").get_to(p.progressFile);
        if (j.contains("timeLimit")) {
            j.at("timeLimit").get_to(p.timeLimit);
        }
    }


    /**
     * @brief Extended configuration for learning loops.
     * Adds parameters for managing training and validation datasets (mini-batching).
     */
    struct LearnConfig : public OptConfig {
        int trainingDataSize; ///< Number of instances in a single training batch.
        int itersToValidate; ///< Number of iterations (generations) before applying validation.
        int numValidationThreads; ///< Number of threads allocated for the validation pass.

        LearnConfig() {
        }
    };

    inline void to_json(nlohmann::json &j, const LearnConfig &c) {
        j = nlohmann::json((OptConfig &) c);
        j.emplace("trainingDataSize", c.trainingDataSize);
        j.emplace("itersToValidate", c.itersToValidate);
        j.emplace("numValidationThreads", c.numValidationThreads);
    }

    inline void from_json(const nlohmann::json &j, LearnConfig &c) {
        from_json(j, (OptConfig &) c);
        j.at("trainingDataSize").get_to(c.trainingDataSize);
        j.at("itersToValidate").get_to(c.itersToValidate);
        j.at("numValidationThreads").get_to(c.numValidationThreads);
    }

    // =================================================
    // SINGLE INSTANCE OPTIMIZATION (LOCAL OPTIMIZATION)
    // =================================================

    /**
     * @brief Optimizes a Construction Heuristic to best solve a SINGLE specific problem instance.
     * * This performs local optimization (overfitting) on a given problem to find the absolute
     * best weights for that specific layout.
     * @tparam CHType The Construction Heuristic type.
     * @param Config Optimization parameters.
     * @param CH The starting heuristic (with initial weights).
     * @param _Data The single problem instance to solve.
     * @param CHOut The resulting optimized heuristic.
     * @return The best generated schedule (solution).
     */
    template<ConstructionHeuristicConcept CHType>
    typename CHType::DataType::SolutionType opt(
        const OptConfig &Config,
        const CHType &CH,
        typename CHType::DataType &_Data,
        CHType &CHOut) {
        auto time_start = std::chrono::steady_clock::now();

        typename CHType::DataType Data = _Data;
        std::ofstream outfile2;
        if (Config.storeProgressInfo) {
            outfile2 = nnutils::openFileWithDirs<std::ofstream>(Config.progressFile, std::ios::app);
            outfile2 << "n-iter; fevals; avg_fval; best_fval; best_so_far; time" << endl;
        }

        vector<typename CHType::DataType *> TrainingDataP;
        TrainingDataP.push_back(&Data);

        DataSetEvaluator DSE(TrainingDataP, CH);
        ParallelEvaluator PE(DSE);

        std::vector<double> x0;
        CH.getParams(x0);

        std::function<double(const double *, const int &n)> F = PE;

        // Callback invoked by CMA-ES after each generation
        ProgressFunc<CMAParameters<>, CMASolutions> progress_func =
                [&](const CMAParameters<> &cmaparams, const CMASolutions &cmasols) {
            double ssum = 0.0;
            int nnum = 0;
            for (const Candidate &c: ((CMASolutions &) cmasols).candidates()) {
                ssum += c.get_fvalue();
                nnum += 1;
            }

            auto time_end = std::chrono::steady_clock::now();
            auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();
            double time_s = 0.001 * elapsed_ms;

            if (outfile2.is_open()) {
                outfile2 << setprecision(4) << cmasols.niter() << "; " << cmasols.fevals() << "; " << ssum / nnum <<
                        "; " << cmasols.best_candidate().get_fvalue() << "; " << cmasols.get_best_seen_candidate().
                        get_fvalue();
                outfile2 << setprecision(2) << "; " << std::fixed << time_s << endl << flush;
            }

            if (Config.timeLimit > 0 && time_s > Config.timeLimit) {
                return 1; // Triggers early stopping in libcmaes
            } else {
                return 0;
            }
        };

        vector<double> BestParams;
        double BestObj = (CH.maximize() ? -DBL_MAX : DBL_MAX);
        typename CHType::DataType::SolutionType BestSolution;

        for (int it = 0; it < Config.NumEvals.size(); it++) {
            CMAParameters<> cmaparams(x0, Config.Sigmas[it], Config.population, (it + 1) * (Config.seed + 1) * 100);

            // Using separable CMA-ES (sepaCMAES) which has linear time complexity (O(n))
            // instead of quadratic, essential for optimizing Neural Networks with many weights.
            cmaparams.set_algo(sepaCMAES);
            cmaparams.set_mt_feval(Config.mt_feval);
            cmaparams.set_max_fevals(Config.NumEvals[it]);
            cmaparams.set_quiet(Config.quiet);
            cmaparams.set_maximize(CH.maximize());
            cmaparams.set_elitism(1);
            cmaparams.set_stopping_criteria(EQUALFUNVALS, false);
            cmaparams.set_stopping_criteria(STAGNATION, false);
            cmaparams.set_stopping_criteria(TOLHISTFUN, false);

            CMASolutions cmasols = cmaes<>(F, cmaparams, progress_func);
            Candidate C = cmasols.get_best_seen_candidate();

            if (!Config.quiet) cout << "\n" << cmasols.status_msg() << "\n" << flush;

            auto Params = C.get_x();
            double obj = DSE(Params.data(), Params.size());

            if (CH.maximize() ? obj > BestObj : obj < BestObj) {
                BestParams = Params;
                BestObj = obj;
            }
            x0 = BestParams;
        }

        if (outfile2.is_open()) outfile2.close();

        CHOut = CH;
        CHOut.setParams(BestParams.data(), BestParams.size());

        return CHOut.run(Data);
    }


    /**
     * @brief Optimizes a heuristic using a target problem and dynamically generated variations.
     * * Similar to opt(), but uses a RandomDataGenerator (RDG) to augment the single instance
     * into a batch of similar instances to prevent overfitting.
     * @param Config Optimization parameters.
     * @param CH The starting heuristic (with initial weights).
     * @param _Data The single problem instance to solve.
     * @param RDG Random data generator used for augmenting instances.
     * @param CHOut The resulting optimized heuristic.
     * @return The best generated schedule (solution).
     */
    template<ConstructionHeuristicConcept CHType, typename RandomDataGenerator>
    typename CHType::DataType::SolutionType opt2(
        const LearnConfig &Config,
        const CHType &CH,
        typename CHType::DataType &_Data,
        RandomDataGenerator &RDG,
        CHType &CHOut) {
        auto time_start = std::chrono::steady_clock::now();

        std::ofstream outfile2;
        if (Config.storeProgressInfo) {
            outfile2 = nnutils::openFileWithDirs<std::ofstream>(Config.progressFile, std::ios::app);
            outfile2 << "n-iter; fevals; avg_fval; best_fval; best_so_far; time" << endl;
        }

        mt19937 gen(Config.seed);

        vector<typename CHType::DataType> DataVec(Config.trainingDataSize);
        DataVec[0] = _Data;
        for (int i = 1; i < Config.trainingDataSize; i++) {
            DataVec[i] = RDG(gen, _Data);
        }

        vector<typename CHType::DataType *> TrainingDataP;
        TrainingDataP.resize(Config.trainingDataSize);
        for (int i = 0; i < Config.trainingDataSize; i++) {
            TrainingDataP[i] = &(DataVec[i]);
        }

        DataSetEvaluator DSE(TrainingDataP, CH);
        ParallelEvaluator PE(DSE);

        std::vector<double> x0;
        CH.getParams(x0);

        std::function<double(const double *, const int &n)> F = PE;

        ProgressFunc<CMAParameters<>, CMASolutions> progress_func =
                [&](const CMAParameters<> &cmaparams, const CMASolutions &cmasols) {
            double ssum = 0.0;
            int nnum = 0;
            for (const Candidate &c: ((CMASolutions &) cmasols).candidates()) {
                ssum += c.get_fvalue();
                nnum += 1;
            }

            auto time_end = std::chrono::steady_clock::now();
            auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();
            double time_s = 0.001 * elapsed_ms;

            if (outfile2.is_open()) {
                outfile2 << setprecision(4) << cmasols.niter() << "; " << cmasols.fevals() << "; " << ssum / nnum <<
                        "; " << cmasols.best_candidate().get_fvalue() << "; " << cmasols.get_best_seen_candidate().
                        get_fvalue();
                outfile2 << setprecision(2) << "; " << std::fixed << time_s << endl << flush;
            }
            if (!Config.quiet) cout << "." << flush;

            if (Config.timeLimit > 0 && time_s > Config.timeLimit) {
                return 1;
            } else {
                // Resample dataset variations for next generation
                for (int i = 1; i < Config.trainingDataSize; i++) {
                    DataVec[i] = RDG(gen, _Data);
                }
                return 0;
            }
        };

        vector<double> BestParams;
        double BestObj = (CH.maximize() ? -DBL_MAX : DBL_MAX);
        typename CHType::DataType::SolutionType BestSolution;

        for (int it = 0; it < Config.NumEvals.size(); it++) {
            CMAParameters<> cmaparams(x0, Config.Sigmas[it], Config.population, (it + 1) * (Config.seed + 1) * 100);

            // -1 for automatically decided lambda, 0 is for random seeding of the internal generator.
            cmaparams.set_algo(sepaCMAES);
            bool mt = Config.mt_feval;
#ifndef NDEBUG
            mt = false;
#endif
            cmaparams.set_mt_feval(mt);
            cmaparams.set_max_fevals(Config.NumEvals[it]);
            cmaparams.set_quiet(Config.quiet);
            cmaparams.set_maximize(CH.maximize());
            cmaparams.set_elitism(1);
            cmaparams.set_stopping_criteria(EQUALFUNVALS, false);
            cmaparams.set_stopping_criteria(STAGNATION, false);
            cmaparams.set_stopping_criteria(TOLHISTFUN, false);

            CMASolutions cmasols = cmaes<>(F, cmaparams, progress_func);
            Candidate C = cmasols.get_best_seen_candidate();

            if (!Config.quiet) cout << "\n" << cmasols.status_msg() << "\n" << flush;

            auto Params = C.get_x();
            double obj = DSE(Params.data(), Params.size());

            if (CH.maximize() ? obj > BestObj : obj < BestObj) {
                BestParams = Params;
                BestObj = obj;
            }
            x0 = BestParams;
        }

        if (outfile2.is_open()) outfile2.close();

        CHOut = CH;
        CHOut.setParams(BestParams.data(), BestParams.size());

        return CHOut.run(_Data);
    }

    // ======================================
    // GLOBAL DATASET OPTIMIZATION (LEARNING)
    // ======================================

    /**
     * @brief Randomly samples a batch of instances from the total dataset.
     */
    template<DataConcept DataType>
    void selectRandomData(std::mt19937 &gen, vector<DataType> &TotalData, int numData,
                          vector<DataType *> &SelectedData) {
        SelectedData.resize(numData);

        for (auto &TD: SelectedData) {
            TD = &(TotalData[uniform_int_distribution<>(0, TotalData.size() - 1)(gen)]);
        }
    }

    /**
     * @brief The core Machine Learning pipeline. Trains a heuristic over a large dataset.
     * * Operates using a mini-batch approach. CMA-ES optimizes weights over a small batch
     * of instances (TrainingDataSize). Every 'itersToValidate' generations, the algorithm
     * evaluates its current best weights against a completely separate ValidationData set
     * to check for generalization and prevent overfitting. The best weights found on the
     * ValidationData are saved and returned.
     * @tparam CHType The Construction Heuristic type.
     * @param Config Learning configuration (batch size, validation frequency, etc.).
     * @param CH Initial heuristic model.
     * @param TotalData The complete pool of training instances.
     * @param ValidationData The held-out set used strictly for unbiased scoring.
     * @param CHOut The trained model (containing the best validation weights).
     * @return The best objective value achieved on the validation dataset.
     */
    template<ConstructionHeuristicConcept CHType>
    double learn(const LearnConfig &Config,
                 const CHType &CH,
                 vector<typename CHType::DataType> &TotalData,
                 vector<typename CHType::DataType> &ValidationData,
                 CHType &CHOut) {
        auto time_start = std::chrono::steady_clock::now();

        std::ofstream outfile2;
        if (Config.storeProgressInfo) {
            outfile2 = nnutils::openFileWithDirs<ofstream>(Config.progressFile, std::ios::app);
            outfile2 << "n-iter; fevals; curr_obj; best_obj; avg_fval; best_fval; time" << endl;
        }

        vector<typename CHType::DataType *> TrainingDataP;
        vector<typename CHType::DataType *> ValidationDataP;

        ValidationDataP.clear();
        for (auto &D: ValidationData) {
            ValidationDataP.push_back(&D);
        }

        if constexpr (InitializedDataConcept<typename CHType::DataType>) {
            for (auto VDP: ValidationDataP) {
                VDP->init();
            }
        }

        std::mt19937 gen(Config.seed);

        // Select the first training batch
        selectRandomData(gen, TotalData, Config.trainingDataSize, TrainingDataP);

        if constexpr (InitializedDataConcept<typename CHType::DataType>) {
            std::set<typename CHType::DataType *> unique_data(TrainingDataP.begin(), TrainingDataP.end());
            for (auto TDP: unique_data) {
                TDP->init();
            }
        }

        DataSetEvaluator DSE(TrainingDataP, CH);
        ParallelEvaluator PE(DSE);

        std::function<double(const double *, const int &n)> F = PE;

        int dim = CH.getParamsSize();

        // starting point
        std::vector<double> x0;
        CH.getParams(x0);

        vector<double> BestXX;
        double BestObj = (CH.maximize() ? -DBL_MAX : DBL_MAX);

        // Progress callback acting as the validation mechanism
        ProgressFunc<CMAParameters<>, CMASolutions> progress_func =
                [&](const CMAParameters<> &cmaparams, const CMASolutions &cmasols) {
            static int last_batch = 0;

            double ssum = 0.0;
            int nnum = 0;
            for (const Candidate &c: ((CMASolutions &) cmasols).candidates()) {
                ssum += c.get_fvalue();
                nnum += 1;
            }

            if (!Config.quiet) {
                cout << "\n" << std::setprecision(std::numeric_limits<double>::digits10) <<
                        "iter=" << cmasols.niter() <<
                        " / evals=" << cmasols.fevals() <<
                        " / avg-value=" << ssum / nnum <<
                        " / f-value=" << cmasols.best_candidate().get_fvalue() <<
                        " / best f-value=" << cmasols.get_best_seen_candidate().get_fvalue() <<
                        " / sigma=" << cmasols.sigma() <<
                        " / last_iter=" << cmasols.elapsed_last_iter() << flush;
            }

            auto Params = cmasols.best_candidate().get_x();
            int new_batch = cmasols.fevals() / (Config.itersToValidate * Config.population);

            // Run validation every 'itersToValidate' batches
            if ((new_batch != last_batch && cmasols.niter() > 0) || cmasols.niter() == 1) {
                last_batch = new_batch;

                // Evaluate current model on the held-out validation set
                ParallelDataSetEvaluator TSS(ValidationDataP, CH, Config.numValidationThreads);
                double TSSobj = TSS(Params.data(), Params.size());

                if (!Config.quiet) cout << "\nValidation Score (TSS.ResAvg) = " << TSSobj << "\n" << flush;

                // Save weights if this is the best validation score seen so far
                if (CH.maximize() ? TSSobj > BestObj : TSSobj < BestObj) {
                    BestObj = TSSobj;
                    BestXX = Params;
                }

                auto time_end = std::chrono::steady_clock::now();
                auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();
                double time_s = 0.001 * elapsed_ms;

                if (outfile2.is_open()) {
                    outfile2 << setprecision(4) << cmasols.niter() << "; " << cmasols.fevals() << "; " << TSSobj << "; "
                            << BestObj << "; " << ssum / nnum << "; " << cmasols.best_candidate().get_fvalue();
                    outfile2 << setprecision(2) << "; " << std::fixed << time_s << endl << flush;
                }

                if (Config.timeLimit > 0 && time_s > Config.timeLimit) {
                    return 1; // Early stop due to time limit
                }
            }

            // Clean memory and draw a NEW random mini-batch for the next generation
            if constexpr (InitializedDataConcept<typename CHType::DataType>) {
                std::set<typename CHType::DataType *> unique_data(TrainingDataP.begin(), TrainingDataP.end());
                for (auto TDP: unique_data) {
                    TDP->free();
                }
            }

            selectRandomData(gen, TotalData, Config.trainingDataSize, TrainingDataP);

            if constexpr (InitializedDataConcept<typename CHType::DataType>) {
                std::set<typename CHType::DataType *> unique_data(TrainingDataP.begin(), TrainingDataP.end());
                for (auto TDP: unique_data) {
                    TDP->init();
                }
            }

            return 0;
        };

        // Main CMA-ES Loop
        for (int it = 0; it < Config.NumEvals.size(); it++) {
            CMAParameters<> cmaparams(x0, Config.Sigmas[it], Config.population, (it + 1) * (Config.seed + 1) * 100);

            cmaparams.set_algo(sepaCMAES);
            cmaparams.set_mt_feval(Config.mt_feval);
            cout << "multi threading:" << (Config.mt_feval ? "yes" : "no") << endl << flush;
            cmaparams.set_max_fevals(Config.NumEvals[it]);
            cmaparams.set_quiet(Config.quiet);
            cmaparams.set_maximize(CH.maximize());
            cmaparams.set_stopping_criteria(EQUALFUNVALS, false);
            cmaparams.set_stopping_criteria(STAGNATION, false);

            CMASolutions cmasols = cmaes<>(F, cmaparams, progress_func);
            x0 = BestXX;

            if constexpr (InitializedDataConcept<typename CHType::DataType>) {
                std::set<typename CHType::DataType *> unique_data(TrainingDataP.begin(), TrainingDataP.end());
                for (auto TDP: unique_data) {
                    TDP->free();
                }
            }
        }

        outfile2.close();

        CHOut = CH;
        CHOut.setParams(BestXX.data(), BestXX.size());

        return BestObj;
    }

    /**
     * @brief A variation of the learning pipeline that guarantees a specific instance is in every training batch.
     * * Useful when the model should heavily prioritize performing well on a specific core instance
     * while still generalizing over the broader dataset.
     * @tparam CHType The Construction Heuristic type.
     * @param Config Learning configuration (batch size, validation frequency, etc.).
     * @param CH Initial heuristic model.
     * @param _Data The single problem instance to occur in every batch.
     * @param TotalData The complete pool of training instances.
     * @param ValidationData The held-out set used strictly for unbiased scoring.
     * @param CHOut The trained model (containing the best validation weights).
     * @return The best objective value achieved on the validation dataset.
     */
    template<ConstructionHeuristicConcept CHType>
    double learn2(const LearnConfig &Config,
                  const CHType &CH,
                  typename CHType::DataType &_Data,
                  vector<typename CHType::DataType> &TotalData,
                  vector<typename CHType::DataType> &ValidationData,
                  CHType &CHOut) {
        auto time_start = std::chrono::steady_clock::now();

        std::ofstream outfile2;
        if (Config.storeProgressInfo) {
            outfile2 = nnutils::openFileWithDirs<ofstream>(Config.progressFile, std::ios::app);
            outfile2 << "n-iter; fevals; curr_obj; best_obj; avg_fval; best_fval; time" << endl;
        }

        vector<typename CHType::DataType *> TrainingDataP;
        vector<typename CHType::DataType *> ValidationDataP;

        ValidationDataP.clear();
        for (auto &D: ValidationData) {
            ValidationDataP.push_back(&D);
        }

        if constexpr (InitializedDataConcept<typename CHType::DataType>) {
            for (auto VDP: ValidationDataP) {
                VDP->init();
            }
        }

        std::mt19937 gen(Config.seed);

        selectRandomData(gen, TotalData, Config.trainingDataSize, TrainingDataP);
        TrainingDataP.front() = &_Data; // Force first element to be the target core instance

        if constexpr (InitializedDataConcept<typename CHType::DataType>) {
            std::set<typename CHType::DataType *> unique_data(TrainingDataP.begin(), TrainingDataP.end());
            for (auto TDP: unique_data) {
                TDP->init();
            }
        }

        DataSetEvaluator DSE(TrainingDataP, CH);
        ParallelEvaluator PE(DSE);

        std::function<double(const double *, const int &n)> F = PE;

        int dim = CH.getParamsSize();
        // starting point
        std::vector<double> x0;
        CH.getParams(x0);

        vector<double> BestXX;
        double BestObj = (CH.maximize() ? -DBL_MAX : DBL_MAX);

        ProgressFunc<CMAParameters<>, CMASolutions> progress_func =
                [&](const CMAParameters<> &cmaparams, const CMASolutions &cmasols) {
            static int last_batch = 0;

            double ssum = 0.0;
            int nnum = 0;
            for (const Candidate &c: ((CMASolutions &) cmasols).candidates()) {
                ssum += c.get_fvalue();
                nnum += 1;
            }

            if (!Config.quiet) {
                cout << "\n" << std::setprecision(std::numeric_limits<double>::digits10) <<
                        "iter=" << cmasols.niter() <<
                        " / evals=" << cmasols.fevals() <<
                        " / avg-value=" << ssum / nnum <<
                        " / f-value=" << cmasols.best_candidate().get_fvalue() <<
                        " / best f-value=" << cmasols.get_best_seen_candidate().get_fvalue() <<
                        " / sigma=" << cmasols.sigma() <<
                        " / last_iter=" << cmasols.elapsed_last_iter();
            }

            auto Params = cmasols.best_candidate().get_x();
            int new_batch = cmasols.fevals() / (Config.itersToValidate * Config.population);

            if ((new_batch != last_batch && cmasols.niter() > 0) || cmasols.niter() == 1) {
                last_batch = new_batch;

                ParallelDataSetEvaluator TSS(ValidationDataP, CH, Config.numValidationThreads);
                double TSSobj = TSS(Params.data(), Params.size());

                if (!Config.quiet) cout << "\nValidation Score (TSS.ResAvg) = " << TSSobj << "\n" << flush;

                if (CH.maximize() ? TSSobj > BestObj : TSSobj < BestObj) {
                    BestObj = TSSobj;
                    BestXX = Params;
                }

                auto time_end = std::chrono::steady_clock::now();
                auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();
                double time_s = 0.001 * elapsed_ms;

                if (outfile2.is_open()) {
                    outfile2 << setprecision(4) << cmasols.niter() << "; " << cmasols.fevals() << "; " << TSSobj << "; "
                            << BestObj << "; " << ssum / nnum << "; " << cmasols.best_candidate().get_fvalue();
                    outfile2 << setprecision(2) << "; " << std::fixed << time_s << endl << flush;
                }

                if (Config.timeLimit > 0 && time_s > Config.timeLimit) {
                    return 1;
                }
            }

            if constexpr (InitializedDataConcept<typename CHType::DataType>) {
                std::set<typename CHType::DataType *> unique_data(TrainingDataP.begin(), TrainingDataP.end());
                for (auto TDP: unique_data) {
                    TDP->free();
                }
            }

            selectRandomData(gen, TotalData, Config.trainingDataSize, TrainingDataP);
            TrainingDataP.front() = &_Data; // Force core instance in every new batch

            if constexpr (InitializedDataConcept<typename CHType::DataType>) {
                std::set<typename CHType::DataType *> unique_data(TrainingDataP.begin(), TrainingDataP.end());
                for (auto TDP: unique_data) {
                    TDP->init();
                }
            }

            return 0;
        };


        for (int it = 0; it < Config.NumEvals.size(); it++) {
            CMAParameters<> cmaparams(x0, Config.Sigmas[it], Config.population, (it + 1) * (Config.seed + 1) * 100);

            cmaparams.set_algo(sepaCMAES);
            cmaparams.set_mt_feval(Config.mt_feval);
            if (!Config.quiet) cout << "multi threading:" << (Config.mt_feval ? "yes" : "no") << "\n" << flush;
            cmaparams.set_max_fevals(Config.NumEvals[it]);
            cmaparams.set_quiet(Config.quiet);
            cmaparams.set_maximize(CH.maximize());
            cmaparams.set_stopping_criteria(EQUALFUNVALS, false);
            cmaparams.set_stopping_criteria(STAGNATION, false);

            CMASolutions cmasols = cmaes<>(F, cmaparams, progress_func);
            x0 = BestXX;

            if constexpr (InitializedDataConcept<typename CHType::DataType>) {
                std::set<typename CHType::DataType *> unique_data(TrainingDataP.begin(), TrainingDataP.end());
                for (auto TDP: unique_data) {
                    TDP->free();
                }
            }
        }

        outfile2.close();

        CHOut = CH;
        CHOut.setParams(BestXX.data(), BestXX.size());

        return BestObj;
    }
}
