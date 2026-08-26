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

    /**
     * @brief Helper structure for basic JSON metadata export (legacy format).
     */
    struct Instance {
        string name; ///< Name of the instance.
        int jobs; ///< Number of jobs.
        int machines; ///< Number of machines.
        string optimum; ///< Known optimum makespan (or "null" if unknown).
        string path; ///< Path to the instance file.
    };

    /**
     * @brief Serializes the basic Instance structure to JSON.
     */
    inline void to_json(nlohmann::json &j, const Instance &I) {
        j = nlohmann::json{};
        j.emplace("name", I.name);
        j.emplace("jobs", I.jobs);
        j.emplace("machines", I.machines);
        j.emplace("optimum", I.optimum);
        j.emplace("path", I.path);
    }

    /**
     * @brief Deserializes JSON into the basic Instance structure.
     */
    inline void from_json(const nlohmann::json &j, Instance &I) {
        j.at("name").get_to(I.name);
        j.at("jobs").get_to(I.jobs);
        j.at("machines").get_to(I.machines);
        j.at("optimum").get_to(I.optimum);
        j.at("path").get_to(I.path);
    }

    /**
     * @brief Advanced helper structure for full JSON factory export (including detailed operations).
     */
    struct Instance2 {
        string name; ///< Name of the instance.
        int numJ; ///< Total number of jobs.
        int numM; ///< Total number of machines.
        int numO; ///< Total number of operation types.

        /** * @brief Multi-dimensional array storing job operations.
         * Structure: Jobs[job_index][operation_index][alternative_machine_index] -> pair<duration, machine_id>
         */
        vector<vector<vector<pair<int, int> > > > Jobs;
    };

    /**
     * @brief Deserializes JSON into the advanced Instance2 structure.
     */
    inline void from_json(const nlohmann::json &j, Instance2 &I) {
        j.at("name").get_to(I.name);
        j.at("numJ").get_to(I.numJ);
        j.at("numM").get_to(I.numM);
        j.at("numO").get_to(I.numO);
        j.at("Jobs").get_to(I.Jobs);
    }

    /**
     * @brief Translates internal JobshopData into the serializable Instance2 structure.
     * @param DIO The source JobshopData instance.
     * @return Formatted Instance2 object ready for JSON export.
     */
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


    /**
     * @brief Custom function to print Instance2 as JSON with structured line breaks.
     * * Ensures the resulting JSON file is human-readable by formatting nested arrays properly.
     * @param ofs Output file stream.
     * @param I Instance2 object to print.
     * @param indent Current indentation level.
     */
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

    /**
     * @brief Exports a vector of datasets to the advanced JSON format.
     * @param DIOs Vector of problem instances to export.
     * @param dir Destination directory path.
     * @param instancesFile Name of the output JSON file.
     */
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

    // =======================
    // LEGACY EXPORT FUNCTIONS
    // =======================

    /**
     * @brief Exports a single JobshopData instance to a legacy text format.
     * @param DIO Problem instance to export.
     * @param I Instance metadata defining the output path.
     */
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

    /**
     * @brief Exports a vector of datasets and generates a metadata JSON file (legacy format).
     * @param DIOs Vector of problem instances to export.
     * @param dir Destination directory path.
     * @param instancesFile Name of the output JSON metadata file.
     */
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

    // ========================================
    // FLEXIBLE JOB SHOP (FJS) BENCHMARK EXPORT
    // ========================================

    /**
     * @brief Exports the dataset to the standard Flexible Job Shop (FJS) benchmark format.
     * * Also includes Sequence-Dependent Setup Times (SDST) at the end of the file if present.
     * @param DIOs Vector of problem instances to export.
     * @param dir Destination directory path.
     * @param fjsFileName Base name for the exported .fjs files.
     */
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
