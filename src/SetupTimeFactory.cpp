#include "SetupTimeGenerator.h"
#include "RandomSetupGenerator.h"
#include "MachineDependentSetupGenerator.h"
#include "AsymmetricSetupGenerator.h"
#include "ClusteredSetupGenerator.h"
#include "ResourceSetupGenerator.h"
#include "Err.h"

namespace jobshop {
    std::unique_ptr<SetupTimeGenerator> createSetupTimeGenerator(int variant) {
        switch (variant) {
            case 1:
                return std::make_unique<RandomSetupGenerator>();
            case 2:
                return std::make_unique<MachineDependentSetupGenerator>();
            case 3:
                return std::make_unique<AsymmetricSetupGenerator>();
            case 4:
                return std::make_unique<ClusteredSetupGenerator>();
            case 5:
                return std::make_unique<ResourceSetupGenerator>();
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

    double SetupTimeGenerator::calculateMu(const std::vector<std::vector<int> > &OMtime) {
        double sum = 0;
        int count = 0;
        for (const auto &row: OMtime) {
            for (int val: row) {
                if (val > 0) {
                    sum += val;
                    count++;
                }
            }
        }
        return (count > 0) ? (sum / count) : 1.0;
    }
} // namespace jobshop
