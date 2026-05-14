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
        // Calculate global mu from OMtime (> 0)
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
        double mu = (count > 0) ? (sum / count) : 1.0;

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
