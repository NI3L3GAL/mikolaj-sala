#ifndef ALGEBRA_ENTITY_HPP
#define ALGEBRA_ENTITY_HPP_HPP

#include "Scalar.hpp"
#include <string>
#include <vector>

/**
 * @brief Abstract base class for solvers.
 * Provides common polimorphic interface and virtual destructor.
 */
class AlgEntity {
public:
    virtual ~AlgEntity() = default;

    /**
     * @brief Name of solver method.
     */
    virtual std::string getName() const = 0;

    /**
     * @brief Solve set of linear equation A * x = b for square matricies A.
     * @param matrixData Coeff matrix in form of vector (n x n).
     * @param rhs Vector of constant terms b (length n).
     * @return Vector of result x (length n).
     * @throws std::invalid_argument or std::runtime_error for invalid dimentions or singularity.
     */
    virtual std::vector<Scalar> solve(
        const std::vector<std::vector<Scalar>>& matrixData,
        const std::vector<Scalar>& rhs) const = 0;
};

#endif // ALGEBRA_ENTITY_HPP