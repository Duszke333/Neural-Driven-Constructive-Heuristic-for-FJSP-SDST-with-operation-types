#pragma once

#include "SetupTimeGenerator.h"

namespace jobshop {
    /**
     * @brief Variant 1: Completely random setup times.
     * * This strategy generates setup times completely independently for each transition
     * between operation types. The transition time from one type to another is identical
     * across all machines. Values are drawn from a uniform distribution based on the global
     * average processing time (mu) and configuration parameters.
     */
    class RandomSetupGenerator : public SetupTimeGenerator {
    public:
        std::vector<std::vector<std::vector<int> > > generate(
            const std::vector<std::vector<int> > &OMtime,
            int numM,
            int numO,
            const SetupConfig &config,
            std::mt19937 &gen) override;
    };
} // namespace jobshop
