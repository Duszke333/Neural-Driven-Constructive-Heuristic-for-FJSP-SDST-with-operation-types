#pragma once
#include <concepts>

namespace chof {
    template<typename T>
    concept BlackBoxEvaluatorConcept = requires(T t, const double *params, int n)
    {
        { t(params, n) } -> std::same_as<double>;
    };
}

