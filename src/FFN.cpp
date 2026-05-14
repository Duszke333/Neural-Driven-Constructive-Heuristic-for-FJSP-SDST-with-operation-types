#include "FFN.h"
#include "utils.h"

// use typedefs for future ease for changing data types like : float to double


//typedef Eigen::MatrixXf Matrix;
//typedef Eigen::RowVectorXf RowVector;
//typedef Eigen::VectorXf ColVector;


// neural network implementation class!
namespace nnutils {
    using namespace Eigen;


    float FFN::operator()(const Matrix<float, Dynamic, 1> &Input) {
        assert(Input.size() == getInputsSize());

        assert(Conf.Topology.size() <= 3);

        int ptr = 0;

        Matrix<float, Dynamic, 1> RetMap(Conf.Topology.back()); //Ret.data(), Conf.FCNTopology.back(), 1);

        switch (Conf.Topology.size()) {
            case 1:
                RetMap.noalias() = Weights[0] * Input + Biases[0];
                break;
            case 2:
                RetMap.noalias() = Weights[1] * ((Weights[0] * Input + Biases[0]).unaryExpr(std::ref(scaleTanh3))) +
                                   Biases[1];
                break;
            case 3:
                RetMap.noalias() =
                        Weights[2] * ((Weights[1] * ((Weights[0] * Input + Biases[0]).unaryExpr(std::ref(scaleTanh3))) +
                                       Biases[1]).unaryExpr(std::ref(scaleTanh3))) + Biases[2];
                break;
        }
        return RetMap(0);
    }
}
