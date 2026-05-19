#include "RandomSetupGenerator.h"
#include <cmath>
#include <algorithm>

namespace jobshop {
    std::vector<std::vector<std::vector<int> > > RandomSetupGenerator::generate(
        const std::vector<std::vector<int> > &OMtime,
        int numM,
        int numO,
        const SetupConfig &config,
        std::mt19937 &gen) {
        const double mu = calculateMu(OMtime);

        // Prepare random range U(a * eta * mu, b * eta * mu)
        int lower = static_cast<int>(std::round(config.a * config.eta * mu));
        int upper = static_cast<int>(std::round(config.b * config.eta * mu));
        std::uniform_int_distribution<int> dist(lower, upper);

        // Generate base matrix N x N for operations
        std::vector<std::vector<int> > baseMatrix(numO, std::vector<int>(numO, 0));
        for (int i = 0; i < numO; ++i) {
            for (int j = 0; j < numO; ++j) {
                if (i == j) {
                    baseMatrix[i][j] = 0;
                } else {
                    baseMatrix[i][j] = dist(gen);
                }
            }
        }

        // Enforce triangle inequality
        enforceTriangleInequality(baseMatrix, numO);

        // Clone matrix for every machine to get 3D result
        std::vector<std::vector<std::vector<int> > > setupTimes(numM, baseMatrix);

        return setupTimes;
    }
} // namespace jobshop
