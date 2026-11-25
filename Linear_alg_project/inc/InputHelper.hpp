#ifndef INPUT_HELPER_HPP
#define INPUT_HELPER_HPP

#include "Scalar.hpp"
#include <string>

/**
 * @brief Tools for safe reading form std input.
 * Protect from: inf loops, bad types, etc.
 */
class InputHelper {
public:
    /**
     * @brief Read Integer number in range [minVal, maxVal].
     */
    static int readIntInRange(const std::string& prompt, int minVal, int maxVal);

    /**
     * @brief Read double.
     */
    static double readDouble(const std::string& prompt);

    /**
     * @brief Read complex number(np. "3", "3 -2", "3+2i", "(3,-2)").
     */
    static Scalar readScalar(const std::string& prompt);

    /**
     * @brief Read not empty object identifier (eg. vector or matrix)
    */
    static std::string readIdentifier(const std::string& prompt);

    /**
     * @brief Read whole line of text
     */
    static std::string readLine(const std::string& prompt);
};

#endif // INPUT_HELPER_HPP