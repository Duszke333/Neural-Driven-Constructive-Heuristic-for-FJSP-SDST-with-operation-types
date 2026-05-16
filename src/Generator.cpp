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
    void GeneratorRnd::load(int _seed, int num, vector<JobshopData> &IODs) {
        if (_seed >= 0) {
            GConf.seed = _seed;
        }

        mt19937 genCommon(GConf.seedCommon);
        mt19937 gen(GConf.seed);

        IODs.clear();


        IODs.resize(num);

        // generate OMtime matrix with processing times of operations on machines
        // this also includes the sets of machines capable of precessing operations
        // common all generated data

        vector<vector<int> > OMtime(GConf.numO, vector<int>(GConf.numM, 0));

        if (GConf.RangeOM.second > GConf.numM) INTERNAL("RangeTM.second > numM");

        for (auto &Mtime: OMtime) {
            //< how many machines will process given operation
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

        // TODO: read flags from CLI
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

        // generate all the problems with common OMtime matrix
        for (int j = 0; j < IODs.size(); j++) {
            JobshopData &IOD = IODs[j];

            IOD.name = GConf.nameBase + "_" + nnutils::to_string(j, 3);

            IOD.numJ = uniform_int_distribution<int>(GConf.RangeJ.first, GConf.RangeJ.second)(gen);
            IOD.numM = GConf.numM;
            IOD.numO = GConf.numO;

            // generate jobs and their operations
            IOD.Jobs.resize(IOD.numJ);

            for (int j = 0; j < IOD.Jobs.size(); j++) {
                auto &J = IOD.Jobs[j];
                J.idx = j;

                // generate number of operations for a given job
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

    void GeneratorTxt::readTxtFile() {
        map<vector<pair<int, int> >, int> OperationTypesMap; //< vector<pair<machine, time>> -> idx

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
                CommonIOD.numJ = numbers[0];
                CommonIOD.numM = numbers[1];
            } else {
                CommonIOD.Jobs.push_back(JobshopData::JType());
                CommonIOD.Jobs.back().idx = jidx;
                jidx++;

                int idx = 1;

                // analyse all the operations
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


        for (auto &OT: OperationTypesMap) {
            auto &Alts = OT.first;
            int ot = OT.second;
            if (ot >= CommonIOD.numO) INTERNAL("Operation out of range");
            for (auto &alt: Alts) {
                if (alt.first >= CommonIOD.numM) INTERNAL("Machine out of range");
                CommonIOD.OMtime[ot][alt.first] = alt.second;
            }
        }

        // odczytanie zakresu liczby operacji w jobie
        int min_jo = INT_MAX, max_jo = INT_MIN;

        for (auto &J: CommonIOD.Jobs) {
            min_jo = min(min_jo, (int) J.Ops.size());
            max_jo = max(max_jo, (int) J.Ops.size());
        }

        RangeJO = make_pair(min_jo, max_jo);

        GConf.numM = CommonIOD.numM;
        GConf.numO = CommonIOD.numO;

        // // duplikacja job'ów
        // auto Jobs = CommonIOD.Jobs;
        // int iii = Jobs.size();
        // for ( auto &J: Jobs ) {
        //     J.idx = iii;
        //     ++iii;
        // }
        // CommonIOD.Jobs.insert(CommonIOD.Jobs.end(), Jobs.begin(), Jobs.end()); //!!!!!!!!!!!!!!!!!!!!!
        // CommonIOD.numJ *= 2;
    }


    void GeneratorTxt::load(int _seed, int num, vector<JobshopData> &IODs) {
        if (_seed >= 0) {
            GConf.seed = _seed;
        }


        mt19937 gen(GConf.seed);

        IODs.clear();


        IODs.resize(num);


        // TODO: setup times

        // generate all the problems with common OMtime matrix
        for (int j = 0; j < IODs.size(); j++) {
            JobshopData &IOD = IODs[j];

            IOD.name = CommonIOD.name + "_" + nnutils::to_string(j, 6);

            int j_min = (int) (round(GConf.RangeJ.first * CommonIOD.numJ));
            int j_max = (int) (round(GConf.RangeJ.second * CommonIOD.numJ));
            if (j_min > j_max) j_min = j_max;

            IOD.numJ = uniform_int_distribution<int>(j_min, j_max)(gen);
            IOD.numM = CommonIOD.numM;
            IOD.numO = CommonIOD.numO;

            // generate jobs and their operations
            IOD.Jobs.resize(IOD.numJ);

            for (int j = 0; j < IOD.Jobs.size(); j++) {
                auto &J = IOD.Jobs[j];
                J.idx = j;

                // generate number of operations for a given job
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
