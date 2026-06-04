#pragma once

#include "SetupTimeGenerator.h"

namespace jobshop {
    /**
     * @brief Variant 2: Machine-dependent setup times.
     * * Simulates a realistic factory environment where machines differ in age,
     * construction, or ergonomics. A single base matrix is generated, and then
     * for each machine, it is scaled by a specific efficiency multiplier drawn
     * from a uniform distribution (e.g., U(0.8, 1.3)).
     */
    class MachineDependentSetupGenerator : public SetupTimeGenerator {
    private:
        double machineMultiplierMin = 0.8; ///< Lower bound for machine efficiency multiplier.
        double machineMultiplierMax = 1.3; ///< Upper bound for machine efficiency multiplier.

    public:
        std::vector<std::vector<std::vector<int> > > generate(
            const std::vector<std::vector<int> > &OMtime,
            int numM,
            int numO,
            const SetupConfig &config,
            std::mt19937 &gen) override;
    };
} // namespace jobshop
