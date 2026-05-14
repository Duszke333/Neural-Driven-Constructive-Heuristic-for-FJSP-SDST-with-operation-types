#pragma once
#include <iomanip>
#include <iostream>
#include <vector>

#include "BlackBoxEvaluatorConcept.h"
#include "ConstructionHeuristicConcept.h"

namespace chof {
    using namespace std;

    /**
     * The class implements the concept of  BlackBoxEvaluatorConcept.
     * It evaluates the set of data.
     *
     *
     */

    template<ConstructionHeuristicConcept CHType>
    struct DataSetEvaluator {
    private:
        vector<typename CHType::DataType *> *Datas;
        CHType CH;

    public:
        DataSetEvaluator(vector<typename CHType::DataType *> &_Datas,
                         const CHType &_CH)
            : Datas(&_Datas), CH(_CH) {
        }

        double operator()(const double *params, const int &n) {
            // for ( int i = 0; i < 10; i++ ) {
            //     cout << setprecision(2) << params[i] << " " ;
            // }
            // cout << endl << flush;
            //
            // exit(0);

            CH.setParams(params, n);
            double objSum = 0.0;

            for (auto &Data: *Datas) {
                objSum += CH.run(*Data).getObj();
            }

            return objSum / Datas->size();
        }
    };
}

