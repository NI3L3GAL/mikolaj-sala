#include "GaussianSolver.hpp"
#include <stdexcept>
#include <cmath>
#include <algorithm>

std::string GaussianSolver::getName() const {
    return "Gausian elimination (with partial choosing of main element)";
}

std::vector<Scalar> GaussianSolver::solve(
    const std::vector<std::vector<Scalar>>& matrixData,
    const std::vector<Scalar>& rhs) const {
    const std::size_t n = matrixData.size();
    if (n == 0) {
        throw std::invalid_argument("Coefficient matrix cannot be empty.");
    }
    if (rhs.size() != n) {
        throw std::invalid_argument("Length of parameters vector need to be equal to dimention of matrix.");
    }
    for (const auto& row : matrixData) {
        if (row.size() != n) {
            throw std::invalid_argument("Coefficient matrix need to be square.");
        }
    }

    // Creation of expanded matrix [A | b]
    std::vector<std::vector<Scalar>> aug(n, std::vector<Scalar>(n + 1, Scalar(0.0, 0.0)));
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            aug[i][j] = matrixData[i][j];
        }
        aug[i][n] = rhs[i];
    }

    // Down elimination, choosing main element in column
    for (std::size_t col = 0; col < n; ++col) {
        std::size_t pivotRow = col;
        double maxAbs = std::abs(aug[col][col]);
        for (std::size_t row = col + 1; row < n; ++row) {
            double candAbs = std::abs(aug[row][col]);
            if (candAbs > maxAbs) {
                maxAbs = candAbs;
                pivotRow = row;
            }
        }

        if (maxAbs <= scalar_utils::kEpsilon) {
            throw std::runtime_error("Set is inconsistient or dependent.");
        }

        if (pivotRow != col) {
            std::swap(aug[col], aug[pivotRow]);
        }

        Scalar pivot = aug[col][col];
        for (std::size_t row = col + 1; row < n; ++row) {
            Scalar factor = aug[row][col] / pivot;
            aug[row][col] = Scalar(0.0, 0.0);
            for (std::size_t k = col + 1; k <= n; ++k) {
                aug[row][k] -= factor * aug[col][k];
            }
        }
    }

    // Back substitution
    std::vector<Scalar> solution(n, Scalar(0.0, 0.0));
    for (std::size_t i = n; i-- > 0;) {
        Scalar sum = aug[i][n];
        for (std::size_t j = i + 1; j < n; ++j) {
            sum -= aug[i][j] * solution[j];
        }
        if (scalar_utils::isNearlyZero(aug[i][i])) {
            throw std::runtime_error("Division by zero detected during back substitution.");
        }
        solution[i] = scalar_utils::cleanZero(sum / aug[i][i]);
    }

    return solution;
}