#pragma once

#include <vector>
#include <random>
#include <mutex>
#include "BlackBoxEvaluatorConcept.h"


namespace chof {
    using namespace std;

    /**
     * @brief Thread-safe pool wrapper for Black-Box Evaluators.
     * * Used directly by the libcmaes engine when internal multi-threading (mt_feval) is enabled.
     * It maintains a stack of available (idle) evaluators up to MAX_THREADS.
     * Uses std::mutex to safely assign an idle evaluator to the requesting CMA-ES thread
     * and returns it to the stack when the evaluation finishes.
     * * @tparam BBEType The underlying Black-Box Evaluator type.
     */
    template<BlackBoxEvaluatorConcept BBEType>
    struct ParallelEvaluator {
        vector<BBEType> Pool; ///< Pool of independent evaluator instances.
        vector<int> Stack; ///< Stack keeping track of indices of currently available (idle) evaluators in the Pool.
        std::mutex fmtx; ///< Mutex to enforce mutually exclusive access when popping from or pushing to the Stack.

        enum { MAX_THREADS = 256 }; ///< Hard limit on maximum supported concurrent threads.

        ParallelEvaluator() {
        }

        /**
         * @brief Constructor that fills the pool and prepares the availability stack.
         * @param _OptObj Base evaluator object to be copied into the pool.
         */
        ParallelEvaluator(const BBEType &_OptObj)
            : Pool(MAX_THREADS, _OptObj), Stack(MAX_THREADS) {
            iota(Stack.begin(), Stack.end(), 0);
        }

        ParallelEvaluator(const ParallelEvaluator<BBEType> &other) : Pool(other.Pool), Stack(other.Stack) {
        }

        /**
         * @brief Thread-safe evaluation request handler called by internal CMA-ES threads.
         * @param psi Pointer to the flat parameter array (weights).
         * @param n Size of the parameter array.
         * @return The objective function value.
         */
        double operator()(const double *psi, const int &n) {
            int ObjIdx = -1;

            // Get free evaluator from the stack
            while (true) {
                // Wait passively until an evaluator becomes available
                while (Stack.empty()) {
                }

                lock_guard<std::mutex> lck(fmtx);
                if (Stack.empty()) {
                    continue;
                }
                ObjIdx = Stack.back();
                Stack.pop_back();
                break;
            }

            // Run the simulation using the reserved evaluator
            double ret = Pool[ObjIdx](psi, n);


            // Return the evaluator to the pool
            {
                lock_guard<std::mutex> lck(fmtx);
                Stack.push_back(ObjIdx);
            }
            return ret;
        }
    };
}
