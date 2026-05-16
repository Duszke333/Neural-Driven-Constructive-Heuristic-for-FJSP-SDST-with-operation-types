#pragma once

#include "SetupTimeGenerator.h"

namespace jobshop {

    /**
     * Variant 5 of setup generation - resource exchange.
     * Each operation type has a binary vector of required tools.
     * Setup time is proportional to Hamming distance between vectors (number of tools that need to be physically swapped).
     */
    class ResourceSetupGenerator : public SetupTimeGenerator {
    private:
        int numResources = 4;
        double swapTimeMultiplier = 0.1;
        double resourceProbability = 0.5;

    public:
        std::vector<std::vector<std::vector<int> > > generate(
            const std::vector<std::vector<int> > &OMtime,
            int numM,
            int numO,
            const SetupConfig &config,
            std::mt19937 &gen) override;
    };

} // namespace jobshop