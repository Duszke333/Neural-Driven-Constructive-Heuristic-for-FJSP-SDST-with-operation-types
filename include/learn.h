#pragma once
#include <random>
#include <iomanip>
#include "DataConcept.h"
#include "ConstructionHeuristicConcept.h"
#include "DataSetEvaluator.h"
#include "ParallelEvaluator.h"
#include "ParallelDataSetEvaluator.h"
#include "Err.h"
#include "float.h"
#include "libcmaes/cmaes.h"
#include "ReadConfig.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include "utils.h"


namespace chof {
    // Alias for cleaner code


    using namespace std;
    using namespace libcmaes;
    // using namespace nlohmann;


    struct OptConfig {
        int seed = 1;
        int population = 192;
        vector<int> NumEvals = {1};
        vector<double> Sigmas = {0.1};
        bool mt_feval = false;
        bool quiet = true;
        bool zero = false;
        bool storeProgressInfo = false;
        string progressFile = "";
        int timeLimit = 0;
        //< time after which the optimization/learning is interrupted. The best result is returned. 0 - no time limit

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


    struct LearnConfig : public OptConfig {
        int trainingDataSize;
        int itersToValidate; //< number of iteratios before applying validation
        int numValidationThreads; //< number of threads used for validation

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


    // /**
    //  * Optimizes a CH heuristic to best solve the _DataVec set of problems, the resulting heuristic is stored in CHOut
    //  * @tparam CHType
    //  * @param Config optimization paramters
    //  * @param CH
    //  * @param _DataVec
    //  * @param CHOut
    //  * @return
    //  */
    //
    // template<ConstructionHeuristicConcept CHType >
    // typename CHType::DataType::SolutionType opt(
    //     const OptConfig &Config,
    //     const CHType &CH,
    //     const typename CHType::DataType &_DataVec,
    //     CHType &CHOut) {
    //
    //     const vector<typename CHType::DataType> DataVec;
    //     DataVec.push_back(_DataVec);
    //     return opt<CHType>(Config, CH, DataVec, CHOut);
    // }


