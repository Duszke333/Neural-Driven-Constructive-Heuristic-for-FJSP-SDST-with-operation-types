#include "ResourceSetupGenerator.h"
#include <cmath>
#include <algorithm>

namespace jobshop {

    std::vector<std::vector<std::vector<int> > > ResourceSetupGenerator::generate(
        const std::vector<std::vector<int> > &OMtime,
        int numM,
        int numO,
        const SetupConfig &config,
        std::mt19937 &gen) {
        const double mu = calculateMu(OMtime);

        // Calculate cost of swapping one tool (resource)
        int singleSwapTime = static_cast<int>(std::round(swapTimeMultiplier * config.eta * mu));
        if (singleSwapTime < 1) singleSwapTime = 1;

        // Generate binary vectors (required resources) for each operation type
        std::vector<std::vector<bool> > resourceVectors(numO, std::vector<bool>(numResources, false));
        std::bernoulli_distribution resDist(resourceProbability);

        for (int i = 0; i < numO; ++i) {
            for (int r = 0; r < numResources; ++r) {
                resourceVectors[i][r] = resDist(gen);
            }
        }

        // Generate base N x N matrix
        std::vector<std::vector<int> > baseMatrix(numO, std::vector<int>(numO, 0));
        for (int i = 0; i < numO; ++i) {
            for (int j = 0; j < numO; ++j) {
                if (i == j) {
                    baseMatrix[i][j] = 0;
                } else {
                    // Calculate Hamming distance
                    int hammingDistance = 0;
                    for (int r = 0; r < numResources; ++r) {
                        if (resourceVectors[i][r] != resourceVectors[j][r]) {
                            hammingDistance++;
                        }
                    }

                    // Total setup time
                    int setupTime = hammingDistance * singleSwapTime;
                    
                    // If two different operations use the same tools, impose a minimal setup time = 1
                    // e.g. loading CNC program
                    baseMatrix[i][j] = std::max(1, setupTime);
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