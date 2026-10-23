#pragma once

#include <vector>
#include <memory>
#include <random>
#include <string>
#include <stdexcept>

namespace jobshop {
    /**
     * @brief Available setup time generation variants.
     */
    enum class SetupVariant {
        Random = 1, ///< Uniform random setup times ("random").
        MachineDependent = 2, ///< Machine-dependent setup times ("machine").
        Asymmetric = 3, ///< Asymmetric setup times ("asymmetric").
        Clustered = 4, ///< Clustered operation types ("clustered").
        Resource = 5 ///< Resource-swap based setup times ("resource").
    };

    /**
     * @brief Parses a CLI variant name into a SetupVariant.
     * @param name One of: random, machine, asymmetric, clustered, resource.
     * @throws std::runtime_error if the name is unknown.
     */
    inline SetupVariant parseSetupVariant(const std::string &name) {
        if (name == "random") return SetupVariant::Random;
        if (name == "machine") return SetupVariant::MachineDependent;
        if (name == "asymmetric") return SetupVariant::Asymmetric;
        if (name == "clustered") return SetupVariant::Clustered;
        if (name == "resource") return SetupVariant::Resource;
        throw std::runtime_error("Invalid --setup_variant '" + name + "'. Use: random, machine, asymmetric, clustered, resource.");
    }

    /**
     * @brief Configuration parameters for setup time generation.
     */
    struct SetupConfig {
        SetupVariant variant = SetupVariant::Resource; ///< Generation variant to use.
        double eta = 1.0; ///< Global multiplier for setup times.
        double a = 0.2; ///< Lower bound parameter for random distribution.
        double b = 0.4; ///< Upper bound parameter for random distribution.
        double alpha = 0.5; ///< Additional scaling parameter.
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
     * @param variant Generation strategy.
     * @return A unique pointer to the chosen SetupTimeGenerator implementation.
     */
    std::unique_ptr<SetupTimeGenerator> createSetupTimeGenerator(SetupVariant variant);
} // namespace jobshop