    /**
     * Optimizes a CH heuristic to best solve the _Data problem, the resulting heuristic is stored in CHOut
     * @tparam CHType
     * @param Config optimization paramters
     * @param CH
     * @param _DataVec
     * @param CHOut
     * @return
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

        std::function < double(const double*, const int&n) > F = PE;

        ProgressFunc<CMAParameters<>, CMASolutions> progress_func = // pfuncdef_impl<TCovarianceUpdate,TGenoPheno>;

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
            //!!!!!!!!!!!!!!!!!!!!!!!!!!!
            //cout << cmasols.niter() << "; " << cmasols.fevals() << "; " << ssum/nnum << "; " << cmasols.best_candidate().get_fvalue() << "; " << cmasols.get_best_seen_candidate().get_fvalue() << endl << flush;
            // cout << "." << flush;

            if (Config.timeLimit > 0 && time_s > Config.timeLimit) {
                // cout << cmasols.fevals() << endl;    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                return 1;
            } else {
                return 0;
            }
        };


        vector<double> BestParams;
        double BestObj = (CH.maximize() ? -DBL_MAX : DBL_MAX);
        typename CHType::DataType::SolutionType BestSolution;

        for (int it = 0; it < Config.NumEvals.size(); it++) {
            CMAParameters<> cmaparams(x0, Config.Sigmas[it], Config.population, (it + 1) * (Config.seed + 1) * 100);

            // -1 for automatically decided lambda, 0 is for random seeding of the internal generator.
            // cmaparams.set_algo(sepaCMAES);
            cmaparams.set_algo(sepaCMAES);
            cmaparams.set_mt_feval(Config.mt_feval);
            cmaparams.set_max_fevals(Config.NumEvals[it]);
            cmaparams.set_quiet(Config.quiet);
            cmaparams.set_maximize(CH.maximize());
            //  cmaparams.set_elitism(2);
            cmaparams.set_elitism(1);
            cmaparams.set_stopping_criteria(EQUALFUNVALS, false);
            cmaparams.set_stopping_criteria(STAGNATION, false);
            cmaparams.set_stopping_criteria(TOLHISTFUN, false); //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


            // CMASolutions cmasols = cmaes<>(F, cmaparams);
            CMASolutions cmasols = cmaes<>(F, cmaparams, progress_func);
            Candidate C = cmasols.get_best_seen_candidate();
            cout << endl << cmasols.status_msg() << endl << flush;

            auto Params = C.get_x();
            double obj = DSE(Params.data(), Params.size());

            if (CH.maximize() ? obj > BestObj : obj < BestObj) {
                BestParams = Params;
                BestObj = obj;
            }
            x0 = BestParams;
        }

        outfile2.close();

        CHOut = CH;

        CHOut.setParams(BestParams.data(), BestParams.size());

        return CHOut.run(Data);
    }


    /**
     * Optimizes a CH heuristic to best solve the _Data problem, the resulting heuristic is stored in CHOut
     * @tparam CHType
     * @param Config optimization paramters
     * @param CH
     * @param _DataVec
     * @param CHOut
     * @return
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

        std::function < double(const double*, const int&n) > F = PE;

        ProgressFunc<CMAParameters<>, CMASolutions> progress_func = // pfuncdef_impl<TCovarianceUpdate,TGenoPheno>;

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
            //!!!!!!!!!!!!!!!!!!!!!!!!!!!
            //cout << cmasols.niter() << "; " << cmasols.fevals() << "; " << ssum/nnum << "; " << cmasols.best_candidate().get_fvalue() << "; " << cmasols.get_best_seen_candidate().get_fvalue() << endl << flush;
            cout << "." << flush;

            if (Config.timeLimit > 0 && time_s > Config.timeLimit) {
                // cout << cmasols.fevals() << endl;    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                return 1;
            } else {
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
            // cmaparams.set_algo(sepaCMAES);
            cmaparams.set_algo(sepaCMAES);
            bool mt = Config.mt_feval;
#ifndef NDEBUG
            mt = false;
#endif
            cmaparams.set_mt_feval(mt);
            cmaparams.set_max_fevals(Config.NumEvals[it]);
            cmaparams.set_quiet(Config.quiet);
            cmaparams.set_maximize(CH.maximize());
            //  cmaparams.set_elitism(2);
            cmaparams.set_elitism(1);
            cmaparams.set_stopping_criteria(EQUALFUNVALS, false);
            cmaparams.set_stopping_criteria(STAGNATION, false);
            cmaparams.set_stopping_criteria(TOLHISTFUN, false); //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


            // CMASolutions cmasols = cmaes<>(F, cmaparams);
            CMASolutions cmasols = cmaes<>(F, cmaparams, progress_func);
            Candidate C = cmasols.get_best_seen_candidate();
            cout << endl << cmasols.status_msg() << endl << flush;

            auto Params = C.get_x();
            double obj = DSE(Params.data(), Params.size());

            if (CH.maximize() ? obj > BestObj : obj < BestObj) {
                BestParams = Params;
                BestObj = obj;
            }
            x0 = BestParams;
        }

        outfile2.close();

        CHOut = CH;

        CHOut.setParams(BestParams.data(), BestParams.size());

        return CHOut.run(_Data);
    }


    template<DataConcept DataType>
    void selectRandomData(std::mt19937 &gen, vector<DataType> &TotalData, int numData,
                          vector<DataType *> &SelectedData) {
        SelectedData.resize(numData);

        for (auto &TD: SelectedData) {
            TD = &(TotalData[uniform_int_distribution<>(0, TotalData.size() - 1)(gen)]);
        }
    }


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

        selectRandomData(gen, TotalData, Config.trainingDataSize, TrainingDataP);

        if constexpr (InitializedDataConcept<typename CHType::DataType>) {
            for (auto TDP: TrainingDataP) {
                TDP->init();
            }
        }

        DataSetEvaluator DSE(TrainingDataP, CH);

        ParallelEvaluator PE(DSE);

        std::function < double(const double*, const int&n) > F = PE;

        int dim = CH.getParamsSize();

        // starting point
        std::vector<double> x0;

        CH.getParams(x0);

        vector<double> BestXX;
        double BestObj;
        BestObj = (CH.maximize() ? -DBL_MAX : DBL_MAX);


        ProgressFunc<CMAParameters<>, CMASolutions> progress_func = // pfuncdef_impl<TCovarianceUpdate,TGenoPheno>;

                [&](const CMAParameters<> &cmaparams, const CMASolutions &cmasols) {
            static int last_batch = 0;

            double ssum = 0.0;
            int nnum = 0;
            for (const Candidate &c: ((CMASolutions &) cmasols).candidates()) {
                ssum += c.get_fvalue();
                nnum += 1;
            }

            cout << endl << std::setprecision(std::numeric_limits<double>::digits10) <<
                    "iter=" << cmasols.niter() <<
                    " / evals=" << cmasols.fevals() <<
                    " / avg-value=" << ssum / nnum <<
                    " / f-value=" << cmasols.best_candidate().get_fvalue() <<
                    " / best f-value=" << cmasols.get_best_seen_candidate().get_fvalue() <<
                    " / sigma=" << cmasols.sigma() <<
                    " / last_iter=" << cmasols.elapsed_last_iter() << flush;

            auto Params = cmasols.best_candidate().get_x();

            int new_batch = cmasols.fevals() / (Config.itersToValidate * Config.population);

            if ((new_batch != last_batch && cmasols.niter() > 0) || cmasols.niter() == 1) {
                last_batch = new_batch;

                // cout << "4" << endl << flush;
                ParallelDataSetEvaluator TSS(ValidationDataP, CH, Config.numValidationThreads);
                // cout << "5" << endl << flush;

                double TSSobj = TSS(Params.data(), Params.size());


                cout << endl << "TSS.ResAvg = " << TSSobj << endl << flush;

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
                for (auto TDP: TrainingDataP) {
                    TDP->free();
                }
            }

            selectRandomData(gen, TotalData, Config.trainingDataSize, TrainingDataP);

            if constexpr (InitializedDataConcept<typename CHType::DataType>) {
                for (auto TDP: TrainingDataP) {
                    TDP->init();
                }
            }

            return 0;
        };


        for (int it = 0; it < Config.NumEvals.size(); it++) {
            // cout << "1" << endl << flush;

            CMAParameters<> cmaparams(x0, Config.Sigmas[it], Config.population, (it + 1) * (Config.seed + 1) * 100);

            // -1 for automatically decided lambda, 0 is for random seeding of the internal generator.
            cmaparams.set_algo(sepaCMAES); // 2
            // cmaparams.set_algo(VD_BIPOP_CMAES ); // 104
            // cmaparams.set_algo(VD_CMAES ); // 103
            // cmaparams.set_algo(sepaBIPOP_CMAES); //102
            cmaparams.set_mt_feval(Config.mt_feval);
            cout << "multi threading:" << (Config.mt_feval ? "yes" : "no") << endl << flush;
            cmaparams.set_max_fevals(Config.NumEvals[it]);
            cmaparams.set_quiet(Config.quiet);
            cmaparams.set_maximize(CH.maximize());
            cmaparams.set_stopping_criteria(EQUALFUNVALS, false);
            cmaparams.set_stopping_criteria(STAGNATION, false);

            // cout << "1.5" << endl << flush;
            CMASolutions cmasols = cmaes<>(F, cmaparams, progress_func);
            // cout << "2" << endl << flush;
            x0 = BestXX;

            if constexpr (InitializedDataConcept<typename CHType::DataType>) {
                for (auto TDP: TrainingDataP) {
                    TDP->free();
                }
            }
        }

        outfile2.close();

        CHOut = CH;
        CHOut.setParams(BestXX.data(), BestXX.size());

        return BestObj;
    }


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
        TrainingDataP.front() = &_Data;

        if constexpr (InitializedDataConcept<typename CHType::DataType>) {
            for (auto TDP: TrainingDataP) {
                TDP->init();
            }
        }

        DataSetEvaluator DSE(TrainingDataP, CH);

        ParallelEvaluator PE(DSE);

        std::function < double(const double*, const int&n) > F = PE;

        int dim = CH.getParamsSize();

        // starting point
        std::vector<double> x0;

        CH.getParams(x0);

        vector<double> BestXX;
        double BestObj;
        BestObj = (CH.maximize() ? -DBL_MAX : DBL_MAX);


        ProgressFunc<CMAParameters<>, CMASolutions> progress_func = // pfuncdef_impl<TCovarianceUpdate,TGenoPheno>;

                [&](const CMAParameters<> &cmaparams, const CMASolutions &cmasols) {
            static int last_batch = 0;

            double ssum = 0.0;
            int nnum = 0;
            for (const Candidate &c: ((CMASolutions &) cmasols).candidates()) {
                ssum += c.get_fvalue();
                nnum += 1;
            }

            cout << endl << std::setprecision(std::numeric_limits<double>::digits10) <<
                    "iter=" << cmasols.niter() <<
                    " / evals=" << cmasols.fevals() <<
                    " / avg-value=" << ssum / nnum <<
                    " / f-value=" << cmasols.best_candidate().get_fvalue() <<
                    " / best f-value=" << cmasols.get_best_seen_candidate().get_fvalue() <<
                    " / sigma=" << cmasols.sigma() <<
                    " / last_iter=" << cmasols.elapsed_last_iter();

            auto Params = cmasols.best_candidate().get_x();

            int new_batch = cmasols.fevals() / (Config.itersToValidate * Config.population);

            if ((new_batch != last_batch && cmasols.niter() > 0) || cmasols.niter() == 1) {
                last_batch = new_batch;

                // cout << "\n4" << endl << flush;
                ParallelDataSetEvaluator TSS(ValidationDataP, CH, Config.numValidationThreads);
                // cout << "\n5" << endl << flush;

                double TSSobj = TSS(Params.data(), Params.size());


                cout << endl << "TSS.ResAvg = " << TSSobj << endl << flush;

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
                for (auto TDP: TrainingDataP) {
                    TDP->free();
                }
            }

            selectRandomData(gen, TotalData, Config.trainingDataSize, TrainingDataP);
            TrainingDataP.front() = &_Data;

            if constexpr (InitializedDataConcept<typename CHType::DataType>) {
                for (auto TDP: TrainingDataP) {
                    TDP->init();
                }
            }

            return 0;
        };


        for (int it = 0; it < Config.NumEvals.size(); it++) {
            CMAParameters<> cmaparams(x0, Config.Sigmas[it], Config.population, (it + 1) * (Config.seed + 1) * 100);

            // -1 for automatically decided lambda, 0 is for random seeding of the internal generator.
            cmaparams.set_algo(sepaCMAES);
            cmaparams.set_mt_feval(Config.mt_feval);
            cout << "multi threading:" << (Config.mt_feval ? "yes" : "no") << endl << flush;
            cmaparams.set_max_fevals(Config.NumEvals[it]);
            cmaparams.set_quiet(Config.quiet);
            cmaparams.set_maximize(CH.maximize());
            cmaparams.set_stopping_criteria(EQUALFUNVALS, false);
            cmaparams.set_stopping_criteria(STAGNATION, false);

            // cout << "1" << endl << flush;
            CMASolutions cmasols = cmaes<>(F, cmaparams, progress_func);
            // cout << "2" << endl << flush;
            x0 = BestXX;

            if constexpr (InitializedDataConcept<typename CHType::DataType>) {
                for (auto TDP: TrainingDataP) {
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