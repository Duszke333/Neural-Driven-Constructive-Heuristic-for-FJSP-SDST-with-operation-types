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

// #include "eigen_boost_serialization.hpp"

// use typedefs for future ease for changing data types like : float to double


//typedef Eigen::MatrixXf Matrix;
//typedef Eigen::RowVectorXf RowVector;
//typedef Eigen::VectorXf ColVector;


// neural network implementation class!
namespace nnutils {
    using namespace Eigen;
    using namespace std;
    // using json = nlohmann::json;

    class FFN {
        friend class boost::serialization::access;

    public:
        struct ConfigType {
            friend class boost::serialization::access;
            int numInputs;
            vector<int> Topology;

            template<class Archive>
            void serialize(Archive &ar, const unsigned int version) {
                ar & numInputs;
                ar & Topology;
            }
        };

    public:
        ConfigType Conf;
        vector<Matrix<float, Dynamic, Dynamic, RowMajor> > Weights;
        vector<Matrix<float, Dynamic, 1> > Biases;

    public:
        template<class Archive>
        void serialize(Archive &ar, const unsigned int version) {
            ar & Conf;
            ar & Weights;
            ar & Biases;
        }

        FFN() {
        }

        FFN(const ConfigType &_Conf) : Conf(_Conf) {
            init();
        }

        void init() {
            Weights.resize(Conf.Topology.size());
            Biases.resize(Conf.Topology.size());

            int popSize = Conf.numInputs;

            for (int l = 0; l < Conf.Topology.size(); ++l) {
                Weights[l].resize(Conf.Topology[l], popSize);
                Biases[l].resize(Conf.Topology[l], 1);
                popSize = Conf.Topology[l];
            }
        }


        float operator()(const Matrix<float, Dynamic, 1> &Input);

        float operator()(const float *data, int n) {
            Map<Matrix<float, Dynamic, 1> > In(const_cast<float *>(data), n, 1);
            return (*this)(In);
        }

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

        int getInputsSize() const {
            assert(Conf.Topology.size() >= 1);
            return Conf.numInputs;
        }

        void getParams(vector<double> &Params) const {
            // FCN
            for (auto &W: Weights) {
                Params.insert(Params.end(), W.data(), W.data() + W.size());
            }
            for (auto &B: Biases) {
                Params.insert(Params.end(), B.data(), B.data() + B.size());
            }
        }

        void setParams(const double *params, int size) {
            assert(getParamsSize() == size);

            // FCN
            for (auto &W: Weights) {
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

    protected:
    };


    inline void to_json(nlohmann::json &j, const FFN::ConfigType &p) {
        j = nlohmann::json{};
        j.emplace("numInputs", p.numInputs);
        j.emplace("Topology", p.Topology);
    }

    inline void from_json(const nlohmann::json &j, FFN::ConfigType &p) {
        j.at("numInputs").get_to(p.numInputs);
        j.at("Topology").get_to(p.Topology);
    }


    // static_assert(binpack::AssessmentFunctionConcept<EigenNetwork>);
}
