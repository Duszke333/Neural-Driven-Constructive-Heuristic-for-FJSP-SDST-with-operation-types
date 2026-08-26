#pragma once

#include <iostream>
#include <vector>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/split_member.hpp>
#include <Eigen/Dense>
#include <nlohmann/json.hpp>
#include "eigen_boost_serialization.hpp"

namespace nnutils {
    using namespace Eigen;
    using namespace std;

    /**
     * @brief A lightweight Feed-Forward Neural Network (FFN) implemented using Eigen.
     * * This class serves as the core decision-making engine (Assessment Function)
     * for the construction heuristics. It supports up to 3 layers with customizable
     * topologies and is highly optimized for fast inference during the CMA-ES evaluation loop.
     */
    class FFN {
        friend class boost::serialization::access;

    public:
        /**
         * @brief Configuration structure for the FFN topology.
         */
        struct ConfigType {
            friend class boost::serialization::access;

            int numInputs; ///< Exact number of input features (input layer size).
            vector<int> Topology; ///< Number of neurons in each subsequent layer (hidden + output).

            template<class Archive>
            void serialize(Archive &ar, const unsigned int version) {
                ar & numInputs;
                ar & Topology;
            }
        };

        ConfigType Conf; ///< Network topology configuration.
        vector<Matrix<float, Dynamic, Dynamic, RowMajor> > Weights; ///< Weight matrices for each layer.
        vector<Matrix<float, Dynamic, 1> > Biases; ///< Bias vectors for each layer.

        FFN() {
        }

        FFN(const ConfigType &_Conf) : Conf(_Conf) {
            init();
        }

        /**
         * @brief Allocates memory for weight matrices and bias vectors based on the topology.
         * * Dimensions are calculated as: Weights = [LayerSize x PreviousLayerSize], Biases = [LayerSize x 1].
         */
        void init() {
            Weights.resize(Conf.Topology.size());
            Biases.resize(Conf.Topology.size());

            int popSize = Conf.numInputs;

            for (int l = 0; l < Conf.Topology.size(); ++l) {
                Weights[l].resize(Conf.Topology[l], popSize);
                Weights[l].setZero(); // Avoid reading uninitialized memory before setParams() is called
                Biases[l].resize(Conf.Topology[l], 1);
                Biases[l].setZero();
                popSize = Conf.Topology[l];
            }
        }

        template<class Archive>
        void serialize(Archive &ar, const unsigned int version) {
            ar & Conf;
            ar & Weights;
            ar & Biases;
        }

        // ===================================
        // CORE INFERENCE LOGIC (FORWARD PASS)
        // ===================================

        /**
         * @brief Performs a forward pass through the network using Eigen matrix operations.
         * @param Input Column vector containing extracted state features.
         * @return The final activation value (evaluation score) of the given state.
         */
        float operator()(const Matrix<float, Dynamic, 1> &Input);

        /**
         * @brief Overload for passing raw C-arrays to the network.
         * @param data Pointer to the flat array of input features.
         * @param n Size of the input array.
         * @return The evaluation score.
         */
        float operator()(const float *data, int n) {
            // Map raw memory directly to Eigen Vector without copying
            Map<Matrix<float, Dynamic, 1> > In(const_cast<float *>(data), n, 1);
            return (*this)(In);
        }

        // ==============================================
        // CMA-ES OPTIMIZER INTERFACE (PARAMETER MAPPING)
        // ==============================================

        /**
         * @brief Calculates the total number of tunable parameters (weights + biases).
         * @return Total parameter count.
         */
        int getParamsSize() const {
            int s = 0;

            // Fully connected network
            for (auto &W: Weights) {
                s += W.size();
            }
            for (auto &B: Biases) {
                s += B.size();
            }

            return s;
        }

        /**
         * @brief Returns the number of Neural Network input parameters.
         * @return The number of Neural Network input parameters.
         */
        int getInputsSize() const {
            assert(Conf.Topology.size() >= 1);
            return Conf.numInputs;
        }

        /**
         * @brief Extracts internal float matrices into a flat 1D double vector for the optimizer.
         * @param Params Vector where the parameters will be appended.
         */
        void getParams(vector<double> &Params) const {
            // FCN
            for (auto &W: Weights) {
                Params.insert(Params.end(), W.data(), W.data() + W.size());
            }
            for (auto &B: Biases) {
                Params.insert(Params.end(), B.data(), B.data() + B.size());
            }
        }

        /**
         * @brief Injects a flat 1D double array from the optimizer back into the internal float matrices.
         * @param params Pointer to the array of double parameters.
         * @param size Size of the array (must match getParamsSize()).
         */
        void setParams(const double *params, int size) {
            assert(getParamsSize() == size);

            // FCN
            for (auto &W: Weights) {
                // Map the double array segment to a matrix shape, then cast to float
                Map<const Matrix<double, Dynamic, Dynamic, RowMajor>> MMM(params, W.rows(), W.cols());
                W = MMM.cast<float>();
                params += W.size();
            }
            for (auto &B: Biases) {
                Map<const Matrix<double, Dynamic, 1, ColMajor>> MMM(params, B.rows(), B.cols());
                B = MMM.cast<float>();
                params += B.size();
            }
        }
    };


    // ==================
    // JSON SERIALIZATION
    // ==================

    inline void to_json(nlohmann::json &j, const FFN::ConfigType &p) {
        j = nlohmann::json{};
        j.emplace("numInputs", p.numInputs);
        j.emplace("Topology", p.Topology);
    }

    inline void from_json(const nlohmann::json &j, FFN::ConfigType &p) {
        j.at("numInputs").get_to(p.numInputs);
        j.at("Topology").get_to(p.Topology);
    }
}
