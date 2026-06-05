//
// Created by tsliwins on 02.12.24.
//

#pragma once
#include <vector>
#include <string>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/utility.hpp>
#include <nlohmann/json.hpp>
#include "JobshopData.h"
#include "utils.h"


namespace jobshop {
    using namespace std;
    using namespace nnutils;

    /**
     * @brief Synthetic problem generator for the Flexible Job Shop Problem.
     * * Generates randomized benchmark instances (including processing times and SDST matrices)
     * based on predefined distributions and parameters.
     */
    struct GeneratorRnd {
        /**
         * @brief Configuration parameters for the random data generator.
         */
        struct GenConfigType {
            friend class boost::serialization::access;

            string nameBase; ///< Base name for generated instances.
            int seedCommon; ///< Seed for shared/common factory layout generation.
            int seed; ///< Seed for specific instance variations.
            int numM; ///< Number of machines.
            int numO; ///< Number of unique operation types.
            bool multiOperation; ///< If true, allows multiple operations of the same type within a single job.
            pair<int, int> RangeOM; ///< [min, max] compatible machines per operation type.
            pair<int, int> RangeJ; ///< [min, max] total jobs in the instance.
            pair<int, int> RangeJO; ///< [min, max] operations per single job.
            pair<int, int> RangeD; ///< [min, max] duration of a single operation.

            template<class Archive>
            void serialize(Archive &ar, const unsigned int version) {
                ar & nameBase;
                ar & seedCommon;
                ar & seed;
                ar & numM;
                ar & numO;
                ar & multiOperation;
                ar & RangeOM;
                ar & RangeJ;
                ar & RangeJO;
                ar & RangeD;
            }
        };


        GenConfigType GConf; ///< Current generator configuration.

        /**
         * @brief Constructor initializing the random generator with a specific configuration.
         * @param _Conf The configuration object.
         */
        GeneratorRnd(const GenConfigType &_Conf) : GConf(_Conf) {
        }

        /**
         * @brief Generates a specified number of randomized Job Shop instances.
         * @param _seed Random seed override (if >= 0).
         * @param _num Number of instances to generate.
         * @param IODs Output vector where generated instances will be stored.
         */
        void load(int _seed, int _num, vector<JobshopData> &IODs);
    };


    // =================================
    // JSON SERIALIZATION (GeneratorRnd)
    // =================================

    inline void to_json(nlohmann::json &j, const GeneratorRnd::GenConfigType &p) {
        j = nlohmann::json{};
        j.emplace("nameBase", p.nameBase);
        j.emplace("seedCommon", p.seedCommon);
        j.emplace("seed", p.seed);
        j.emplace("numM", p.numM);
        j.emplace("numT", p.numO);
        j.emplace("multiTask", p.multiOperation);
        j.emplace("RangeTM", p.RangeOM);
        j.emplace("RangeJ", p.RangeJ);
        j.emplace("RangeJT", p.RangeJO);
        j.emplace("RangeD", p.RangeD);
    }

    inline void from_json(const nlohmann::json &j, GeneratorRnd::GenConfigType &p) {
        j.at("nameBase").get_to(p.nameBase);
        j.at("seedCommon").get_to(p.seedCommon);
        j.at("seed").get_to(p.seed);
        j.at("numM").get_to(p.numM);
        j.at("numT").get_to(p.numO);
        j.at("multiTask").get_to(p.multiOperation);
        j.at("RangeTM").get_to(p.RangeOM);
        j.at("RangeJ").get_to(p.RangeJ);
        j.at("RangeJT").get_to(p.RangeJO);
        j.at("RangeD").get_to(p.RangeD);
    }

    // ===================
    // TEXT FILE GENERATOR
    // ===================

    /**
     * @brief Problem generator based on existing text files (e.g., benchmark datasets).
     * * Loads a base structure from a file and can generate variations of it.
     */
    struct GeneratorTxt {
        /**
         * @brief Configuration parameters for the text-based data generator.
         */
        struct GenConfigType {
            friend class boost::serialization::access;

            string txtFileName; ///< Path or base name of the source text file.
            int numM; ///< Number of machines.
            int numO; ///< Number of operation types.
            int seed; ///< Seed for instance variations.
            bool multiTask; ///< Are multiple tasks in the same job permitted.
            pair<float, float> RangeJ; ///< [min, max] jobs, as a fraction of the jobs number in the txt file.

            template<class Archive>
            void serialize(Archive &ar, const unsigned int version) {
                ar & txtFileName;
                ar & numM;
                ar & numO;
                ar & seed;
                ar & multiTask;
                ar & RangeJ;
            }
        };

        GenConfigType GConf; ///< Current generator configuration.
        JobshopData CommonIOD; ///< Common base structure parsed from the text file.
        pair<int, int> RangeJO; ///< [min, max] how many operations in a single job.

        /**
         * @brief Constructor initializing the generator and immediately parsing the text file.
         * @param _Conf The configuration object.
         */
        GeneratorTxt(const GenConfigType &_Conf) : GConf(_Conf) {
            readTxtFile();
        }

        /**
         * @brief Retrieves the shared base instance configuration loaded from the file.
         * @return A reference to the parsed CommonIOD structure.
         */
        const JobshopData &getCommonDIO() {
            return CommonIOD;
        }

        /**
         * @brief Parses the underlying text file and populates the CommonIOD structure.
         */
        void readTxtFile();

        /**
         * @brief Generates a specified number of Job Shop instances based on the loaded text file.
         * @param _seed Random seed override (if >= 0).
         * @param num Number of instances to generate.
         * @param IODs Output vector where generated instances will be stored.
         */
        void load(int _seed, int num, vector<JobshopData> &IODs);
    };

    // =================================
    // JSON SERIALIZATION (GeneratorTxt)
    // =================================

    inline void to_json(nlohmann::json &j, const GeneratorTxt::GenConfigType &p) {
        j = nlohmann::json{};
        j.emplace("nameBase", p.txtFileName);
        j.emplace("numM", p.numM);
        j.emplace("numO", p.numO);
        j.emplace("seed", p.seed);
        j.emplace("multiTask", p.multiTask);
        j.emplace("RangeJ", p.RangeJ);
    }

    inline void from_json(const nlohmann::json &j, GeneratorTxt::GenConfigType &p) {
        j.at("nameBase").get_to(p.txtFileName);
        j.at("numM").get_to(p.numM);
        j.at("numO").get_to(p.numO);
        j.at("seed").get_to(p.seed);
        j.at("multiTask").get_to(p.multiTask);
        j.at("RangeJ").get_to(p.RangeJ);
    }
}
