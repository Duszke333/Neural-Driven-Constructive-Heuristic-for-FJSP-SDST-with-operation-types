#include "ClusteredSetupGenerator.h"
#include <cmath>
#include <algorithm>

#include "Err.h"

namespace jobshop {

    std::vector<std::vector<std::vector<int> > > ClusteredSetupGenerator::generate(
        const std::vector<std::vector<int> > &OMtime,
        int numM,
        int numO,
        const SetupConfig &config,
        std::mt19937 &gen) {
        const double mu = calculateMu(OMtime);

        // Prepare distributions for short and long setups
        int shortLowerr = static_cast<int>(std::round(shortLower * config.eta * mu));
        int shortUpperr = static_cast<int>(std::round(shortUpper * config.eta * mu));
        if (shortLowerr < 1) shortLowerr = 1;
        if (shortUpperr <= shortLowerr) shortUpperr = shortLowerr + 1;

        int longLowerr = static_cast<int>(std::round(longLower * config.eta * mu));
        int longUpperr = static_cast<int>(std::round(longUpper * config.eta * mu));
        if (longLowerr < 1) longLowerr = 1;
        if (longUpperr <= longLowerr) longUpperr = longLowerr + 1;

        std::uniform_int_distribution<int> distShort(shortLowerr, shortUpperr);
        std::uniform_int_distribution<int> distLong(longLowerr, longUpperr);

        // Assign each operation type randomly to one of the clusters (cluster sizes are balanced)
        std::vector<int> operationCluster(numO);
        for (int i = 0; i < numO; ++i) {
            operationCluster[i] = i % numClusters;
        }
        std::ranges::shuffle(operationCluster, gen);

        // Generate base N x N matrix
        std::vector<std::vector<int> > baseMatrix(numO, std::vector<int>(numO, 0));
        for (int i = 0; i < numO; ++i) {
            for (int j = 0; j < numO; ++j) {
                if (i == j) {
                    baseMatrix[i][j] = 0;
                } else if (operationCluster[i] == operationCluster[j]) {
                    // Same cluster = short setup
                    baseMatrix[i][j] = distShort(gen);
                } else {
                    // Different clusters = long setup
                    baseMatrix[i][j] = distLong(gen);
                }
            }
        }

        // Enforce triangle inequality
        enforceTriangleInequality(baseMatrix, numO);

        // Clone base matrix
        std::vector<std::vector<std::vector<int> > > setupTimes(numM, baseMatrix);

        return setupTimes;
    }

} // namespace jobshop