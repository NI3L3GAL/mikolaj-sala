#ifndef SCALAR_HPP
#define SCALAR_HPP

#include <complex>
#include <cmath>
#include <iostream>

/**
 * @brief Scalar type: double precision floating point complex number.
 * Real values are represented as complex numbers with Im() = 0.0
 */
using Scalar = std::complex<double>;

namespace scalar_utils {

constexpr double kEpsilon = 1e-9;

/**
 * @brief Checks if modulus of complex value is nearly 0 ~ really small epsilon
 */
inline bool isNearlyZero(const Scalar& value, double eps = kEpsilon) noexcept {
    return std::abs(value) <= eps;
}

/**
 * @brief Clearence of numerical error (eg. -0.0, eps > x).
 */
inline Scalar cleanZero(const Scalar& value, double eps = kEpsilon) noexcept {
    double re = (std::abs(value.real()) <= eps) ? 0.0 : value.real();
    double im = (std::abs(value.imag()) <= eps) ? 0.0 : value.imag();
    if (re == 0.0) {
        re = 0.0;
    }
    if (im == 0.0) {
        im = 0.0;
    }
    return Scalar(re, im);
}

/**
 * @brief Formats complex numbers in console or file
 */
inline std::string formatScalar(const Scalar& raw, int precision = 4) {
    Scalar v = cleanZero(raw);
    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss.precision(precision);

    if (std::abs(v.imag()) <= kEpsilon) {
        oss << v.real();
    } else {
        oss << "(" << v.real();
        if (v.imag() >= 0.0) {
            oss << "+" << v.imag() << "i)";
        } else {
            oss << "-" << std::abs(v.imag()) << "i)";
        }
    }
    return oss.str();
}

} // namespace scalar_utils

#endif // SCALAR_HPP