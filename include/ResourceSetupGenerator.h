#pragma once

#include "SetupTimeGenerator.h"

namespace jobshop {
    /**
     * @brief Variant 5: Resource exchange setup times.
     * * Accurately models CNC tool magazines or SMT pick-and-place machines.
     * Each operation type is assigned a binary vector representing required tools.
     * The setup time between two operations is strictly proportional to the
     * Hamming distance between their resource vectors (the exact number of tools
     * that must be physically swapped).
     */
    class ResourceSetupGenerator : public SetupTimeGenerator {
    private:
        int numResources = 4; ///< Size of the global tool pool.
        double swapTimeMultiplier = 0.1; ///< Time cost to swap a single tool (relative to mu).
        double resourceProbability = 0.5; ///< Probability of an operation requiring a specific tool.

    public:
        std::vector<std::vector<std::vector<int> > > generate(
            const std::vector<std::vector<int> > &OMtime,
            int numM,
            int numO,
            const SetupConfig &config,
            std::mt19937 &gen) override;
    };
} // namespace jobshop
