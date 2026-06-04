//
// Created by tsliwins on 17.12.24.
//

#pragma once

namespace chof {
    /**
     * @brief Basic concept of an optimization problem solution.
     * * Requires the type to provide a method returning the objective function value (getObj).
     * * @tparam T The type to be checked against the concept.
     */
    template<typename T>
    concept SolutionConcept = std::copyable<T> && requires(const T t)
    {
        { t.getObj() } -> std::same_as<double>;
    };

    /**
     * @brief Extension of SolutionConcept for clustered heuristics.
     * * Requires an additional method allowing the assignment of the solution to a specific cluster or group.
     * * @tparam T The type to be checked against the concept.
     * @see SolutionConcept
     */
    template<typename T>
    concept ClusteredSolutionConcept = std::copyable<T> && requires(const T t, int c)
    {
        { t.getObj() } -> std::same_as<double>;
        { t.setCluster(c) } -> std::same_as<void>;
    };

    /**
     * @brief Interface for a class that holds problem instance data (e.g., JobshopData).
     * * Requires defining an internal SolutionType type (compliant with SolutionConcept
     * or ClusteredSolutionConcept) and delivering methods for getting and setting a solution,
     * as well as extracting target function value for the instance.
     * * @tparam T The type to be checked against the concept.
     * @see SolutionConcept
     * @see ClusteredSolutionConcept
     */
    template<typename T>
    concept DataConcept = std::copyable<T> && requires(T t, const typename T::SolutionType &S)
    {
        { t.getObj() } -> std::same_as<double>;
        { t.getSolution() } -> std::same_as<const typename T::SolutionType &>;
        { t.setSolution(S) } -> std::same_as<void>;
        typename T::SolutionType;
    } && (SolutionConcept<typename T::SolutionType> || ClusteredSolutionConcept<typename T::SolutionType>);

    /**
     * @brief Extension of DataConcept covering memory lifecycle management.
     * * Requires init() and free() methods, used to prepare and release data
     * during intensive, parallel evaluations.
     * * @tparam T The type to be checked against the concept.
     * @see DataConcept
     */
    template<typename T>
    concept InitializedDataConcept = std::copyable<T> && requires(T t, const typename T::SolutionType &S)
    {
        { t.getObj() } -> std::same_as<double>;
        { t.getSolution() } -> std::same_as<const typename T::SolutionType &>;
        { t.setSolution(S) } -> std::same_as<void>;
        { t.init() } -> std::same_as<void>;
        { t.free() } -> std::same_as<void>;
        typename T::SolutionType;
    } && (SolutionConcept<typename T::SolutionType> || ClusteredSolutionConcept<typename T::SolutionType>);
}
