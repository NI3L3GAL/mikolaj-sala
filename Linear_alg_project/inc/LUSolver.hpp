#ifndef LU_SOLVER_HPP
#define LU_SOLVER_HPP

#include "AlgEntity.hpp"

/**
 * @brief Set of linear equations solver (Ax = b) decomposition method PA = LU
 * (Doolittle's decomposition with pivoting).
 */
class LUSolver : public AlgEntity {
public:
    struct DecompositionResult {
        std::vector<std::vector<Scalar>> lower;
        std::vector<std::vector<Scalar>> upper;
        std::vector<std::size_t> permutation;
    };

    std::string getName() const override;

    /**
     * @brief Determine matricies L, U and permutation vector P, such as P * A = L * U.
     */
    DecompositionResult decompose(const std::vector<std::vector<Scalar>>& matrixData) const;

    std::vector<Scalar> solve(
        const std::vector<std::vector<Scalar>>& matrixData,
        const std::vector<Scalar>& rhs) const override;
};

#endif // LU_SOLVER_HPP