#include "LUSolver.hpp"
#include <stdexcept>
#include <cmath>
#include <algorithm>

std::string LUSolver::getName() const {
    return "Decomposition PA = LU (Doolittle with pivoting)";
}

LUSolver::DecompositionResult LUSolver::decompose(
    const std::vector<std::vector<Scalar>>& matrixData) const {
    const std::size_t n = matrixData.size();
    if (n == 0) {
        throw std::invalid_argument("Matrix cannot be empty.");
    }
    for (const auto& row : matrixData) {
        if (row.size() != n) {
            throw std::invalid_argument("LU decomposition require square matrix.");
        }
    }

    DecompositionResult res;
    res.lower.assign(n, std::vector<Scalar>(n, Scalar(0.0, 0.0)));
    res.upper = matrixData;
    res.permutation.resize(n);
    for (std::size_t i = 0; i < n; ++i) {
        res.permutation[i] = i;
        res.lower[i][i] = Scalar(1.0, 0.0);
    }

    for (std::size_t col = 0; col < n; ++col) {
        std::size_t pivotRow = col;
        double maxAbs = std::abs(res.upper[col][col]);
        for (std::size_t row = col + 1; row < n; ++row) {
            double candAbs = std::abs(res.upper[row][col]);
            if (candAbs > maxAbs) {
                maxAbs = candAbs;
                pivotRow = row;
            }
        }

        if (maxAbs <= scalar_utils::kEpsilon) {
            throw std::runtime_error("Identity matrix detected - cannot perform unambiguous decomposition.");
        }

        if (pivotRow != col) {
            std::swap(res.upper[col], res.upper[pivotRow]);
            std::swap(res.permutation[col], res.permutation[pivotRow]);
            // Change of calculated multipliers in L matrix (column 0 .. col-1)
            for (std::size_t k = 0; k < col; ++k) {
                std::swap(res.lower[col][k], res.lower[pivotRow][k]);
            }
        }

        Scalar pivot = res.upper[col][col];
        for (std::size_t row = col + 1; row < n; ++row) {
            Scalar multiplier = res.upper[row][col] / pivot;
            res.lower[row][col] = scalar_utils::cleanZero(multiplier);
            res.upper[row][col] = Scalar(0.0, 0.0);
            for (std::size_t k = col + 1; k < n; ++k) {
                res.upper[row][k] = scalar_utils::cleanZero(res.upper[row][k] - multiplier * res.upper[col][k]);
            }
        }
    }

    return res;
}

std::vector<Scalar> LUSolver::solve(
    const std::vector<std::vector<Scalar>>& matrixData,
    const std::vector<Scalar>& rhs) const {
    const std::size_t n = matrixData.size();
    if (rhs.size() != n) {
        throw std::invalid_argument("Length of parameters vector needs to be equal to matrix dimentions.");
    }

    DecompositionResult lu = decompose(matrixData);

    // 1. Permutation of parameters vector: pb = P * b
    std::vector<Scalar> pb(n, Scalar(0.0, 0.0));
    for (std::size_t i = 0; i < n; ++i) {
        pb[i] = rhs[lu.permutation[i]];
    }

    // 2. Forward substitution: L * y = P * b
    std::vector<Scalar> y(n, Scalar(0.0, 0.0));
    for (std::size_t i = 0; i < n; ++i) {
        Scalar sum = pb[i];
        for (std::size_t j = 0; j < i; ++j) {
            sum -= lu.lower[i][j] * y[j];
        }
        y[i] = scalar_utils::cleanZero(sum); // L[i][i] == 1.0
    }

    // 3. Back substitution: U * x = y
    std::vector<Scalar> x(n, Scalar(0.0, 0.0));
    for (std::size_t i = n; i-- > 0;) {
        Scalar sum = y[i];
        for (std::size_t j = i + 1; j < n; ++j) {
            sum -= lu.upper[i][j] * x[j];
        }
        if (scalar_utils::isNearlyZero(lu.upper[i][i])) {
            throw std::runtime_error("U matrix has zero in its diagonal.");
        }
        x[i] = scalar_utils::cleanZero(sum / lu.upper[i][i]);
    }

    return x;
}