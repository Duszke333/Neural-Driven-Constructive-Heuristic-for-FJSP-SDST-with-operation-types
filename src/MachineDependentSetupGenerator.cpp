#include "MachineDependentSetupGenerator.h"
#include <cmath>
#include <algorithm>
#include "Err.h"

namespace jobshop {
    std::vector<std::vector<std::vector<int> > > MachineDependentSetupGenerator::generate(
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
        if (lower < 1) lower = 1;
        if (upper <= lower) upper = lower + 1;

        std::uniform_int_distribution<int> baseDist(lower, upper);

        // Generate base matrix N x N for operations
        std::vector<std::vector<int> > baseMatrix(numO, std::vector<int>(numO, 0));
        for (int i = 0; i < numO; ++i) {
            for (int j = 0; j < numO; ++j) {
                if (i != j) {
                    baseMatrix[i][j] = baseDist(gen);
                }
            }
        }

        // Clone matrix for every machine to get 3D set
        std::vector<std::vector<std::vector<int> > > setupTimes(
            numM, std::vector<std::vector<int> >(numO, std::vector<int>(numO, 0))
        );

        // Machine multiplier distribution U(0.8, 1.3)
        std::uniform_real_distribution<double> machineDist(machineMultiplierMin, machineMultiplierMax);

        // Apply multiplier for each machine independently
        for (int m = 0; m < numM; ++m) {
            double coeff = machineDist(gen);
            INFO("Machine " << m << " multiplier: " << coeff);
            for (int i = 0; i < numO; ++i) {
                for (int j = 0; j < numO; ++j) {
                    if (i != j) {
                        int scaledVal = static_cast<int>(std::round(baseMatrix[i][j] * coeff));
                        setupTimes[m][i][j] = std::max(1, scaledVal); // Make it at least 1
                    }
                }
            }
        }

        // Floyd-Warshall for each machine
        for (int m = 0; m < numM; ++m) {
            INFO("Machine " << m << " FW");
            enforceTriangleInequality(setupTimes[m], numO);
        }

        return setupTimes;
    }
} // namespace jobshop
