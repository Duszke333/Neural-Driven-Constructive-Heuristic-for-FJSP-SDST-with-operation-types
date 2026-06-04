#pragma once
#include <concepts>
#include "DataConcept.h"

namespace chof {
    using namespace std;

    /**
     * @brief Defines an interface for a parameter-driven Construction Heuristic.
     * * To satisfy this concept, the type must provide:
     * - Optimization direction via maximize().
     * - Methods to read and write control parameters (e.g., Neural Network weights):
     * getParamsSize(), setParams(), getParams().
     * - A core execution method run(IOD), which takes the problem instance data
     * and returns the constructed solution (SolutionType).
     *
     * Additionally, it requires the definition of internal types DataType
     * (compliant with DataConcept) and ConfigType (configuration structure).
     * @tparam T The type to be checked against the concept.
     * @see DataConcept
     */
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
