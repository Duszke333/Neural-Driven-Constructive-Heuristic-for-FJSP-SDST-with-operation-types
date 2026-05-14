//
// Created by tsliwins on 14.01.25.
//

#pragma once
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <nlohmann/json.hpp>

#include <filesystem>

#include "chof.h"
#include "utils.h"


namespace chof {
    using namespace std;

    /**
     * Creates a new  TCH heuristic with CHConf configuration, and optimizes it ona sets TotalData and ValidationData.
     * The resulting heuristic is stored to the chFilename file.
     * The learning results (avg objective of ValidationData) is stored to the resFilename + "_res.csv" file.
     * @tparam TCH
     * @param LConf
     * @param CHConf
     * @param TotalData
     * @param ValidationData
     * @param chFilename
     * @param resFilename
     * @return
     */
    template<typename TCH>
    double learn(const LearnConfig &LConf, const typename TCH::ConfigType &CHConf,
                 vector<typename TCH::DataType> &TotalData, vector<typename TCH::DataType> &ValidationData,
                 string chFilename, string resFilename) {
        TCH CH(CHConf);

        return learn(LConf, CH, TotalData, ValidationData, chFilename, resFilename);
    }

    //
    //


    /**
     * Creates a new  TCH heuristic with CHConf configuration, and optimizes it ona sets TotalData and ValidationData.
     * The resulting heuristic is stored to the chFilename file.
     * The learning results (avg objective of ValidationData) is stored to the resFilename + "_res.csv" file.
     * @tparam TCH
     * @param LConf
     * @param CHConf
     * @param TotalData
     * @param ValidationData
     * @param chFilename
     * @param resFilename
     * @return
     */
    template<typename TCH>
    double learn(const LearnConfig &LConf, const TCH &CH,
                 vector<typename TCH::DataType> &TotalData, vector<typename TCH::DataType> &ValidationData,
                 string chFilename, string resFilename, bool force = false) {
        if (TotalData.empty() || ValidationData.empty()) return 0;

        {
            // if ConstructionHeuristic file exists, exit
            if (!force) {
                auto ifs = ifstream(chFilename + ".dat", ios::binary);
                if (ifs.is_open()) return 0.0;
            }
        }

        TCH CHOut;

        LearnConfig LConfig = LConf;

#ifndef NDEBUG
        LConfig.mt_feval = false;
#else
        LConfig.mt_feval = LConf.mt_feval;
#endif

        vector<double> ParamsOut;

        double ret = chof::learn(LConfig, CH, TotalData, ValidationData, CHOut);

        {
            auto ofs = nnutils::openFileWithDirs<ofstream>(chFilename + ".dat", ios::binary);
            boost::archive::binary_oarchive oa(ofs);
            oa << CHOut;
        }

        {
            auto ofs = nnutils::openFileWithDirs<ofstream>(resFilename + "_res.csv", ios::app);
            ofs << "learn" << "; " << chFilename << "; " << ret << "; " << endl << flush;
        }
        return ret;
    }


