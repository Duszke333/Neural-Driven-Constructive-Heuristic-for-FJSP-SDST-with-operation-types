#pragma once

#include <vector>
#include "Err.h"
// #include "BlackBoxEvaluatorConcept.h"
#include "ConstructionHeuristicConcept.h"
#include <thread>


namespace chof {
    using namespace std;

    /**
     * @brief Multi-threaded Black-Box Evaluator for a dataset.
     * * Divides the provided dataset evenly across a specified number of std::threads.
     * Each thread runs its own cloned copy of the Construction Heuristic.
     * Used primarily during validation phases to speed up evaluation.
     * * @tparam CHType The Construction Heuristic type.
     */
    template<ConstructionHeuristicConcept CHType>
    struct ParallelDataSetEvaluator {
    private:
        vector<typename CHType::DataType *> *Datas; ///< Pointer to the full dataset.
        vector<CHType> CHS; ///< Dedicated copies of the Heuristic for each thread.
        vector<double> Sums; ///< Array storing the sum of objective values calculated by each thread.

    public:
        /**
         * @brief Initializes the parallel evaluator, cloning the heuristic for each required thread.
         * @param _Datas Reference to the vector of problem instance pointers.
         * @param _CH Base Construction Heuristic to clone.
         * @param numThreads Desired number of parallel threads to use.
         */
        ParallelDataSetEvaluator(vector<typename CHType::DataType *> &_Datas, const CHType &_CH, int numThreads)
            : Datas(&_Datas), CHS(numThreads, _CH), Sums(numThreads, 0.0) {
            if (numThreads <= 0) INTERNAL("numThreads must be > 0");
        }

        /**
         * @brief Worker function executed by a single thread.
         * * Evaluates a specific contiguous chunk (subset) of the dataset.
         * @param psi Pointer to the flat parameter array (weights).
         * @param n Size of the parameter array.
         * @param t Thread ID (used to access dedicated heuristic and store results).
         * @param beg Starting index of the dataset chunk.
         * @param num Number of instances to evaluate in this chunk.
         */
        void solve(const double *psi, const int &n, int t, int beg, int num) {
            // Inject params to the instance assigned to this thread
            CHS[t].setParams(psi, n);
            Sums[t] = 0.0;

            // Solve assigned subset of the problems
            for (int i = beg; i < beg + num; i++) {
                if (i >= Datas->size()) INTERNAL("Problem with parallelization");
                auto &Data = (*Datas)[i];
                Sums[t] += CHS[t].run(*Data).getObj();
            }
        }

        /**
         * @brief Evaluation operator splitting the workload and spawning threads.
         * @param psi Pointer to the flat parameter array.
         * @param n Size of the parameter array.
         * @return The average objective function value across the entire dataset.
         */
        double operator()(const double *psi, int n) {
            std::vector<std::thread> threads;
            int m = Datas->size(); // Total number of instances to solve
            int tnum = CHS.size(); // Number of threads available
            int tpt = m / tnum; // Base number of tasks per thread
            int r = m % tnum; // Remainder (how many tasks to split additionally)

            int beg = 0;
            for (int t = 0; t < tnum; t++) {
                int num = tpt;
                if (t < r) num++;

                threads.push_back(std::thread(&ParallelDataSetEvaluator<CHType>::solve, this, psi, n, t, beg, num));
                beg += num;
            }

            for (auto &t: threads) {
                t.join();
            }

            // Sum partial results from each thread and calculate final average
            double objSum = accumulate(Sums.begin(), Sums.end(), 0.0);
            return objSum / Datas->size();
        }
    };
}
