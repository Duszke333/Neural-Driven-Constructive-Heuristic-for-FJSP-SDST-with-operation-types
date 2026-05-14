//
// Created by tsliwins on 24.11.2025.
//
#include "io.h"
#include <filesystem>
#include <vector>
#include <string>
#include <iostream>
#include <stdexcept>
#include <limits.h>
#include "chof.h"
#include "Generator.h"
#include "JobshopConstructionHeuristic.h"
#include "options.h"

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/utility.hpp>

#include "Enums.h"
#include "utils.h"
#include "FFN.h"

#include "JobshopData.h"
#include "DataExport.h"
#include "JobshopDrawer.h"

namespace jobshop {
    namespace fs = std::filesystem;


    void readFjsFile(string file_name, const map<vector<pair<int, int> >, int> &OperationTypesMap, int M,
                     JobshopData &_IOD) {
        JobshopData IOD;

        auto getOT = [&](vector<pair<int, int> > &data) -> int {
            auto it = OperationTypesMap.find(data);
            if (it == OperationTypesMap.end()) {
                ERROR("Unknown operation type.");
            }
            return it->second;
        };

        auto ifs = nnutils::openFileWithDirs<ifstream>(file_name);


        IOD.name = std::filesystem::path(file_name).filename().string();
        IOD.Jobs.clear();

        int jidx = 0;
        int line_number = 0;
        std::string line;
        bool reading_setups = false;
        while (std::getline(ifs, line)) {
            // Skip empty lines
            if (line.empty() || line.find_first_not_of(" \t\r\n") == std::string::npos) continue;
            if (line.find("SDST") != std::string::npos) {
                reading_setups = true;
                break; // Wychodzimy z głównej pętli, przezbrojenia wczytamy osobną
            }

            // Read each line
            std::vector<int> numbers;
            std::istringstream iss(line);
            int num;

            while (iss >> num) {
                // Extract numbers from the line
                numbers.push_back(num);
            }

            if (numbers.empty()) continue;

            if (line_number == 0) {
                IOD.numJ = numbers[0];
                IOD.numM = numbers[1];
                if (IOD.numM != M) {
                    ERROR("Inconsistent number of machines in the input file (" + file_name + ": " + to_string(IOD.numM)
                        + " != " + to_string(M) + ")");
                }
            } else {
                set<int> OTs;
                IOD.Jobs.push_back(JobshopData::JType());
                IOD.Jobs.back().idx = jidx;
                jidx++;

                int idx = 1;

                // analyse all the operations
                for (int i = 0; i < numbers[0]; i++) {
                    vector<pair<int, int> > Alternatives;
                    int num_alt = numbers[idx++];
                    for (int a = 0; a < num_alt; a++) {
                        int m = numbers[idx++] - 1;
                        int t = numbers[idx++];
                        if (m >= IOD.numM) INTERNAL("m > numM");

                        Alternatives.push_back(make_pair(m, t));
                    }

                    int ot = getOT(Alternatives);
                    if (OTs.find(ot) != OTs.end()) {
                        ERROR("There may be only one operation of a give type in a single job (" + file_name + ": " +
                            to_string(line_number) + ", op:" + to_string(i) +")");
                    }
                    IOD.Jobs.back().Ops.push_back(ot);
                }
            }
            line_number++;
        }


        IOD.numO = OperationTypesMap.size();
        IOD.numJ = IOD.Jobs.size();

        IOD.OMtime.assign(IOD.numO, vector<int>(IOD.numM, 0));


        for (auto &OT: OperationTypesMap) {
            auto &Alts = OT.first;
            int ot = OT.second;
            if (ot >= IOD.numO) INTERNAL("Operation out of range");
            for (auto &alt: Alts) {
                if (alt.first >= IOD.numM) INTERNAL("Machine out of range");
                IOD.OMtime[ot][alt.first] = alt.second;
            }
        }

        // TODO: TEST
        IOD.setupTimes.assign(IOD.numO, std::vector<std::vector<int>>(IOD.numO, std::vector<int>(IOD.numM, 0)));

        if (reading_setups) {
            for (int m = 0; m < IOD.numM; ++m) {
                // Skipping "Machine X" line
                std::getline(ifs, line);

                for (int i = 0; i < IOD.numO; ++i) {
                    if (!std::getline(ifs, line)) break;
                    std::istringstream iss(line);
                    for (int j = 0; j < IOD.numO; ++j) {
                        if (int val; iss >> val) {
                            IOD.setupTimes[m][i][j] = val;
                        }
                    }
                }
            }
        }

        _IOD = IOD;
    }

