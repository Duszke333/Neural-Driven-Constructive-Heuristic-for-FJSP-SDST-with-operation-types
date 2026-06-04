#pragma once

#include "SetupTimeGenerator.h"

namespace jobshop {
    /**
     * @brief Variant 3: Asymmetric setup times.
     * * Reflects industries with strict sanitary or thermal requirements (e.g., chemical, food).
     * Operations are divided into Standard (Even ID) and Special (Odd ID).
     * Transitioning from a Special operation to a Standard one incurs a heavy time penalty
     * (e.g., rigorous machine cleaning), making the setup matrix highly asymmetric.
     */
    class AsymmetricSetupGenerator : public SetupTimeGenerator {
    private:
        double shortLower = 0.1; ///< Lower bound multiplier for short setups.
        double shortUpper = 0.2; ///< Upper bound multiplier for short setups.
        double longLower = 0.4; ///< Lower bound multiplier for long (penalty) setups.
        double longUpper = 0.5; ///< Upper bound multiplier for long (penalty) setups.

    public:
        std::vector<std::vector<std::vector<int> > > generate(
            const std::vector<std::vector<int> > &OMtime,
            int numM,
            int numO,
            const SetupConfig &config,
            std::mt19937 &gen) override;
    };
} // namespace jobshop
