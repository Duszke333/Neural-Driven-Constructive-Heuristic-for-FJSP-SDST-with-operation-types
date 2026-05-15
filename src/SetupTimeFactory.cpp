#include "SetupTimeGenerator.h"
#include "RandomSetupGenerator.h"
#include "MachineDependentSetupGenerator.h"
#include "AsymmetricSetupGenerator.h"
#include "Err.h"
#include <algorithm>
#include <iostream>

namespace jobshop {
    std::unique_ptr<SetupTimeGenerator> createSetupTimeGenerator(int variant) {
        switch (variant) {
            case 1:
                return std::make_unique<RandomSetupGenerator>();
            case 2:
                return std::make_unique<MachineDependentSetupGenerator>();
            case 3:
                return std::make_unique<AsymmetricSetupGenerator>();
            default:
                return nullptr;
        }
    }

    void SetupTimeGenerator::enforceTriangleInequality(std::vector<std::vector<int> > &setupTimes, int numO) {
        // INFO("Running floyd-warshall");
        // Classic Floyd-Warshall for each machine
        for (int k = 0; k < numO; ++k) {
            for (int i = 0; i < numO; ++i) {
                for (int j = 0; j < numO; ++j) {
                    // If there is a shorter path through k, update i-j
                    if (setupTimes[i][k] + setupTimes[k][j] < setupTimes[i][j]) {
                        setupTimes[i][j] = setupTimes[i][k] + setupTimes[k][j];
                    }
                }
            }
        }
        // INFO("Ran floyd-warshall");
    }
} // namespace jobshop
