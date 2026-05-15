#pragma once

#include "SetupTimeGenerator.h"

namespace jobshop {
    /**
     * Variant 3 of setup time generation - asymmetric times
     * Reflects difficulty of setups. Even ID = standard operation, Odd ID = special operation.
     * Transition from Special to Standard operation takes a long time, other types are short.
     */
    class AsymmetricSetupGenerator : public SetupTimeGenerator {
    private:
        double shortLower = 0.1;
        double shortUpper = 0.2;
        double longLower = 0.4;
        double longUpper = 0.5;

    public:
        std::vector<std::vector<std::vector<int> > > generate(
            const std::vector<std::vector<int> > &OMtime,
            int numM,
            int numO,
            const SetupConfig &config,
            std::mt19937 &gen) override;
    };
} // namespace jobshop
