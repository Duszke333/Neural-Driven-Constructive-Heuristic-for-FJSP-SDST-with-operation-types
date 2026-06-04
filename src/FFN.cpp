#include "FFN.h"
#include "utils.h"

namespace nnutils {
    using namespace Eigen;


    float FFN::operator()(const Matrix<float, Dynamic, 1> &Input) {
        // Ensure the input vector size matches the configured network input layer
        assert(Input.size() == getInputsSize());

        // This implementation is hardcoded for speed and supports up to 3 layers
        assert(Conf.Topology.size() <= 3);

        int ptr = 0;

        // Preallocate the return map representing the final output layer.
        // For the heuristic this is typically a 1x1 matrix (a single evaluation score).
        Matrix<float, Dynamic, 1> RetMap(Conf.Topology.back());

        // Perform the feed-forward pass.
        // .noalias() is an Eigen optimization that prevents creating temporary matrices during evaluation.
        // scaleTanh3 is applied ONLY to hidden layers. The output layer is purely linear (W * X + B) as we want a raw regression score.
        switch (Conf.Topology.size()) {
            case 1:
                // No hidden layers. Direct linear projection.
                // Out = W0 * Input + B0
                RetMap.noalias() = Weights[0] * Input + Biases[0];
                break;
            case 2:
                // 1 Hidden layer + 1 Output layer.
                // Hidden = Tanh(W0 * Input + B0)
                // Out = W1 * Hidden + B1
                RetMap.noalias() = Weights[1] * ((Weights[0] * Input + Biases[0]).unaryExpr(std::ref(scaleTanh3))) +
                                   Biases[1];
                break;
            case 3:
                // 2 Hidden layers + 1 Output layer.
                // Hidden1 = Tanh(W0 * Input + B0)
                // Hidden2 = Tanh(W1 * Hidden1 + B1)
                // Out = W2 * Hidden2 + B2
                RetMap.noalias() =
                        Weights[2] * ((Weights[1] * ((Weights[0] * Input + Biases[0]).unaryExpr(std::ref(scaleTanh3))) +
                                       Biases[1]).unaryExpr(std::ref(scaleTanh3))) + Biases[2];
                break;
        }
        // Return the single scalar evaluation score
        return RetMap(0);
    }
}