    /**
         * Evaluates many problems, results ares stored in the Data vector. The results are also stored in the resFilename
         * @tparam TCH Construction heuristic
         * @param OConf depending on the configuration, each problem can be just solved using the TCH or further optimized starting with TCH
         * @param Data vector of input data to be evaluated. The results will be stored inside the data itself.
         * @param chFilename file name of the construction heuristic binary, without ".dat" extension
         * @param resFilename file name of the results, two files will be created, one with "_det" appendix, both files with ".csv" extension
         * @return
         */
    template<typename TCH>
    double evaluate(const OptConfig &OConf, string chFilename, vector<typename TCH::DataType> &Data,
                    string resFilename) {
        bool single = (OConf.NumEvals.size() == 1 && OConf.NumEvals[0] == 1);

        if (Data.empty()) return 0.0;

        TCH BCH;

        {
            auto ifs = nnutils::openFileWithDirs<ifstream>(chFilename + ".dat", ios::binary);
            boost::archive::binary_iarchive ia(ifs);
            ia >> BCH;
        }

        if (OConf.zero) {
            int dim = BCH.getParamsSize();
            vector<double> Params(dim, 0.0);
            BCH.setParams(Params.data(), Params.size());
        }

        //  BCH.Conf.autoScale = false; //!!!!!!!!!!!!!!!!!!!!!!!!!1

        auto ofs = nnutils::openFileWithDirs<ofstream>(resFilename + ".csv", ios::app);

        auto ofs_det = nnutils::openFileWithDirs<ofstream>(resFilename + "_det.csv", ios::app);

        double sum = 0;
        int num = 0;

        auto time_start = std::chrono::steady_clock::now();

        for (auto &D: Data) {
            typename TCH::DataType::SolutionType S;

            double time_ms = 0.0;
            if (single) {
                auto time_start = std::chrono::steady_clock::now();
                S = BCH.run(D);
                auto time_end = std::chrono::steady_clock::now();
                auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(time_end - time_start).count();
                time_ms = (double) elapsed_us / 1000.0;
            } else {
                TCH BCHOut;
                S = chof::opt(OConf, BCH, D, BCHOut);
            }
            D.setSolution(S);


            ofs_det << "eval" << "; " << chFilename + ".dat" << "; " << D.name << "; " << S.getObj() << "; " << time_ms
                    << ";" << endl << flush;

            sum += S.getObj();
            num++;
        }

        auto time_end = std::chrono::steady_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();

        double time_single_ms = (double) elapsed_ms / Data.size();

        ofs << "eval" << "; " << chFilename << "; ; " << sum / num << "; " << time_single_ms << endl << flush;
        return sum / num;
    }

    /**
         * Evaluates many problems, results ares stored in the Data vector. The results are also stored in the resFilename
         * @tparam TCH Construction heuristic
         * @param OConf depending on the configuration, each problem can be just solved using the TCH or further optimized starting with TCH
         * @param Data vector of input data to be evaluated. The results will be stored inside the data itself.
         * @param chFilename file name of the construction heuristic binary, withoud ".dat" extension
         * @param resFilename file name of the results, two files will be created, one with "_det" appendix, both files with ".csv" extension
         * @return
         */
    template<typename TCH>
    double evaluateClustered(const OptConfig &OConf, string chFilename, vector<typename TCH::DataType> &Data,
                             string resFilename) {
        bool single = (OConf.NumEvals.size() == 1 && OConf.NumEvals[0] == 1);

        if (Data.empty()) return 0.0;

        TCH BCH;

        {
            auto ifs = nnutils::openFileWithDirs<ifstream>(chFilename + ".dat", ios::binary);
            boost::archive::binary_iarchive ia(ifs);
            ia >> BCH;
        }

        typedef ClusteringConstructionHeuristic<TCH> TCCH;
        TCCH CCH(4, BCH);

        if (OConf.zero) {
            int dim = CCH.getParamsSize();
            vector<double> Params(dim, 0.0);
            CCH.setParams(Params.data(), Params.size());
        }

        auto ofs = nnutils::openFileWithDirs<ofstream>(resFilename + ".csv", ios::app);

        auto ofs_det = nnutils::openFileWithDirs<ofstream>(resFilename + "_det.csv", ios::app);

        double sum = 0;
        int num = 0;

        for (auto &D: Data) {
            typename TCCH::DataType::SolutionType S;

            if (single) {
                S = CCH.run(D);
            } else {
                TCCH BCHOut;
                S = chof::opt<TCCH>(OConf, CCH, D, BCHOut);
            }
            D.setSolution(S);


            ofs_det << "eval" << "; " << chFilename + ".dat" << "; " << D.name << "; " << (int) (round(S.getObj())) <<
                    "; " << endl;

            sum += round(S.getObj());
            num++;
        }

        ofs << "eval" << "; " << chFilename << "; ; " << sum / num << "; " << endl << flush;
        return sum / num;
    }
}