#include <fstream>
#include <sstream>
#include <filesystem>
#include <random>
#include <set>
#include "Generator.h"
#include "Err.h"
#include "utils.h"
#include "SetupTimeGenerator.h"

namespace jobshop {
    // ===============================
    // RANDOM SYNTHETIC DATA GENERATOR
    // ===============================

    void GeneratorRnd::load(int _seed, int num, vector<JobshopData> &IODs) {
        if (_seed >= 0) {
            GConf.seed = _seed;
        }

        mt19937 genCommon(GConf.seedCommon);
        mt19937 gen(GConf.seed);

        IODs.clear();


        IODs.resize(num);

        // 1. Generate OMtime matrix with processing times of operations on machines.
        // This also includes the sets of machines capable of processing given operations.
        // This matrix is common to all generated instances in this specific batch.
        vector<vector<int> > OMtime(GConf.numO, vector<int>(GConf.numM, 0));

        if (GConf.RangeOM.second > GConf.numM) INTERNAL("RangeTM.second > numM");

        for (auto &Mtime: OMtime) {
            // Determine how many machines will process the given operation
            int M = uniform_int_distribution<int>(GConf.RangeOM.first, GConf.RangeOM.second)(genCommon);

            fill(Mtime.begin(), Mtime.end(), 0);

            set<int> MachineSet;
            while (MachineSet.size() < M) {
                MachineSet.insert(uniform_int_distribution<int>(0, GConf.numM - 1)(genCommon));
            }

            for (auto m: MachineSet) {
                Mtime[m] = uniform_int_distribution<int>(GConf.RangeD.first, GConf.RangeD.second)(genCommon);
            }
        }

        // 2. Generate Sequence-Dependent Setup Times (SDST) based on the OMtime matrix
        // TODO: read these parameters directly from CLI Config
        SetupConfig sConfig{};
        sConfig.variant = 5;
        sConfig.eta = 1.0;
        sConfig.a = 0.2;
        sConfig.b = 0.4;
        sConfig.alpha = 0.5;

        std::vector<std::vector<std::vector<int> > > globalSetupTimes;
        if (sConfig.variant > 0) {
            auto setupGen = createSetupTimeGenerator(sConfig.variant);
            globalSetupTimes = setupGen->generate(OMtime, GConf.numM, GConf.numO, sConfig, genCommon);
        }

        // 3. Generate all specific problem instances utilizing the common matrices
        for (int j = 0; j < IODs.size(); j++) {
            JobshopData &IOD = IODs[j];

            IOD.name = GConf.nameBase + "_" + nnutils::to_string(j, 3);

            IOD.numJ = uniform_int_distribution<int>(GConf.RangeJ.first, GConf.RangeJ.second)(gen);
            IOD.numM = GConf.numM;
            IOD.numO = GConf.numO;

            // Generate jobs and their operations
            IOD.Jobs.resize(IOD.numJ);

            for (int j = 0; j < IOD.Jobs.size(); j++) {
                auto &J = IOD.Jobs[j];
                J.idx = j;

                // Generate the number of operations for this specific job
                int noj = uniform_int_distribution<int>(GConf.RangeJO.first, GConf.RangeJO.second)(gen);

                if (GConf.multiOperation) {
                    for (int i = 0; i < noj; i++) {
                        J.Ops.push_back(uniform_int_distribution<int>(0, GConf.numO - 1)(gen));
                    }
                } else {
                    if (GConf.RangeJO.second > GConf.numO) INTERNAL("RangeJO.second > numO");
                    set<int> OpSet;
                    while (OpSet.size() < noj) {
                        OpSet.insert(uniform_int_distribution<int>(0, GConf.numO - 1)(gen));
                    }
                    for (auto o: OpSet) {
                        J.Ops.push_back(o);
                    }
                    shuffle(J.Ops.begin(), J.Ops.end(), gen);
                }
            }

            IOD.OMtime = OMtime;
            IOD.setupTimes = globalSetupTimes;
        }
    };

    // =================================================
    // TEXT FILE BENCHMARK GENERATOR (e.g., Brandimarte)
    // =================================================

