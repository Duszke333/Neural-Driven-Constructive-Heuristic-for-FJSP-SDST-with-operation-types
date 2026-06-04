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
    using namespace std;

    /**
     * @brief An ensemble heuristic that manages a cluster of multiple base heuristics.
     * * This class evaluates the problem instance using multiple independent Construction
     * Heuristics (e.g., networks with slightly different weights or structures) and
     * returns the best generated solution among them.
     * * @tparam CHType The underlying Construction Heuristic type.
     */
    template<ConstructionHeuristicConcept CHType>
    class ClusteringConstructionHeuristic {
        friend class boost::serialization::access;

    public:
        /**
         * @brief Configuration for the clustered ensemble.
         */
        struct ConfigType {
            using CHT = CHType;

            int num; ///< Number of heuristic instances in the cluster.
            typename CHType::ConfigType CHConf; ///< Shared configuration for all child heuristics.
        };

        ConfigType Conf; ///< General configuration for the cluster.
        vector<CHType> CHS; ///< Vector storing the individual heuristic instances.

        template<class Archive>
        void serialize(Archive &ar, const unsigned int version) {
            ar & CHS;
        }

        ClusteringConstructionHeuristic() {
        }

        /**
         * @brief Constructs the ensemble based on the provided configuration.
         * @param _Conf The configuration specifying the number of heuristics and their setup.
         */
        ClusteringConstructionHeuristic(const ConfigType &_Conf) : Conf(_Conf), CHS(_Conf.num, CHType(_Conf.CHConf)) {
        }

        /**
         * @brief Constructs the ensemble by cloning a provided heuristic.
         * @param _M Number of clones to create.
         * @param CS The base heuristic to copy.
         */
        ClusteringConstructionHeuristic(int _M, CHType &CS) : CHS(_M, CS) {
        }

        // ConstructionHeuristicConcept interface
        typedef CHType::DataType DataType;

        /**
         * @brief Returns the optimization direction (delegated to the first child heuristic).
         */
        bool maximize() const {
            assert(!CHS.empty());
            return CHS.front().maximize();
        }

        /**
         * @brief Calculates the total number of tunable parameters across the entire ensemble.
         * @return Size of a single heuristic's parameters multiplied by the number of heuristics in the cluster.
         */
        int getParamsSize() const {
            return CHS.size() * CHS.front().getParamsSize();
        }

        /**
         * @brief Extracts parameters from all child heuristics into a single flat vector.
         * @param Params Vector where the parameters will be appended.
         */
        void getParams(vector<double> &Params) const {
            for (auto &ch: CHS) {
                ch.getParams(Params);
            }
        }

        /**
         * @brief Distributes a flat array of parameters evenly across all child heuristics.
         * @param params Pointer to the array of double parameters.
         * @param n Total size of the array.
         */
        ClusteringConstructionHeuristic &setParams(const double *params, int n) {
            int ch_size = CHS.front().getParamsSize();
            assert(n == ch_size * CHS.size());
            for (auto &ch: CHS) {
                ch.setParams(params, ch_size);
                params += ch_size;
            }
            return *this;
        }

        /**
         * @brief Core ensemble logic: Runs all child heuristics and returns the best solution.
         * @param Data The problem instance to be solved.
         * @return The best generated solution.
         */
        DataType::SolutionType run(const DataType &Data) {
            typename DataType::SolutionType Sol, SolTmp;

            // Initialize with the worst possible objective value
            Sol.setObj(CHS.front().maximize() ? -DBL_MAX : DBL_MAX);

            int c = 0;

            for (auto &CH: CHS) {
                SolTmp = CH.run(Data);

                // Check if the current heuristic produced a strictly better result
                if (CHS.front().maximize() ? SolTmp.getObj() > Sol.getObj() : SolTmp.getObj() < Sol.getObj()) {
                    swap(Sol, SolTmp);

                    // If the solution supports clustering, tag it with the winning heuristic's ID
                    if constexpr (ClusteredSolutionConcept<typename DataType::SolutionType>) {
                        Sol.setCluster(c);
                    }
                }

                c++;
            }

            return Sol;
        }

        // end of interface

        /**
         * @brief Access a specific heuristic in the cluster.
         * @param c Index of the heuristic.
         */
        CHType &operator[](int c) {
            return CHS[c];
        }
    };

    // ==================
    // JSON SERIALIZATION
    // ==================

    template<typename T>
    auto to_json(nlohmann::json &j, const T &c)
        -> std::enable_if_t<std::is_same_v<T, typename ClusteringConstructionHeuristic<typename T::CHT>::ConfigType> > {
        j = nlohmann::json{};
        j.emplace("num", c.num);
        j.emplace("CHConf", c.CHConf);
    }

    template<typename T>
    auto from_json(nlohmann::json &j, T &c)
        -> std::enable_if_t<std::is_same_v<T, typename ClusteringConstructionHeuristic<typename T::CHT>::ConfigType> > {
        j.at("num").get_to(c.num);
        j.at("CHConf").get_to(c.CHConf);
    }
}
