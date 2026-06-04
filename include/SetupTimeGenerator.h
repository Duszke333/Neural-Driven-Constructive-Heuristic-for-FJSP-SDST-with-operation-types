#pragma once

#include <vector>
#include <memory>
#include <random>

namespace jobshop {
    /**
     * @brief Configuration parameters for setup time generation.
     */
    struct SetupConfig {
        int variant; ///< ID of the generation variant to use (1-5).
        double eta; ///< Global multiplier for setup times.
        double a; ///< Lower bound parameter for random distribution.
        double b; ///< Upper bound parameter for random distribution.
        double alpha; ///< Additional scaling parameter.
    };

    /**
     * @brief Abstract base class for Sequence-Dependent Setup Times (SDST) generation strategies.
     * Utilizes the Strategy Design Pattern to allow dynamic switching between different
     * setup time generation algorithms.
     */
    class SetupTimeGenerator {
    public:
        virtual ~SetupTimeGenerator() = default;

        /**
         * @brief Generates the 3D Setup Times Matrix.
         * @param OMtime The base processing times matrix [Operation][Machine].
         * @param numM Total number of machines.
         * @param numO Total number of operation types.
         * @param config Generation parameters.
         * @param gen Random number generator instance.
         * @return A 3D vector representing setup times indexed as [Machine][FromOperation][ToOperation].
         */
        virtual std::vector<std::vector<std::vector<int> > > generate(
            const std::vector<std::vector<int> > &OMtime,
            int numM,
            int numO,
            const SetupConfig &config,
            std::mt19937 &gen) = 0;

    protected:
        /**
         * @brief Applies the Floyd-Warshall algorithm to enforce the triangle inequality.
         * Modifies the provided matrix in-place to ensure no setup transition can be
         * artificially shortened by passing through an intermediate "dummy" operation.
         * @param setupTimes A 2D matrix representing setup times on a single machine.
         * @param numO Total number of operation types.
         */
        static void enforceTriangleInequality(std::vector<std::vector<int> > &setupTimes, int numO);

        /**
         * @brief Calculates the global average processing time (mu) across all valid machine-operation pairs.
         * @param OMtime The processing times matrix [Operation][Machine].
         * @return The average processing time (mu), excluding zero values.
         */
        static double calculateMu(const std::vector<std::vector<int> > &OMtime);
    };

    /**
     * @brief Factory method that instantiates the appropriate generator based on the configuration variant.
     * @param variant Integer ID representing the generation strategy (1-5).
     * @return A unique pointer to the chosen SetupTimeGenerator implementation.
     */
    std::unique_ptr<SetupTimeGenerator> createSetupTimeGenerator(int variant);
} // namespace jobshop
