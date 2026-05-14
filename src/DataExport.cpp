//
// Created by tsliwins on 18.01.25.
//


#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include "utils.h"
#include "DataExport.h"
#include "Err.h"


namespace jobshop {
    using namespace std;
    using json = nlohmann::json;

    struct Instance {
        string name;
        int jobs;
        int machines;
        string optimum;
        string path;
    };

    inline void to_json(nlohmann::json &j, const Instance &I) {
        j = nlohmann::json{};
        j.emplace("name", I.name);
        j.emplace("jobs", I.jobs);
        j.emplace("machines", I.machines);
        j.emplace("optimum", I.optimum);
        j.emplace("path", I.path);
    }

    inline void from_json(const nlohmann::json &j, Instance &I) {
        j.at("name").get_to(I.name);
        j.at("jobs").get_to(I.jobs);
        j.at("machines").get_to(I.machines);
        j.at("optimum").get_to(I.optimum);
        j.at("path").get_to(I.path);
    }

    struct Instance2 {
        string name;
        int numJ;
        int numM;
        int numO;
        vector<vector<vector<pair<int, int> > > > Jobs;
    };


    using json = nlohmann::json;
    using namespace std;


    inline void from_json(const nlohmann::json &j, Instance2 &I) {
        j.at("name").get_to(I.name);
        j.at("numJ").get_to(I.numJ);
        j.at("numM").get_to(I.numM);
        j.at("numO").get_to(I.numO);
        j.at("Jobs").get_to(I.Jobs);
    }


    Instance2 createInstance2(const JobshopData &DIO) {
        Instance2 I;
        I.name = DIO.name;
        I.numJ = DIO.numJ;
        I.numM = DIO.numM;
        I.numO = DIO.numO;
        I.Jobs.clear();

        for (int j = 0; j < DIO.Jobs.size(); j++) {
            I.Jobs.push_back(vector<vector<pair<int, int> > >{});

            for (auto o: DIO.Jobs[j].Ops) {
                I.Jobs.back().push_back(vector<pair<int, int> >{});

                for (int m = 0; m < DIO.OMtime[o].size(); m++) {
                    if (DIO.OMtime[o][m] > 0) {
                        I.Jobs.back().back().push_back(pair<int, int>{DIO.OMtime[o][m], m});
                    }
                }
            }
        }

        return I;
    }


    // Custom function to print JSON with line breaks after each vector<int>
    void printInstance2(ofstream &ofs, Instance2 &I, int indent = 0) {
        string indentation(indent, ' ');

        ofs << "{" << endl;

        ofs << "\"name\": \"" << I.name << "\"," << endl;
        ofs << "\"numJ\": " << I.numJ << "," << endl;
        ofs << "\"numM\": " << I.numM << "," << endl;
        ofs << "\"numO\": " << I.numO << "," << endl;

        ofs << "\"Jobs\": [" << endl;
        for (int j = 0; j < I.Jobs.size(); j++) {
            ofs << "[" << endl;
            auto &J = I.Jobs[j];
            for (int t = 0; t < J.size(); t++) {
                auto &T = J[t];
                ofs << "[";
                for (int p = 0; p < T.size(); p++) {
                    auto &P = T[p];
                    ofs << "[" << P.first << "," << P.second << "]";

                    if (p < T.size() - 1) {
                        ofs << ", ";
                    }
                }
                ofs << "]";
                if (t < J.size() - 1) {
                    ofs << "," << endl;
                } else {
                    ofs << endl;
                }
            }

            ofs << "]" << endl;
            if (j < I.Jobs.size() - 1) {
                ofs << "," << endl;
            } else {
                ofs << endl;
            }
        }
        ofs << "]";

        ofs << "}";
    }


    void dataExport2(const vector<JobshopData> &DIOs, string dir, string instancesFile) {
        vector<Instance2> Instances;

        for (auto &DIO: DIOs) {
            Instances.push_back(createInstance2(DIO));
        }

        {
            string fn = dir + "/" + instancesFile;
            ofstream ofs(fn);
            if (!ofs.is_open()) {
                INTERNAL(string("Error opening output file ") + fn);
            };

            ofs << "[" << endl;
            for (int i = 0; i < Instances.size(); i++) {
                printInstance2(ofs, Instances[i], 0);
                if (i < Instances.size() - 1) {
                    ofs << "," << endl;
                } else {
                    ofs << endl;
                }
            }
            ofs << "]" << endl;
        }
    }


    ////////////////// OLD //////////////////////////////////////////////


    static void dataExport(const JobshopData &DIO, const Instance &I) {
        ofstream ofs(I.path);

        if (!ofs.is_open()) {
            INTERNAL(string("Error opening output file ") + DIO.name);
        };

        ofs << DIO.numJ << " " << DIO.numM << endl;

        for (auto &J: DIO.Jobs) {
            for (auto o: J.Ops) {
                for (int m = 0; m < DIO.numM; m++) {
                    if (DIO.OMtime[o][m] > 0) {
                        ofs << m << " " << DIO.OMtime[o][m] << " ";
                    }
                }
            }
            ofs << endl;
        }
    }


