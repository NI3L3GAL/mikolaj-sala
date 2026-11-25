#include "Matrix.hpp"
#include <stdexcept>
#include <sstream>
#include <cmath>
#include <algorithm>

Matrix::Matrix(std::size_t rows, std::size_t cols, const Scalar& initialValue)
    : rowsCount(rows), colsCount(cols),
      grid(rows, std::vector<Scalar>(cols, scalar_utils::cleanZero(initialValue))) {}

Matrix::Matrix(const std::vector<std::vector<Scalar>>& values)
    : rowsCount(values.size()),
      colsCount(values.empty() ? 0 : values.front().size()),
      grid(values) {
    for (std::size_t i = 0; i < rowsCount; ++i) {
        if (grid[i].size() != colsCount) {
            throw std::invalid_argument("Matrix's rows need to have same length.");
        }
        for (std::size_t j = 0; j < colsCount; ++j) {
            grid[i][j] = scalar_utils::cleanZero(grid[i][j]);
        }
    }
}

std::size_t Matrix::rows() const noexcept {
    return rowsCount;
}

std::size_t Matrix::cols() const noexcept {
    return colsCount;
}

bool Matrix::isSquare() const noexcept {
    return rowsCount > 0 && rowsCount == colsCount;
}

bool Matrix::empty() const noexcept {
    return rowsCount == 0 || colsCount == 0;
}

Scalar& Matrix::at(std::size_t r, std::size_t c) {
    if (r >= rowsCount || c >= colsCount) {
        throw std::out_of_range("Matrix's index out of the range.");
    }
    return grid[r][c];
}

const Scalar& Matrix::at(std::size_t r, std::size_t c) const {
    if (r >= rowsCount || c >= colsCount) {
        throw std::out_of_range("Matrix's index out of the range.");
    }
    return grid[r][c];
}

const std::vector<std::vector<Scalar>>& Matrix::data() const noexcept {
    return grid;
}

Matrix Matrix::identity(std::size_t size) {
    Matrix id(size, size, Scalar(0.0, 0.0));
    for (std::size_t i = 0; i < size; ++i) {
        id.grid[i][i] = Scalar(1.0, 0.0);
    }
    return id;
}

Matrix Matrix::minorMatrix(std::size_t skipRow, std::size_t skipCol) const {
    Matrix sub(rowsCount - 1, colsCount - 1);
    std::size_t outR = 0;
    for (std::size_t i = 0; i < rowsCount; ++i) {
        if (i == skipRow) {
            continue;
        }
        std::size_t outC = 0;
        for (std::size_t j = 0; j < colsCount; ++j) {
            if (j == skipCol) {
                continue;
            }
            sub.grid[outR][outC] = grid[i][j];
            ++outC;
        }
        ++outR;
    }
    return sub;
}

Matrix Matrix::operator+(const Matrix& other) const {
    if (rowsCount != other.rowsCount || colsCount != other.colsCount) {
        throw std::invalid_argument("Matricies need to have same dimentions for addition.");
    }
    Matrix result(rowsCount, colsCount);
    for (std::size_t i = 0; i < rowsCount; ++i) {
        for (std::size_t j = 0; j < colsCount; ++j) {
            result.grid[i][j] = scalar_utils::cleanZero(grid[i][j] + other.grid[i][j]);
        }
    }
    return result;
}

Matrix Matrix::operator-(const Matrix& other) const {
    if (rowsCount != other.rowsCount || colsCount != other.colsCount) {
        throw std::invalid_argument("Matricies need to have same dimentions for substraction.");
    }
    Matrix result(rowsCount, colsCount);
    for (std::size_t i = 0; i < rowsCount; ++i) {
        for (std::size_t j = 0; j < colsCount; ++j) {
            result.grid[i][j] = scalar_utils::cleanZero(grid[i][j] - other.grid[i][j]);
        }
    }
    return result;
}

Matrix Matrix::operator*(const Matrix& other) const {
    if (colsCount != other.rowsCount) {
        throw std::invalid_argument("Number of columns of first matrix need to be the equal to number of rows of the second one.");
    }
    Matrix result(rowsCount, other.colsCount, Scalar(0.0, 0.0));
    for (std::size_t i = 0; i < rowsCount; ++i) {
        for (std::size_t k = 0; k < colsCount; ++k) {
            Scalar aik = grid[i][k];
            if (scalar_utils::isNearlyZero(aik)) {
                continue;
            }
            for (std::size_t j = 0; j < other.colsCount; ++j) {
                result.grid[i][j] += aik * other.grid[k][j];
            }
        }
    }
    for (std::size_t i = 0; i < rowsCount; ++i) {
        for (std::size_t j = 0; j < other.colsCount; ++j) {
            result.grid[i][j] = scalar_utils::cleanZero(result.grid[i][j]);
        }
    }
    return result;
}

Vector Matrix::operator*(const Vector& vec) const {
    if (colsCount != vec.size()) {
        throw std::invalid_argument("Number of columns of matrix need to be equel to vector's length.");
    }
    Vector result(rowsCount, Scalar(0.0, 0.0));
    for (std::size_t i = 0; i < rowsCount; ++i) {
        Scalar sum(0.0, 0.0);
        for (std::size_t j = 0; j < colsCount; ++j) {
            sum += grid[i][j] * vec.at(j);
        }
        result.at(i) = scalar_utils::cleanZero(sum);
    }
    return result;
}

