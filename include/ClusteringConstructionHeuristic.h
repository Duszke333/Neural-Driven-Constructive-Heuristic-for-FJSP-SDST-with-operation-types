#pragma once

#include <vector>
#include <debug/vector>
#include <map>
#include <algorithm>
#include <float.h>
#include <assert.h>
#include <random>
#include "DataConcept.h"
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <nlohmann/json.hpp>

#include "ConstructionHeuristicConcept.h"


namespace chof {
    // using namespace __gnu_debug;
    using namespace std;
    // using json = nlohmann::json;

    /**
     * ClusteringHeuristic implements ConstructionHeuristicConcept interface
    **/

    template<ConstructionHeuristicConcept CHType>
    class ClusteringConstructionHeuristic {
        friend class boost::serialization::access;

    public:
        struct ConfigType {
            using CHT = CHType;

            int num;
            typename CHType::ConfigType CHConf;
        };

        ConfigType Conf;
        vector<CHType> CHS;

    public:
        template<class Archive>
        void serialize(Archive &ar, const unsigned int version) {
            ar & CHS;
        }

        // ConstructionHeuristicConcept interface
        typedef CHType::DataType DataType;

        bool maximize() const {
            assert(!CHS.empty());
            return CHS.front().maximize();
        };

        ClusteringConstructionHeuristic() {
        }

        ClusteringConstructionHeuristic(const ConfigType &_Conf) : Conf(_Conf), CHS(_Conf.num, CHType(_Conf.CHConf)) {
        }


        ClusteringConstructionHeuristic(int _M, CHType &CS) : CHS(_M, CS) {
        }

        int getParamsSize() const {
            return CHS.size() * CHS.front().getParamsSize();
        }

        void getParams(vector<double> &Params) const {
            for (auto &ch: CHS) {
                ch.getParams(Params);
            }
        }


        ClusteringConstructionHeuristic &setParams(const double *params, int n) {
            int ch_size = CHS.front().getParamsSize();
            assert(n == ch_size * CHS.size());
            for (auto &ch: CHS) {
                ch.setParams(params, ch_size);
                params += ch_size;
            }
            return *this;
        }

        DataType::SolutionType run(const DataType &Data) {
            typename DataType::SolutionType Sol, SolTmp;

            Sol.setObj(CHS.front().maximize() ? -DBL_MAX : DBL_MAX);

            int c = 0;

            for (auto &CH: CHS) {
                SolTmp = CH.run(Data);

                if (CHS.front().maximize() ? SolTmp.getObj() > Sol.getObj() : SolTmp.getObj() < Sol.getObj()) {
                    swap(Sol, SolTmp);
                    // Sol = SolTmp;

                    if constexpr (ClusteredSolutionConcept<typename DataType::SolutionType>) {
                        Sol.setCluster(c);
                    }
                }

                c++;
            }

            return Sol;
        }

        // end of interface
        CHType &operator[](int c) {
            return CHS[c];
        }
    };

    // template<typename T>
    // inline void to_json(nlohmann::json& j, const T& c) {
    //     to_json<typename T::ParentType::ConfigType>(j, c);
    // }
    // template<typename CHType>
    // void to_json(nlohmann::json& j, const typename ClusteringConstructionHeuristic<CHType>::ConfigType& c) {
    //     j = nlohmann::json{};
    //     j.emplace("num", c.num);
    //     j.emplace("CHConf", c.CHConf );
    // }

    template<typename T>
    auto to_json(nlohmann::json &j, const T &c)
        -> std::enable_if_t<std::is_same_v<T, typename ClusteringConstructionHeuristic<typename T::CHT>::ConfigType> > {
        j = nlohmann::json{};
        j.emplace("num", c.num);
        j.emplace("CHConf", c.CHConf);
    }


    // template<typename T>
    // inline void from_json(const nlohmann::json &j, T &c) {
    //     from_json<typename T::ParentType::ConfigType>(j, c);
    // }
    // template<typename CHType>
    // void from_json(const nlohmann::json &j, typename ClusteringConstructionHeuristic<CHType>::ConfigType& c) {
    //     j.at("num").get_to(c.num);
    //     j.at("CHConf").get_to(c.CHConf);
    // }

    template<typename T>
    auto from_json(nlohmann::json &j, T &c)
        -> std::enable_if_t<std::is_same_v<T, typename ClusteringConstructionHeuristic<typename T::CHT>::ConfigType> > {
        j.at("num").get_to(c.num);
        j.at("CHConf").get_to(c.CHConf);
    }
}