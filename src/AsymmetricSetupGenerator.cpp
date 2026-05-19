#include "AsymmetricSetupGenerator.h"
#include <cmath>
#include <algorithm>

namespace jobshop {
    std::vector<std::vector<std::vector<int> > > AsymmetricSetupGenerator::generate(
        const std::vector<std::vector<int> > &OMtime,
        int numM,
        int numO,
        const SetupConfig &config,
        std::mt19937 &gen) {
        const double mu = calculateMu(OMtime);

        // Prepare distributions for short and long transitions
        int short_lower = static_cast<int>(std::round(shortLower * config.eta * mu));
        int short_upper = static_cast<int>(std::round(shortUpper * config.eta * mu));
        if (short_lower < 1) short_lower = 1;
        if (short_upper <= short_lower) short_upper = short_lower + 1;

        int long_lower = static_cast<int>(std::round(longLower * config.eta * mu));
        int long_upper = static_cast<int>(std::round(longUpper * config.eta * mu));
        if (long_lower < 1) long_lower = 1;
        if (long_upper <= long_lower) long_upper = long_lower + 1;

        std::uniform_int_distribution<int> distShort(short_lower, short_upper);
        std::uniform_int_distribution<int> distLong(long_lower, long_upper);

        // Generate base N x N matrix
        std::vector<std::vector<int> > baseMatrix(numO, std::vector<int>(numO, 0));

        for (int i = 0; i < numO; ++i) {
            // Even = standard, Odd = special
            bool is_i_special = (i % 2 != 0);

            for (int j = 0; j < numO; ++j) {
                if (i == j) {
                    baseMatrix[i][j] = 0;
                    continue;
                }

                bool is_j_special = (j % 2 != 0);

                // Special -> Standard = long setup (e.g. cleaning the machine)
                if (is_i_special && !is_j_special) {
                    baseMatrix[i][j] = distLong(gen);
                }
                // All other variants have short setup
                else {
                    baseMatrix[i][j] = distShort(gen);
                }
            }
        }

        // Enforce triangle inequality
        enforceTriangleInequality(baseMatrix, numO);

        // Copy matrix for each machine
        std::vector<std::vector<std::vector<int> > > setupTimes(numM, baseMatrix);

        return setupTimes;
    }
} // namespace jobshop
