#pragma once

#include "SetupTimeGenerator.h"

namespace jobshop {

    /**
     * Variant 1 of setup time generation - completly randome
     */
    class RandomSetupGenerator : public SetupTimeGenerator {
    public:
        std::vector<std::vector<std::vector<int>>> generate(
            const std::vector<std::vector<int>>& OMtime,
            int numM,
            int numO,
            const SetupConfig& config,
            std::mt19937& gen) override;
    };

} // namespace jobshop