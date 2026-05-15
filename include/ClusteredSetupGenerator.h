#pragma once

#include "SetupTimeGenerator.h"

namespace jobshop {

    /**
     * Variant 4 of setup time generation - clustered setups.
     * Operation types are randomly assigned to "clusters".
     * Setups are short within a cluster, and long between cluster.
     */
    class ClusteredSetupGenerator : public SetupTimeGenerator {
    private:
        // Number of operation clusters
        int numClusters = 3;
        
        double shortLower = 0.1;
        double shortUpper = 0.2;
        
        double longLower = 0.4;
        double longUpper = 0.6;

    public:
        std::vector<std::vector<std::vector<int> > > generate(
            const std::vector<std::vector<int> > &OMtime,
            int numM,
            int numO,
            const SetupConfig &config,
            std::mt19937 &gen) override;
    };

} // namespace jobshop