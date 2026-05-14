#pragma once
#include <map>
#include <vector>
#include <string>
#include "options.h"

namespace jobshop {
    using namespace std;

    typedef map<vector<pair<int, int> >, int> TOperationsTypesMap;

    void train(Config Cfg);

    void test(Config Cfg);

    void generateRandom(Config Cfg);

    void generateBrandimarte(Config Cfg);
}