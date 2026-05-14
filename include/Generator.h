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
    //using namespace __gnu_debug;


    struct GeneratorRnd {
        struct GenConfigType {
            friend class boost::serialization::access;
            string nameBase;
            //            int num;
            int seedCommon;
            int seed;
            int numM;
            int numO;
            bool multiOperation; //< are multiple tasks in the same job permited
            pair<int, int> RangeOM; //< how many machines for single operation
            pair<int, int> RangeJ; //< how many jobs
            pair<int, int> RangeJO; //< how many operations is a single job
            pair<int, int> RangeD; //< task duration range


            // string to_string( string phase ) const {
            //     return dir + "/" + phase + "/" + nameBase; // + "_" + to_string(hash_value());
            // }

            template<class Archive>
            void serialize(Archive &ar, const unsigned int version) {
                ar & nameBase;
                // ar & num;
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


        GenConfigType GConf;


        // int num;
        // DataLoaderJobshop( int _seed, string _filename, int _numM, int _numO, bool _multiOps, pair<int, int> _RangeOM, pair<int, int> _RangeJ, pair<int, int> _RangeJO, pair<int, int> _RangeT, int _numTotal, int _numValidation  )
        // : seed(_seed), filename( _filename ), numTotal( _numTotal ), numValidation(_numValidation), numM( _numM ), numO( _numO ), multiOps(_multiOps), RangeOM(_RangeOM), RangeJ( _RangeJ ), RangeT( _RangeT ), RangeJO(_RangeJO) {
        // }

        GeneratorRnd(const GenConfigType &_Conf) : GConf(_Conf) {
        }

        // GenConfigType& GenConfig() {
        //     return GConf;
        // }

        //    vector<PalOpt::InputOutputData> IODs;
        void load(int _seed, int _num, vector<JobshopData> &IODs);

        // static string composeFileName( const GenConfig &Conf, string dir, string prefix  ) {
        //      return dir
        //         + "/"
        //         + prefix
        //         + "_" + std::to_string( Conf.hash_value() );
        //     ;
        // }
    };


    inline void to_json(nlohmann::json &j, const GeneratorRnd::GenConfigType &p) {
        j = nlohmann::json{};
        j.emplace("nameBase", p.nameBase);
        // j.emplace("num", p.num);
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
        // j.at("num").get_to(p.num);
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


    struct GeneratorTxt {
        struct GenConfigType {
            friend class boost::serialization::access;
            string txtFileName;
            // int num;
            int numM;
            int numO;
            int seed;
            bool multiTask; //< are multiple tasks in the same job permited
            pair<float, float> RangeJ; //< how many jobs, as fraction of the jobs number in txt file


            // string to_string( string phase ) const {
            //     return dir + "/" + phase + "/" + nameBase; // + "_" + to_string(hash_value());
            // }

            template<class Archive>
            void serialize(Archive &ar, const unsigned int version) {
                ar & txtFileName;
                // ar & num;
                ar & numM;
                ar & numO;
                ar & seed;
                ar & multiTask;
                ar & RangeJ;
                // ar & RangeJO;
            }
        };

        GenConfigType GConf;

        // int num;

        JobshopData CommonIOD;
        pair<int, int> RangeJO; //< how many operations is a single job


        // DataLoaderJobshop( int _seed, string _filename, int _numM, int _numO, bool _multiOps, pair<int, int> _RangeOM, pair<int, int> _RangeJ, pair<int, int> _RangeJO, pair<int, int> _RangeT, int _numTotal, int _numValidation  )
        // : seed(_seed), filename( _filename ), numTotal( _numTotal ), numValidation(_numValidation), numM( _numM ), numO( _numO ), multiOps(_multiOps), RangeOM(_RangeOM), RangeJ( _RangeJ ), RangeT( _RangeT ), RangeJO(_RangeJO) {
        // }

        GeneratorTxt(const GenConfigType &_Conf) : GConf(_Conf) {
            readTxtFile();
        }

        // GenConfigType& GenConfig() {
        //     return GConf;
        // }

        // GeneratorTxt& completeGenTxtConfig( GenConfigType &_Conf) {
        //     auto GConfTmp = GConf;
        //     auto num_tmp = num;
        //     GConf = _Conf;
        //     num = 1;
        //     vector<JobshopData> IODs;
        //     load(IODs);
        //     GConf.numM = IODs.front().numM;
        //     GConf.numO = IODs.front().numO;
        //     _Conf = GConf;
        //     GConf = GConfTmp;
        //     num = num_tmp;
        //     return *this;
        // }

        const JobshopData &getCommonDIO() {
            return CommonIOD;
        }

        void readTxtFile();

        //    vector<PalOpt::InputOutputData> IODs;
        void load(int _seed, int num, vector<JobshopData> &IODs);

        // static string composeFileName( const GenConfig &Conf, string dir, string prefix  ) {
        //      return dir
        //         + "/"
        //         + prefix
        //         + "_" + std::to_string( Conf.hash_value() );
        //     ;
        // }
    };

    inline void to_json(nlohmann::json &j, const GeneratorTxt::GenConfigType &p) {
        j = nlohmann::json{};
        j.emplace("nameBase", p.txtFileName);
        // j.emplace("num", p.num);
        j.emplace("numM", p.numM);
        j.emplace("numO", p.numO);
        j.emplace("seed", p.seed);
        j.emplace("multiTask", p.multiTask);
        j.emplace("RangeJ", p.RangeJ);
        // j.emplace("RangeJO", p.RangeJO);
    }

    inline void from_json(const nlohmann::json &j, GeneratorTxt::GenConfigType &p) {
        j.at("nameBase").get_to(p.txtFileName);
        // j.at("num").get_to(p.num);
        j.at("numM").get_to(p.numM);
        j.at("numO").get_to(p.numO);
        j.at("seed").get_to(p.seed);
        j.at("multiTask").get_to(p.multiTask);
        j.at("RangeJ").get_to(p.RangeJ);
        // j.at("RangeJO").get_to(p.RangeJO);
    }
}

