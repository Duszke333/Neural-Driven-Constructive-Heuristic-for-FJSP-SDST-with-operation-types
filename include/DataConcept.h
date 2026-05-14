//
// Created by tsliwins on 17.12.24.
//

#pragma once

namespace chof {
    template<typename T>
    concept SolutionConcept = std::copyable<T> && requires(const T t)
    {
        { t.getObj() } -> std::same_as<double>;
    };

    template<typename T>
    concept ClusteredSolutionConcept = std::copyable<T> && requires(const T t, int c)
    {
        { t.getObj() } -> std::same_as<double>;
        { t.setCluster(c) } -> std::same_as<void>;
    };

    template<typename T>
    concept DataConcept = std::copyable<T> && requires(T t, const typename T::SolutionType &S)
    {
        { t.getObj() } -> std::same_as<double>;
        { t.getSolution() } -> std::same_as<const typename T::SolutionType &>;
        { t.setSolution(S) } -> std::same_as<void>;
        typename T::SolutionType;
    } && (SolutionConcept<typename T::SolutionType> || ClusteredSolutionConcept<typename T::SolutionType>);


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