#pragma once

#include <vector>
#include <memory>
#include <random>

namespace jobshop {
    struct SetupConfig {
        int variant;
        double eta;
        double a;
        double b;
        double alpha;
    };

    /**
     * Abstract base class for setup times generation strategies
     */
    class SetupTimeGenerator {
    public:
        virtual ~SetupTimeGenerator() = default;

        /**
         * Generates the Setup Times Matrix: [machine][op-from][op-to]
         */
        virtual std::vector<std::vector<std::vector<int> > > generate(
            const std::vector<std::vector<int> > &OMtime,
            int numM,
            int numO,
            const SetupConfig &config,
            std::mt19937 &gen) = 0;

    protected:
        /**
         * Applies Floyd-Warshall algorithm to enforce triangle inequality
         */
        static void enforceTriangleInequality(std::vector<std::vector<int> > &setupTimes, int numO);

        /**
         * Calculates global mu based on OMtime matrix
         */
        static double calculateMu(const std::vector<std::vector<int> > &OMtime);
    };

    /**
     * Factory that creates the generator based on variant
     */
    std::unique_ptr<SetupTimeGenerator> createSetupTimeGenerator(int variant);
} // namespace jobshop