    void GeneratorTxt::readTxtFile() {
        // Map associating unique combinations of (machine, processing_time) with an Operation ID
        map<vector<pair<int, int> >, int> OperationTypesMap;

        auto get_or_insert = [&](vector<pair<int, int> > &data) -> int {
            auto [it, inserted] = OperationTypesMap.try_emplace(data, OperationTypesMap.size());
            return it->second;
        };

        auto ifs = openFileWithDirs<ifstream>(GConf.txtFileName);

        CommonIOD.name = std::filesystem::path(GConf.txtFileName).filename().string();
        CommonIOD.Jobs.clear();

        int jidx = 0;
        int line_number = 0;
        std::string line;

        // 1. Parse the text file line by line
        while (std::getline(ifs, line)) {
            std::vector<int> numbers;
            std::istringstream iss(line);
            int num;

            while (iss >> num) {
                // Extract numbers from the line
                numbers.push_back(num);
            }

            if (numbers.empty()) continue;

            if (line_number == 0) {
                // First line contains global dimensions
                CommonIOD.numJ = numbers[0];
                CommonIOD.numM = numbers[1];
            } else {
                CommonIOD.Jobs.push_back(JobshopData::JType());
                CommonIOD.Jobs.back().idx = jidx;
                jidx++;

                int idx = 1;

                // Analyze all operations for the current job
                for (int i = 0; i < numbers[0]; i++) {
                    vector<pair<int, int> > Alternatives;
                    int num_alt = numbers[idx++];
                    for (int a = 0; a < num_alt; a++) {
                        int m = numbers[idx++] - 1;
                        int t = numbers[idx++];
                        if (m >= CommonIOD.numM) INTERNAL("m > numM");

                        Alternatives.push_back(make_pair(m, t));
                    }

                    int ot = get_or_insert(Alternatives);
                    CommonIOD.Jobs.back().Ops.push_back(ot);
                }
            }
            line_number++;
        }

        CommonIOD.numO = OperationTypesMap.size();
        CommonIOD.OMtime.assign(CommonIOD.numO, vector<int>(CommonIOD.numM, 0));

        // 2. Reconstruct the global OMtime matrix from parsed operations
        for (auto &OT: OperationTypesMap) {
            auto &Alts = OT.first;
            int ot = OT.second;
            if (ot >= CommonIOD.numO) INTERNAL("Operation out of range");
            for (auto &alt: Alts) {
                if (alt.first >= CommonIOD.numM) INTERNAL("Machine out of range");
                CommonIOD.OMtime[ot][alt.first] = alt.second;
            }
        }

        // 3. Determine the minimum and maximum number of operations per job in the dataset
        int min_jo = INT_MAX, max_jo = INT_MIN;

        for (auto &J: CommonIOD.Jobs) {
            min_jo = min(min_jo, (int) J.Ops.size());
            max_jo = max(max_jo, (int) J.Ops.size());
        }

        RangeJO = make_pair(min_jo, max_jo);
        GConf.numM = CommonIOD.numM;
        GConf.numO = CommonIOD.numO;
    }


    void GeneratorTxt::load(int _seed, int num, vector<JobshopData> &IODs) {
        if (_seed >= 0) {
            GConf.seed = _seed;
        }

        mt19937 gen(GConf.seed);

        IODs.clear();
        IODs.resize(num);

        // TODO: Generate or load setup times for text-based instances

        // Generate all instance variations based on the common text-loaded structure
        for (int j = 0; j < IODs.size(); j++) {
            JobshopData &IOD = IODs[j];

            IOD.name = CommonIOD.name + "_" + nnutils::to_string(j, 6);

            int j_min = (int) (round(GConf.RangeJ.first * CommonIOD.numJ));
            int j_max = (int) (round(GConf.RangeJ.second * CommonIOD.numJ));
            if (j_min > j_max) j_min = j_max;

            IOD.numJ = uniform_int_distribution<int>(j_min, j_max)(gen);
            IOD.numM = CommonIOD.numM;
            IOD.numO = CommonIOD.numO;

            // Generate jobs and their operations
            IOD.Jobs.resize(IOD.numJ);

            for (int j = 0; j < IOD.Jobs.size(); j++) {
                auto &J = IOD.Jobs[j];
                J.idx = j;

                // Generate number of operations for a given job based on the original file's range
                int noj = uniform_int_distribution<int>(RangeJO.first, RangeJO.second)(gen);

                if (GConf.multiTask) {
                    for (int i = 0; i < noj; i++) {
                        J.Ops.push_back(uniform_int_distribution<int>(0, CommonIOD.numO - 1)(gen));
                    }
                } else {
                    if (RangeJO.second > CommonIOD.numO) INTERNAL("RangeJO.second > numO");
                    set<int> OpSet;
                    while (OpSet.size() < noj) {
                        OpSet.insert(uniform_int_distribution<int>(0, CommonIOD.numO - 1)(gen));
                    }
                    for (auto o: OpSet) {
                        J.Ops.push_back(o);
                    }
                    shuffle(J.Ops.begin(), J.Ops.end(), gen);
                }
            }

            IOD.OMtime = CommonIOD.OMtime;
        }
    };
}