    /**
      * Reads all the files from the dir_name into vector of JobshopData
      * @param dir_path
      * @return
      */
    void readFjsDir(string dir_path, const map<vector<pair<int, int> >, int> &OperationTypesMap, int M,
                    vector<JobshopData> &IODs) {
        IODs.clear();

        // Check if directory exists
        if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) {
            ERROR("Directory does not exist or is not a directory: " + dir_path);
        }

        // Iterate through directory entries
        for (const auto &entry: fs::directory_iterator(dir_path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".fjs") {
                JobshopData IOD;
                readFjsFile(entry.path().string(), OperationTypesMap, M, IOD);

                IODs.push_back(IOD);
            }
        }
    }


    /**
     * Extract operation types and machine number from a file
     * @param file_name
     * @param OperationTypesMap
     * @param M
     */
    void extractOperationTypesMachinesFromFile(string file_name, map<vector<pair<int, int> >, int> &OperationTypesMap,
                                               int &M) {
        auto get_or_insert = [&](vector<pair<int, int> > &data) -> int {
            auto [it, inserted] = OperationTypesMap.try_emplace(data, OperationTypesMap.size());
            return it->second;
        };

        auto ifs = nnutils::openFileWithDirs<ifstream>(file_name);


        string name = std::filesystem::path(file_name).filename().string();

        int jidx = 0;
        int line_number = 0;
        std::string line;

        while (std::getline(ifs, line)) {
            // Read each line
            std::vector<int> numbers;
            std::istringstream iss(line);
            int num;

            while (iss >> num) {
                // Extract numbers from the line
                numbers.push_back(num);
            }

            if (numbers.empty()) continue;

            if (line_number == 0) {
                if (M > 0 && M != numbers[1]) {
                    INTERNAL(
                        "Inconsistent number of machines over input files (" + file_name + ": " + to_string(numbers[1])
                        + " != " + to_string(M) + ")");
                }
                M = numbers[1];
            } else {
                int idx = 1;

                // analyse all the operations
                for (int i = 0; i < numbers[0]; i++) {
                    vector<pair<int, int> > Alternatives;
                    int num_alt = numbers[idx++];
                    for (int a = 0; a < num_alt; a++) {
                        int m = numbers[idx++] - 1;
                        int t = numbers[idx++];
                        if (m >= M) INTERNAL("m > M");

                        Alternatives.push_back(make_pair(m, t));
                    }

                    get_or_insert(Alternatives);
                }
            }

            line_number++;
        }
    }


    void extractOperationTypesAndMFromDir(string dir_name, map<vector<pair<int, int> >, int> &OperationTypesMap,
                                          int &M) {
        // Check if directory exists
        if (!fs::exists(dir_name) || !fs::is_directory(dir_name)) {
            ERROR("Directory does not exist or is not a directory: " + dir_name);
        }

        // init
        OperationTypesMap.clear();
        M = 0;

        // Iterate through directory entries
        for (const auto &entry: fs::directory_iterator(dir_name)) {
            if (entry.is_regular_file() && entry.path().extension() == ".fjs") {
                extractOperationTypesMachinesFromFile(entry.path().string(), OperationTypesMap, M);
            }
        }
    }

    void generateBrandimarte(Config Cfg) {
        typedef jobshop::GeneratorTxt GeneratorType;
        typedef GeneratorType::GenConfigType GenConfigType;
        typedef nnutils::FFN ASType;
        typedef jobshop::JobshopConstructionHeuristic<ASType, GenConfigType> TCHType;

        vector<TCHType::DataType> TotalData;

        string mkname = "BrandimarteMk" + to_string(Cfg.brandimarte);


        GenConfigType GConfTmp;
        GConfTmp.txtFileName = "Brandimarte/" + mkname + ".fjs";
        GConfTmp.seed = Cfg.seed;
        GConfTmp.multiTask = false;
        GConfTmp.RangeJ = make_pair(.8f, 1.2f);

        GeneratorType Generator(GConfTmp);

        Generator.load(Cfg.seed, Cfg.set_size, TotalData);

        jobshop::dataExport_fjs(TotalData, Cfg.output_dir + "/", mkname);

        auto ofs = nnutils::openFileWithDirs<ofstream>(Cfg.output_dir + "/" + "parameters.txt");
        ofs << Cfg;
    }

    void generateRandom(Config Cfg) {
        typedef jobshop::GeneratorRnd GeneratorType;
        typedef GeneratorType::GenConfigType GenConfigType;
        typedef nnutils::FFN ASType;
        typedef jobshop::JobshopConstructionHeuristic<ASType, GenConfigType> TCHType;

        GenConfigType GConfTmp;
        GConfTmp.nameBase = "synthetic"; // the name of the last dir
        GConfTmp.seedCommon = Cfg.common_seed;
        GConfTmp.seed = Cfg.seed;
        GConfTmp.numM = Cfg.machines;
        GConfTmp.numO = Cfg.operation_types;
        GConfTmp.multiOperation = false;
        GConfTmp.RangeOM = {Cfg.num_alt_min, Cfg.num_alt_max};
        GConfTmp.RangeJ = {Cfg.jobs_min, Cfg.jobs_max};
        GConfTmp.RangeJO = {Cfg.job_len_min, Cfg.job_len_max};
        GConfTmp.RangeD = {Cfg.t_min, Cfg.t_max};

        GeneratorType Generator(GConfTmp);

        vector<TCHType::DataType> TotalData;

        Generator.load(Cfg.seed, Cfg.set_size, TotalData);
        jobshop::dataExport_fjs(TotalData, Cfg.output_dir + "/", GConfTmp.nameBase);

        auto ofs = nnutils::openFileWithDirs<ofstream>(Cfg.output_dir + "/" + "parameters.txt");
        ofs << Cfg;
    }


    void train(Config Cfg) {
        std::filesystem::current_path(".");

        // typedef jobshop::GeneratorTxt GeneratorType;
        typedef jobshop::GeneratorRnd GeneratorType;

        typedef GeneratorType::GenConfigType GenConfigType;

        typedef nnutils::FFN ASType;
        typedef jobshop::JobshopConstructionHeuristic<ASType, GenConfigType> TCHType;

        TOperationsTypesMap OperationTypesMap;
        int M;
        vector<JobshopData> TotalData;

        cout << "Reading operation types from the input data..." << endl;
        extractOperationTypesAndMFromDir(Cfg.files_dir, OperationTypesMap, M);

        cout << "Reading input data..." << endl;
        readFjsDir(Cfg.files_dir, OperationTypesMap, M, TotalData);

        cout << to_string(TotalData.size()) << " files read." << endl;

        if (Cfg.val_set_size * 2 > TotalData.size()) {
            ERROR("Too small traing data set (Cfg.val_set_size < 2 * TotalData.size())");
        }

        vector<JobshopData> ValidationData(TotalData.begin(), TotalData.begin() + Cfg.val_set_size);
        TotalData.erase(TotalData.begin(), TotalData.begin() + Cfg.val_set_size);

        GenConfigType GConf;
        // //  GConf.dir = "data/jobshop";
        GConf.nameBase = "fjsp"; //Cfg.run_name;
        GConf.seedCommon = 1;
        GConf.seed = 10000;
        GConf.numM = M;
        GConf.numO = OperationTypesMap.size();
        GConf.multiOperation = false;
        GConf.RangeOM = make_pair<int, int>(0, 0);
        GConf.RangeJ = make_pair<int, int>(0, 0);
        GConf.RangeJO = make_pair<int, int>(0, 0);
        GConf.RangeD = make_pair<int, int>(0, 0);


        chof::LearnConfig LConf;
        LConf.trainingDataSize = Cfg.batch_size;
        LConf.seed = 1;
        LConf.population = Cfg.population;
        LConf.NumEvals = {Cfg.max_evals};
        LConf.Sigmas = {Cfg.sigma};
        LConf.mt_feval = true;
        LConf.quiet = false;
        LConf.itersToValidate = 10;
        LConf.numValidationThreads = 24;
        LConf.storeProgressInfo = true;
        LConf.progressFile = Cfg.output_dir + "/progress.csv";

        typename TCHType::ConfigType CHConf = {
            .desc = "Flexible Job Shop Constructive Heuristic",
            .noAutoScaleEval = false,
            .autoScale = false,
            .autoScaleNumOperationsInfo = false,
            .nextOperationInfo = false,
            .numOperationsInfo = true,
            .numAllOperationsInfo = false,
            .numM = GConf.numM,
            .numO = GConf.numO,
            .AConf = {
                .
                numInputs = 2 * GConf.numM + 2 * GConf.numO + GConf.numO + 1,
                .Topology = {Cfg.layer1, Cfg.layer2, 1}
            },
            .GConf = GConf
        };

        {
            auto ifs = ifstream(Cfg.output_dir + "/network.dat", ios::binary);
            if (ifs.is_open()) {
                ERROR("Results file already present, remove it, exiting... (" +Cfg.output_dir + "/network.dat)");
            }
        }


        LearnConfig LConfig = LConf;

#ifndef NDEBUG
        LConfig.mt_feval = false;
#else
        LConfig.mt_feval = LConf.mt_feval;
#endif

        vector<double> ParamsOut;

        TCHType CH(CHConf);
        TCHType CHOut;

        double ret = chof::learn(LConfig, CH, TotalData, ValidationData, CHOut);

        {
            auto ofs = nnutils::openFileWithDirs<ofstream>(Cfg.output_dir + "/network.dat", ios::binary);
            boost::archive::binary_oarchive oa(ofs);
            oa << CHOut;
            oa << M;
            oa << OperationTypesMap;

            auto outfile2 = nnutils::openFileWithDirs<ofstream>(Cfg.output_dir + "/operations.txt");
            outfile2 << "operation_type: num_alternatives (machine, time) ..." << endl;
            vector<vector<pair<int, int> > > Ops(OperationTypesMap.size());
            for (auto it: OperationTypesMap) {
                Ops[it.second] = it.first;
            }
            for (int i = 0; i < Ops.size(); i++) {
                auto &Op = Ops[i];
                outfile2 << std::right << std::setw(5) << i << ": ";
                outfile2 << Op.size() << " ";
                for (auto p: Op) {
                    outfile2 << "(" << p.first << ", " << p.second << ") ";
                }
                outfile2 << endl;
            }
        }
    }


    /**
     *
     *
     *
     * @param Cfg
     */
    void test(Config Cfg) {
        bool single = (Cfg.max_evals == 1);

        std::filesystem::current_path(".");

        // typedef jobshop::GeneratorTxt GeneratorType;
        typedef jobshop::GeneratorRnd GeneratorType;

        typedef GeneratorType::GenConfigType GenConfigType;


        typedef nnutils::FFN ASType;
        typedef jobshop::JobshopConstructionHeuristic<ASType, GenConfigType> TCHType;


        TOperationsTypesMap OperationTypesMap;
        int M;
        vector<JobshopData> TotalData;

        TCHType BCH;

        {
            cout << "Reading network from " + Cfg.training_output_dir + "/network.dat";
            auto ifs = nnutils::openFileWithDirs<ifstream>(Cfg.training_output_dir + "/network.dat", ios::binary);
            boost::archive::binary_iarchive ia(ifs);
            ia >> BCH;
            ia >> M;
            ia >> OperationTypesMap;
            cout << " ok." << endl << flush;
        }

        cout << "Reading input data...";
        readFjsDir(Cfg.files_dir, OperationTypesMap, M, TotalData);
        cout << " ok." << endl << flush;

        if (TotalData.empty()) {
            ERROR("No files in directory " + Cfg.files_dir);
        }

        auto ofs_sum = nnutils::openFileWithDirs<ofstream>(Cfg.output_dir + "/summary.csv", ios::app);

        auto ofs_det = nnutils::openFileWithDirs<ofstream>(Cfg.output_dir + "/details.csv", ios::app);


        double sum = 0;
        int num = 0;

        chof::OptConfig OConf;
        OConf.seed = Cfg.seed;
        OConf.population = Cfg.population;
        OConf.NumEvals = {Cfg.max_evals};
        OConf.Sigmas = {Cfg.sigma};
        OConf.mt_feval = true;
        OConf.quiet = true;
        OConf.storeProgressInfo = false;
        OConf.progressFile = "";
        OConf.timeLimit = Cfg.time_limit;


        auto time_start = std::chrono::steady_clock::now();

        for (auto &D: TotalData) {
            typename TCHType::DataType::SolutionType S;

            double time_ms = 0.0;
            auto time_start = std::chrono::steady_clock::now();
            if (single) {
                S = BCH.run(D);
            } else {
                TCHType BCHOut;
                S = chof::opt(OConf, BCH, D, BCHOut);
            }
            auto time_end = std::chrono::steady_clock::now();
            auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(time_end - time_start).count();
            time_ms = (double) elapsed_us / 1000.0;
            D.setSolution(S);

            cout << "." << flush;

            ofs_det << "eval" << "; " << Cfg.files_dir << "; " << D.name << "; " << S.getObj() << "; " << time_ms << ";"
                    << endl << flush;

            sum += S.getObj();
            num++;
        }

        auto time_end = std::chrono::steady_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();

        double time_avg_ms = (double) elapsed_ms / TotalData.size();

        ofs_sum << "eval" << "; " << Cfg.files_dir << "; ; " << sum / num << "; " << time_avg_ms << endl << flush;

        if (Cfg.graphics) {
            JobshopDrawer JD;
            for (auto &D: TotalData) {
                JD.drawToFile(D, Cfg.output_dir, ".png");
            }
        }

        if (Cfg.schedules) {
            for (auto &D: TotalData) {
                auto ofs_sch = nnutils::openFileWithDirs<ofstream>(Cfg.output_dir + "/schedule_" + D.name + ".csv");
                D.printSolution(ofs_sch);
            }
        }
    }
}
