#pragma once

#include "SetupTimeGenerator.h"

namespace jobshop {

    /**
     * Variant 2 of setup time generation - times are machine-dependent
     * Each machine differs in efficiency (multiplier drawn from distribution U(0.8, 1.3))
     */
    class MachineDependentSetupGenerator : public SetupTimeGenerator {
    private:
        double machineMultiplierMin = 0.8;
        double machineMultiplierMax = 1.3;
    public:
        std::vector<std::vector<std::vector<int> > > generate(
            const std::vector<std::vector<int> > &OMtime,
            int numM,
            int numO,
            const SetupConfig &config,
            std::mt19937 &gen) override;
    };

} // namespace jobshop