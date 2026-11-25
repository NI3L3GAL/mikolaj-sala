#ifndef MATRIX_HPP
#define MATRIX_HPP

#include "Scalar.hpp"
#include "Vector.hpp"
#include <vector>
#include <string>
#include <cstddef>

/**
 * @brief Class representation of matrices (m x n) over the complex body (Scalar).
 */
class Matrix {
private:
    std::size_t rowsCount;
    std::size_t colsCount;
    std::vector<std::vector<Scalar>> grid;

    /**
     * @brief Returns minor matrix after deletion of given row and column.
     */
    Matrix minorMatrix(std::size_t skipRow, std::size_t skipCol) const;

public:
    Matrix(std::size_t rows = 0, std::size_t cols = 0, const Scalar& initialValue = Scalar(0.0, 0.0));
    explicit Matrix(const std::vector<std::vector<Scalar>>& values);

    std::size_t rows() const noexcept;
    std::size_t cols() const noexcept;
    bool isSquare() const noexcept;
    bool empty() const noexcept;

    Scalar& at(std::size_t r, std::size_t c);
    const Scalar& at(std::size_t r, std::size_t c) const;

    const std::vector<std::vector<Scalar>>& data() const noexcept;

    /**
     * @brief Identity matrix n x n.
     */
    static Matrix identity(std::size_t size);

    // Operacje macierzowe
    Matrix operator+(const Matrix& other) const;
    Matrix operator-(const Matrix& other) const;
    Matrix operator*(const Matrix& other) const;
    Vector operator*(const Vector& vec) const;
    Matrix operator*(const Scalar& scalar) const;

    /**
     * @brief Matrix transposition (A^T).
     */
    Matrix transpose() const;

    /**
     * @brief Hermitean Conjuction (A^H = conj(A^T)).
     */
    Matrix conjugateTranspose() const;

    /**
     * @brief Square matrix trace (sum over main diagonal).
     */
    Scalar trace() const;

    /**
     * @brief Recursive determinant (Laplace expansion).
     * Bonded for security to size <= 9.
     */
    Scalar determinantRecursive() const;

    /**
     * @brief Determinant using elimination method with choosing main element (fast and stable).
     */
    Scalar determinantElimination() const;

    /**
     * @brief Inverse of square matrix (Gauss-Jordan elimination method with pivoting).
     */
    Matrix inverse() const;

    std::string toString(int precision = 4) const;
};

Matrix operator*(const Scalar& scalar, const Matrix& mat);

#endif // MATRIX_HPP