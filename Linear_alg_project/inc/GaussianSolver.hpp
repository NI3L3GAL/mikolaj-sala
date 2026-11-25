#ifndef GAUSSIAN_SOLVER_HPP
#define GAUSSIAN_SOLVER_HPP

#include "AlgEntity.hpp"

/**
 * @brief Set of linear equations solver (Ax = b) Gaussian elimination method
 * with partial pivoting.
 */
class GaussianSolver : public AlgEntity {
public:
    std::string getName() const override;

    std::vector<Scalar> solve(
        const std::vector<std::vector<Scalar>>& matrixData,
        const std::vector<Scalar>& rhs) const override;
};

#endif // GAUSSIAN_SOLVER_HPP