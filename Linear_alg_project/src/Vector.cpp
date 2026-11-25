#include "Vector.hpp"
#include <stdexcept>
#include <sstream>
#include <algorithm>

Vector::Vector(std::size_t size, const Scalar& initialValue)
    : elements(size, scalar_utils::cleanZero(initialValue)) {}

Vector::Vector(const std::vector<Scalar>& values)
    : elements(values) {
    for (Scalar& val : elements) {
        val = scalar_utils::cleanZero(val);
    }
}

std::size_t Vector::size() const noexcept {
    return elements.size();
}

bool Vector::empty() const noexcept {
    return elements.empty();
}

Scalar& Vector::at(std::size_t index) {
    if (index >= elements.size()) {
        throw std::out_of_range("Index of vector out of range.");
    }
    return elements[index];
}

const Scalar& Vector::at(std::size_t index) const {
    if (index >= elements.size()) {
        throw std::out_of_range("Index of vector out of range.");
    }
    return elements[index];
}

const std::vector<Scalar>& Vector::data() const noexcept {
    return elements;
}

Vector Vector::operator+(const Vector& other) const {
    if (elements.size() != other.elements.size()) {
        throw std::invalid_argument("Unable to add vectors with different dimentions.");
    }
    Vector result(elements.size());
    for (std::size_t i = 0; i < elements.size(); ++i) {
        result.elements[i] = scalar_utils::cleanZero(elements[i] + other.elements[i]);
    }
    return result;
}

Vector Vector::operator-(const Vector& other) const {
    if (elements.size() != other.elements.size()) {
        throw std::invalid_argument("Unable to substract vectors with different dimentions");
    }
    Vector result(elements.size());
    for (std::size_t i = 0; i < elements.size(); ++i) {
        result.elements[i] = scalar_utils::cleanZero(elements[i] - other.elements[i]);
    }
    return result;
}

Vector Vector::operator*(const Scalar& scalar) const {
    Vector result(elements.size());
    for (std::size_t i = 0; i < elements.size(); ++i) {
        result.elements[i] = scalar_utils::cleanZero(elements[i] * scalar);
    }
    return result;
}

Scalar Vector::dotProduct(const Vector& other) const {
    if (elements.size() != other.elements.size()) {
        throw std::invalid_argument("Unable to calculate dot product of vectors with different dimentions");
    }
    Scalar sum(0.0, 0.0);
    for (std::size_t i = 0; i < elements.size(); ++i) {
        sum += std::conj(elements[i]) * other.elements[i];
    }
    return scalar_utils::cleanZero(sum);
}

Vector Vector::crossProduct3D(const Vector& other) const {
    if (elements.size() != 3 || other.elements.size() != 3) {
        throw std::invalid_argument("Cross product is defined for 3d vectors.");
    }
    Vector result(3);
    result.elements[0] = scalar_utils::cleanZero(elements[1] * other.elements[2] - elements[2] * other.elements[1]);
    result.elements[1] = scalar_utils::cleanZero(elements[2] * other.elements[0] - elements[0] * other.elements[2]);
    result.elements[2] = scalar_utils::cleanZero(elements[0] * other.elements[1] - elements[1] * other.elements[0]);
    return result;
}

double Vector::normL2() const noexcept {
    double sumSquares = 0.0;
    for (const Scalar& val : elements) {
        double absVal = std::abs(val);
        sumSquares += absVal * absVal;
    }
    return std::sqrt(sumSquares);
}

double Vector::normMax() const noexcept {
    double maxVal = 0.0;
    for (const Scalar& val : elements) {
        maxVal = std::max(maxVal, std::abs(val));
    }
    return maxVal;
}

Vector Vector::normalized() const {
    double n = normL2();
    if (n <= scalar_utils::kEpsilon) {
        throw std::runtime_error("Unable to normalize 0 vector.");
    }
    Vector result(elements.size());
    for (std::size_t i = 0; i < elements.size(); ++i) {
        result.elements[i] = scalar_utils::cleanZero(elements[i] / n);
    }
    return result;
}

std::string Vector::toString(int precision) const {
    std::ostringstream oss;
    oss << "[";
    for (std::size_t i = 0; i < elements.size(); ++i) {
        oss << scalar_utils::formatScalar(elements[i], precision);
        if (i + 1 < elements.size()) {
            oss << ", ";
        }
    }
    oss << "]";
    return oss.str();
}

Vector operator*(const Scalar& scalar, const Vector& vec) {
    return vec * scalar;
}