    void dataExport(const vector<JobshopData> &DIOs, string dir, string instancesFile) {
        vector<Instance> Instances;

        for (auto &DIO: DIOs) {
            Instance I;
            I.name = DIO.name;
            I.jobs = DIO.numJ;
            I.machines = DIO.numM;
            I.optimum = "null";
            I.path = dir + "/" + DIO.name;

            Instances.push_back(I);

            dataExport(DIO, I);
        }
        {
            ofstream ofs(dir + "/" + instancesFile);
            if (!ofs.is_open()) {
                INTERNAL(string("Error opening output file ") + instancesFile);
            };
            json json_data = Instances;
            ofs << json_data.dump(4); // Pretty print with 4 spaces indentation
        }
    }


    // void dataExport_fjs( const vector<JobshopData>& DIOs, string dir, string fjsFileName ) {
    //
    //     int idx = 0;
    //     for ( auto &DIO: DIOs ) {
    //
    //         auto ofs = nnutils::openFileWithDirs<ofstream>(dir + "/" + fjsFileName + "_" +nnutils::to_string(idx++, 3) + ".fjs");
    //
    //         // computing average number of machines used by each operation
    //         vector<vector<pair<int,int>>> operationMachines(DIO.OMtime.size()); // list of machines eligible for each operation pair<machine, duration>
    //
    //         for ( int o = 0; o < DIO.OMtime.size(); o++ ) {
    //             for ( int m = 0; m < DIO.OMtime[o].size(); m++ ) {
    //                 int dur = DIO.OMtime[o][m];
    //                 if (dur > 0) {
    //                     operationMachines[o].push_back(make_pair(m+1, dur));
    //                 }
    //             }
    //         }
    //
    //         int sum = 0, num = 0;
    //         for ( auto &J: DIO.Jobs ) {
    //             for ( auto &o: J.Ops ) {
    //                 sum += operationMachines[o].size();
    //                 num += 1;
    //             }
    //         }
    //
    //         float avgNumMO = (float)sum / num;
    //
    //         ofs << DIO.numJ << " " << DIO.numM << " " << avgNumMO << endl;
    //
    //         for ( auto &J: DIO.Jobs ) {
    //             ofs << J.Ops.size();
    //             for ( auto &o: J.Ops ) {
    //                 ofs << " " << operationMachines[o].size();
    //                 for ( auto &MD: operationMachines[o] ) {
    //                     ofs << " " << MD.first << " " << MD.second;
    //                 }
    //             }
    //             ofs << endl;
    //         }
    //         ofs << flush;
    //
    //     }
    // }

    void dataExport_fjs(const vector<JobshopData> &DIOs, string dir, string fjsFileName) {
        int idx = 0;
        for (auto &DIO: DIOs) {
            auto ofs = nnutils::openFileWithDirs<ofstream>(
                dir + "/" + fjsFileName + (fjsFileName == "" ? "" : "_") + nnutils::to_string(idx++, 6) + ".fjs");

            // computing average number of machines used by each operation
            vector<vector<pair<int, int> > > operationMachines(DIO.OMtime.size());
            // list of machines eligible for each operation pair<machine, duration>

            for (int o = 0; o < DIO.OMtime.size(); o++) {
                for (int m = 0; m < DIO.OMtime[o].size(); m++) {
                    int dur = DIO.OMtime[o][m];
                    if (dur > 0) {
                        operationMachines[o].push_back(make_pair(m + 1, dur));
                    }
                }
            }

            int sum = 0, num = 0;
            for (auto &J: DIO.Jobs) {
                for (auto &o: J.Ops) {
                    sum += operationMachines[o].size();
                    num += 1;
                }
            }

            float avgNumMO = (float) sum / num;

            ofs << DIO.numJ << " " << DIO.numM << " " << avgNumMO << endl;

            for (auto &J: DIO.Jobs) {
                ofs << J.Ops.size();
                for (auto &o: J.Ops) {
                    ofs << " " << operationMachines[o].size();
                    for (auto &MD: operationMachines[o]) {
                        ofs << " " << MD.first << " " << MD.second;
                    }
                }
                ofs << endl;
            }

            INFO("EXPORT " << DIO.setupTimes.size() << " | " << DIO.setupTimes[0].size() << " | " << DIO.setupTimes[0][0
            ].size() << " M " << DIO.numM << " O " << DIO.numO);

            if (!DIO.setupTimes.empty() && DIO.setupTimes.size() == DIO.numM) {
                ofs << "SDST\n";
                for (int m = 0; m < DIO.numM; ++m) {
                    ofs << "Machine " << m << "\n";
                    for (int i = 0; i < DIO.numO; ++i) {
                        for (int j = 0; j < DIO.numO; ++j) {
                            ofs << DIO.setupTimes[m][i][j] << "\t";
                        }
                        ofs << "\n";
                    }
                }
            }

            ofs << flush;
        }
    }
}
