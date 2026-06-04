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
     * @brief Helper function that initializes a heuristic from config and starts the learning process.
     * * Creates a new TCH heuristic using the provided CHConf configuration, and delegates
     * the actual optimization to the main learn() function.
     * * @tparam TCH Construction Heuristic type (e.g., JobshopConstructionHeuristic).
     * @param LConf Configuration for the learning algorithm (CMA-ES parameters).
     * @param CHConf Configuration for the heuristic (Network topology, features).
     * @param TotalData Complete dataset available for drawing training batches.
     * @param ValidationData Dataset used strictly to evaluate model generalization.
     * @param chFilename Base name for the output file (without .dat extension) where the trained model will be saved.
     * @param resFilename Base name for the CSV results file.
     * @return The best objective value achieved on the validation set.
     */
    template<typename TCH>
    double learn(const LearnConfig &LConf, const typename TCH::ConfigType &CHConf,
                 vector<typename TCH::DataType> &TotalData, vector<typename TCH::DataType> &ValidationData,
                 string chFilename, string resFilename) {
        TCH CH(CHConf);

        return learn(LConf, CH, TotalData, ValidationData, chFilename, resFilename);
    }

    /**
     * @brief Main wrapper for the learning pipeline. Runs CMA-ES and serializes the trained model.
     * * This function ensures file safety (prevents overwriting trained models unless forced),
     * configures multithreading, executes the CMA-ES optimizer, and finally dumps the optimized
     * neural network weights to a binary file using boost::serialization.
     * * @tparam TCH Construction Heuristic type.
     * @param LConf Configuration for the learning algorithm.
     * @param CH The initial Construction Heuristic object to be optimized.
     * @param TotalData Complete training dataset.
     * @param ValidationData Validation dataset.
     * @param chFilename Output file name for the binary model.
     * @param resFilename Output file name for CSV logs.
     * @param force If false, the function aborts if the output .dat file already exists.
     * @return The best objective value achieved on the validation set.
     */
    template<typename TCH>
    double learn(const LearnConfig &LConf, const TCH &CH,
                 vector<typename TCH::DataType> &TotalData, vector<typename TCH::DataType> &ValidationData,
                 string chFilename, string resFilename, bool force = false) {
        if (TotalData.empty() || ValidationData.empty()) return 0;

        // Protection against overwriting an existing trained model
        {
            if (!force) {
                auto ifs = ifstream(chFilename + ".dat", ios::binary);
                if (ifs.is_open()) return 0.0;
            }
        }

        TCH CHOut;
        LearnConfig LConfig = LConf;

        // Disable multithreading in DEBUG mode
#ifndef NDEBUG
        LConfig.mt_feval = false;
#else
        LConfig.mt_feval = LConf.mt_feval;
#endif

        vector<double> ParamsOut;

        // Call CMA-ES algorithm
        double ret = chof::learn(LConfig, CH, TotalData, ValidationData, CHOut);

        // Serialize trained network to a binary file
        {
            auto ofs = nnutils::openFileWithDirs<ofstream>(chFilename + ".dat", ios::binary);
            boost::archive::binary_oarchive oa(ofs);
            oa << CHOut;
        }

        // Save logs to CSV file
        {
            auto ofs = nnutils::openFileWithDirs<ofstream>(resFilename + "_res.csv", ios::app);
            ofs << "learn" << "; " << chFilename << "; " << ret << "; " << endl << flush;
        }
        return ret;
    }


    /**
     * @brief Evaluates a dataset using a pre-trained Construction Heuristic.
     * * Loads a trained model from a .dat file. Depending on the OptConfig, it either directly
     * executes the heuristic (inference mode) or runs a short CMA-ES optimization locally
     * on each problem instance using the loaded weights as a starting point.
     * * @tparam TCH Construction Heuristic type.
     * @param OConf Optimization configuration (decides single evaluation vs local optimization).
     * @param chFilename Path to the binary model file (without .dat extension).
     * @param Data Vector of problem instances. Solved schedules are stored inside them.
     * @param resFilename Base name for detailed and summary CSV result files.
     * @return The average objective value (makespan) across the dataset.
     */
    template<typename TCH>
    double evaluate(const OptConfig &OConf, string chFilename, vector<typename TCH::DataType> &Data,
                    string resFilename) {
        // If evaluation limt = 1, model runs in Inference mode (only evaluation,
        bool single = (OConf.NumEvals.size() == 1 && OConf.NumEvals[0] == 1);

        if (Data.empty()) return 0.0;

        TCH BCH;

        // Deserialize trained model
        {
            auto ifs = nnutils::openFileWithDirs<ifstream>(chFilename + ".dat", ios::binary);
            boost::archive::binary_iarchive ia(ifs);
            ia >> BCH;
        }

        // Reset weights to 0 (for testing base algorithm behavior)
        if (OConf.zero) {
            int dim = BCH.getParamsSize();
            vector<double> Params(dim, 0.0);
            BCH.setParams(Params.data(), Params.size());
        }

        auto ofs = nnutils::openFileWithDirs<ofstream>(resFilename + ".csv", ios::app);
        auto ofs_det = nnutils::openFileWithDirs<ofstream>(resFilename + "_det.csv", ios::app);

        double sum = 0;
        int num = 0;

        auto time_start = std::chrono::steady_clock::now();

        // Evaluate all problem instances
        for (auto &D: Data) {
            typename TCH::DataType::SolutionType S;
            double time_ms = 0.0;

            if (single) {
                // Inference mode - just generate harmonogram
                auto time_start = std::chrono::steady_clock::now();
                S = BCH.run(D);
                auto time_end = std::chrono::steady_clock::now();
                auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(time_end - time_start).count();
                time_ms = (double) elapsed_us / 1000.0;
            } else {
                // Optimization mode - training the model for a specific problem
                TCH BCHOut;
                S = chof::opt(OConf, BCH, D, BCHOut);
            }
            D.setSolution(S);


            // Save details for a single instance
            ofs_det << "eval" << "; " << chFilename + ".dat" << "; " << D.name << "; " << S.getObj() << "; " << time_ms
                    << ";" << endl << flush;

            sum += S.getObj();
            num++;
        }

        auto time_end = std::chrono::steady_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();

        double time_single_ms = (double) elapsed_ms / Data.size();

        // Save averaged summary
        ofs << "eval" << "; " << chFilename << "; ; " << sum / num << "; " << time_single_ms << endl << flush;
        return sum / num;
    }

    /**
     * @brief Evaluates many problems, results ares stored in the Data vector. The results are also stored in the resFilename
     * * @tparam TCH Construction Heuristic type.
     * @param OConf depending on the configuration, each problem can be just solved using the TCH or further optimized starting with TCH
     * @param Data vector of input data to be evaluated. The results will be stored inside the data itself.
     * @param chFilename file name of the construction heuristic binary, without ".dat" extension
     * @param resFilename file name of the results, two files will be created, one with "_det" appendix, both files with ".csv" extension
     * @return The average objective value (makespan) across the dataset.
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

        // Pack base heuristic into cluster manager
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
