#pragma once
#include <map>
#include <vector>
#include <string>
#include "options.h"

namespace jobshop {
    using namespace std;

    /**
     * @brief Mapping between complex operation signatures and their numerical IDs.
     */
    typedef map<vector<pair<int, int> >, int> TOperationsTypesMap;

    /**
     * @brief Main pipeline entry point for training the Neural Network (CMA-ES).
     * @param Cfg Parsed command-line configuration.
     */
    void train(Config Cfg);

    /**
     * @brief Main pipeline entry point for testing/evaluating a trained model.
     * @param Cfg Parsed command-line configuration.
     */
    void test(Config Cfg);

    /**
     * @brief Main pipeline entry point for generating synthetic random datasets.
     * @param Cfg Parsed command-line configuration.
     */
    void generateRandom(Config Cfg);

    /**
     * @brief Main pipeline entry point for generating Brandimarte benchmark datasets.
     * @param Cfg Parsed command-line configuration.
     */
    void generateBrandimarte(Config Cfg);
}
