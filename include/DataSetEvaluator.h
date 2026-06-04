#pragma once
#include <iomanip>
#include <iostream>
#include <vector>

#include "BlackBoxEvaluatorConcept.h"
#include "ConstructionHeuristicConcept.h"

namespace chof {
    using namespace std;

    /**
     * @brief Single-threaded Black-Box Evaluator for a dataset.
     * * Implements the BlackBoxEvaluatorConcept. It translates the flat double array
     * provided by the CMA-ES optimizer into neural network parameters, runs the
     * Construction Heuristic over the entire provided dataset sequentially,
     * and returns the average objective value (e.g., average makespan).
     * * @tparam CHType The Construction Heuristic type.
     */
    template<ConstructionHeuristicConcept CHType>
    struct DataSetEvaluator {
    private:
        vector<typename CHType::DataType *> *Datas; ///< Pointer to the dataset (vector of problem instances).

        /**
         * @brief A local instance of the Construction Heuristic.
         * * This instance will be parameterized with weights received from the optimizer.
         */
        CHType CH;

    public:
        /**
         * @brief Constructor initializing the evaluator.
         * @param _Datas Reference to the vector of problem instance pointers.
         * @param _CH Base Construction Heuristic to be copied into the evaluator.
         */
        DataSetEvaluator(vector<typename CHType::DataType *> &_Datas,
                         const CHType &_CH)
            : Datas(&_Datas), CH(_CH) {
        }

        /**
         * @brief Evaluation operator called directly by the optimization algorithm (e.g. CMA-ES).
         * @param params Pointer to a flat array of double-precision parameters (weights).
         * @param n Size of the parameter array.
         * @return The average objective function value across all problem instances in the dataset.
         */
        double operator()(const double *params, const int &n) {
            // Inject weights from the optimizer to local Neural Network
            CH.setParams(params, n);
            double objSum = 0.0;

            // Sequentially solve each problem in the dataset
            for (auto &Data: *Datas) {
                objSum += CH.run(*Data).getObj();
            }

            // Return averaged result to the optimizer
            return objSum / Datas->size();
        }
    };
}
