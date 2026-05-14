#pragma once

#include <vector>
#include "Err.h"
// #include "BlackBoxEvaluatorConcept.h"
#include "ConstructionHeuristicConcept.h"
#include <thread>


namespace chof {
    using namespace std;

    /**
     * The class implements the concept of  BlackBoxEvaluatorType
     *
     * @tparam ConstructionHeuristicType
     * @tparam InputOutputData
     */

    template<ConstructionHeuristicConcept CHType>
    struct ParallelDataSetEvaluator {
    private:
        vector<typename CHType::DataType *> *Datas;
        vector<CHType> CHS;
        vector<double> Sums;

    public:
        ParallelDataSetEvaluator(vector<typename CHType::DataType *> &_Datas, const CHType &_CH, int numThreads)
            : Datas(&_Datas), CHS(numThreads, _CH), Sums(numThreads, 0.0) {
            if (numThreads <= 0) INTERNAL("numThreads must be > 0");
        }

        void solve(const double *psi, const int &n, int t, int beg, int num) {
            CHS[t].setParams(psi, n);

            Sums[t] = 0.0;

            for (int i = beg; i < beg + num; i++) {
                if (i >= Datas->size()) INTERNAL("Problem with parallelization");

                auto &Data = (*Datas)[i];

                Sums[t] += CHS[t].run(*Data).getObj();
            }
        }

        double operator()(const double *psi, int n) {
            std::vector<std::thread> threads;

            int m = Datas->size(); //< number of tasks

            int tnum = CHS.size(); //< number of threads

            int tpt = m / tnum; //< tasks per thread
            int r = m % tnum; //< the reminder

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

            double objSum = accumulate(Sums.begin(), Sums.end(), 0.0);


            return objSum / Datas->size();
        }
    };
}
