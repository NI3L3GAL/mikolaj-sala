#ifndef VECTOR_HPP
#define VECTOR_HPP

#include "Scalar.hpp"
#include <vector>
#include <string>
#include <cstddef>

/**
 * @brief Class representation of vector over complex body (Scalar).
 */
class Vector {
private:
    std::vector<Scalar> elements;

public:
    /**
     * @brief Creates vector object of given length with initial values.
     */
    explicit Vector(std::size_t size = 0, const Scalar& initialValue = Scalar(0.0, 0.0));

    /**
     * @brief Creates vector object with given values.
     */
    explicit Vector(const std::vector<Scalar>& values);

    std::size_t size() const noexcept;
    bool empty() const noexcept;

    /**
     * @brief secured access to elements with range control.
     */
    Scalar& at(std::size_t index);
    const Scalar& at(std::size_t index) const;

    const std::vector<Scalar>& data() const noexcept;

    // Operacje wektorowe
    Vector operator+(const Vector& other) const;
    Vector operator-(const Vector& other) const;
    Vector operator*(const Scalar& scalar) const;

    /**
     * @brief Dot product (Hermit's: sum_i conj(u_i) * v_i),
     * for real values it is just normal dot product.
     */
    Scalar dotProduct(const Vector& other) const;

    /**
     * @brief Cross produtct for 3-element vectors (u x v).
     */
    Vector crossProduct3D(const Vector& other) const;

    /**
     * @brief Euclidean norm (L2).
     */
    double normL2() const noexcept;

    /**
     * @brief Maximum norm (L-infinity).
     */
    double normMax() const noexcept;

    /**
     * @brief Returns normalized copy of vector (with L2 norm).
     */
    Vector normalized() const;

    std::string toString(int precision = 4) const;
};

Vector operator*(const Scalar& scalar, const Vector& vec);

#endif // VECTOR_HPP