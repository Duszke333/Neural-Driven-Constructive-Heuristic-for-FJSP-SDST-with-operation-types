#pragma once
#include <concepts>

namespace chof {
    /**
     * @brief Defines an interface for a black-box evaluator.
     * * This concept requires the type T to be a functor (overloaded operator()),
     * which takes a pointer to an array of double parameters and its size,
     * and returns the fitness/objective value as a double.
     * It is primarily used by continuous optimization algorithms (e.g., CMA-ES).
     * * @tparam T The type to be checked against the concept.
     */
    template<typename T>
    concept BlackBoxEvaluatorConcept = requires(T t, const double *params, int n)
    {
        { t(params, n) } -> std::same_as<double>;
    };
}
