#pragma once
#include <concepts>
#include "DataConcept.h"

namespace chof {
    using namespace std;
    template<typename T>
    concept ConstructionHeuristicConcept = requires(T t, const T ct, const double *params, int n,
                                                    vector<double> &VParams, const typename T::DataType &IOD)
    {
        { ct.maximize() } -> std::same_as<bool>;
        { ct.getParamsSize() } -> std::same_as<int>;
        { t.setParams(params, n) } -> std::same_as<T &>;
        { ct.getParams(VParams) } -> std::same_as<void>; //< appends parameters to the VParams vector
        { t.run(IOD) } -> std::same_as<typename T::DataType::SolutionType>;
        typename T::DataType;
        typename T::ConfigType;
    } && DataConcept<typename T::DataType>;
}