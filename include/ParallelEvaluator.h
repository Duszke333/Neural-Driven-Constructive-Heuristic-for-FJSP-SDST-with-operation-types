
#pragma once

#include <vector>
#include <random>
#include <mutex>
#include "BlackBoxEvaluatorConcept.h"


namespace chof {
    using namespace std;

    template<BlackBoxEvaluatorConcept BBEType>
    struct ParallelEvaluator {
        vector<BBEType> Pool;
        vector<int> Stack;
        std::mutex fmtx;

        enum { MAX_THREADS = 256 };

        ParallelEvaluator() {
        }

        ParallelEvaluator(const BBEType &_OptObj)
            : Pool(MAX_THREADS, _OptObj), Stack(MAX_THREADS) {
            iota(Stack.begin(), Stack.end(), 0);
        }

        ParallelEvaluator(const ParallelEvaluator<BBEType> &other) : Pool(other.Pool), Stack(other.Stack) {
        }

        double operator()(const double *psi, const int &n) {
            int ObjIdx = -1;
            while (true) {
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

            // uruchomienie symulacji
            double ret = Pool[ObjIdx](psi, n);


            {
                lock_guard<std::mutex> lck(fmtx);
                Stack.push_back(ObjIdx);
            }
            return ret;
        }
    };
}