Matrix Matrix::operator*(const Scalar& scalar) const {
    Matrix result(rowsCount, colsCount);
    for (std::size_t i = 0; i < rowsCount; ++i) {
        for (std::size_t j = 0; j < colsCount; ++j) {
            result.grid[i][j] = scalar_utils::cleanZero(grid[i][j] * scalar);
        }
    }
    return result;
}

Matrix Matrix::transpose() const {
    Matrix result(colsCount, rowsCount);
    for (std::size_t i = 0; i < rowsCount; ++i) {
        for (std::size_t j = 0; j < colsCount; ++j) {
            result.grid[j][i] = grid[i][j];
        }
    }
    return result;
}

Matrix Matrix::conjugateTranspose() const {
    Matrix result(colsCount, rowsCount);
    for (std::size_t i = 0; i < rowsCount; ++i) {
        for (std::size_t j = 0; j < colsCount; ++j) {
            result.grid[j][i] = scalar_utils::cleanZero(std::conj(grid[i][j]));
        }
    }
    return result;
}

Scalar Matrix::trace() const {
    if (!isSquare()) {
        throw std::invalid_argument("Trace can be calculated only for non-zero square matrix.");
    }
    Scalar sum(0.0, 0.0);
    for (std::size_t i = 0; i < rowsCount; ++i) {
        sum += grid[i][i];
    }
    return scalar_utils::cleanZero(sum);
}

Scalar Matrix::determinantRecursive() const {
    if (!isSquare()) {
        throw std::invalid_argument("Determinant can be calculated only for non-zero square matrix.");
    }
    if (rowsCount > 9) {
        throw std::domain_error("Recursive expansion is bounded to matricies of max 9x9 dimensions (use elimination method).");
    }
    // Base requirement 1: matrix 1x1
    if (rowsCount == 1) {
        return scalar_utils::cleanZero(grid[0][0]);
    }
    // Base requirement 2: matrix 2x2
    if (rowsCount == 2) {
        return scalar_utils::cleanZero(grid[0][0] * grid[1][1] - grid[0][1] * grid[1][0]);
    }

    // Recursive step: expansion over first row
    Scalar det(0.0, 0.0);
    for (std::size_t col = 0; col < colsCount; ++col) {
        if (scalar_utils::isNearlyZero(grid[0][col])) {
            continue;
        }
        double sign = ((col % 2) == 0) ? 1.0 : -1.0;
        Matrix sub = minorMatrix(0, col);
        det += sign * grid[0][col] * sub.determinantRecursive();
    }
    return scalar_utils::cleanZero(det);
}

Scalar Matrix::determinantElimination() const {
    if (!isSquare()) {
        throw std::invalid_argument("Determinant can be calculated only for non-zero square matrix.");
    }
    const std::size_t n = rowsCount;
    std::vector<std::vector<Scalar>> work = grid;
    Scalar det(1.0, 0.0);

    for (std::size_t col = 0; col < n; ++col) {
        std::size_t pivotRow = col;
        double bestAbs = std::abs(work[col][col]);
        for (std::size_t row = col + 1; row < n; ++row) {
            double candAbs = std::abs(work[row][col]);
            if (candAbs > bestAbs) {
                bestAbs = candAbs;
                pivotRow = row;
            }
        }

        if (bestAbs <= scalar_utils::kEpsilon) {
            return Scalar(0.0, 0.0);
        }

        if (pivotRow != col) {
            std::swap(work[col], work[pivotRow]);
            det = -det;
        }

        Scalar pivot = work[col][col];
        det *= pivot;

        for (std::size_t row = col + 1; row < n; ++row) {
            Scalar factor = work[row][col] / pivot;
            for (std::size_t k = col + 1; k < n; ++k) {
                work[row][k] -= factor * work[col][k];
            }
        }
    }

    return scalar_utils::cleanZero(det);
}

Matrix Matrix::inverse() const {
    if (!isSquare()) {
        throw std::invalid_argument("Macierz odwrotna istnieje tylko dla niepustej macierzy kwadratowej.");
    }
    const std::size_t n = rowsCount;
    // Expanded matrix [A | I]
    std::vector<std::vector<Scalar>> aug(n, std::vector<Scalar>(2 * n, Scalar(0.0, 0.0)));
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            aug[i][j] = grid[i][j];
        }
        aug[i][n + i] = Scalar(1.0, 0.0);
    }

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
            throw std::runtime_error("Identity matrix (det(A) = 0) - inverse do not exist.");
        }

        if (pivotRow != col) {
            std::swap(aug[col], aug[pivotRow]);
        }

        Scalar pivot = aug[col][col];
        for (std::size_t j = col; j < 2 * n; ++j) {
            aug[col][j] /= pivot;
        }

        for (std::size_t row = 0; row < n; ++row) {
            if (row == col) {
                continue;
            }
            Scalar factor = aug[row][col];
            for (std::size_t j = col; j < 2 * n; ++j) {
                aug[row][j] -= factor * aug[col][j];
            }
        }
    }

    Matrix inv(n, n);
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            inv.grid[i][j] = scalar_utils::cleanZero(aug[i][n + j]);
        }
    }
    return inv;
}

std::string Matrix::toString(int precision) const {
    std::ostringstream oss;
    for (std::size_t i = 0; i < rowsCount; ++i) {
        oss << "[ ";
        for (std::size_t j = 0; j < colsCount; ++j) {
            oss << scalar_utils::formatScalar(grid[i][j], precision);
            if (j + 1 < colsCount) {
                oss << "  ";
            }
        }
        oss << " ]\n";
    }
    return oss.str();
}

Matrix operator*(const Scalar& scalar, const Matrix& mat) {
    return mat * scalar;
}