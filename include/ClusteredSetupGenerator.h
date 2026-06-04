#pragma once

#include "SetupTimeGenerator.h"

namespace jobshop {
    /**
     * @brief Variant 4: Clustered (Family-based) setup times.
     * * Represents Group Technology principles common in metalworking and CNC machining.
     * Operation types are randomly and evenly assigned to distinct "clusters" (tool families).
     * Transitions within the same cluster (minor setup) are fast, while transitions
     * between different clusters (major setup) take significantly longer.
     */
    class ClusteredSetupGenerator : public SetupTimeGenerator {
    private:
        // Number of operation clusters
        int numClusters = 3; ///< Number of distinct operation clusters (families).

        double shortLower = 0.1; ///< Lower bound multiplier for intra-cluster setups.
        double shortUpper = 0.2; ///< Upper bound multiplier for intra-cluster setups.

        double longLower = 0.4; ///< Lower bound multiplier for inter-cluster setups.
        double longUpper = 0.6; ///< Upper bound multiplier for inter-cluster setups.

    public:
        std::vector<std::vector<std::vector<int> > > generate(
            const std::vector<std::vector<int> > &OMtime,
            int numM,
            int numO,
            const SetupConfig &config,
            std::mt19937 &gen) override;
    };
} // namespace jobshop
