#ifndef STORAGE_HPP
#define STORAGE_HPP

#include "Vector.hpp"
#include "Matrix.hpp"
#include <map>
#include <string>

/**
 * @brief Handles loading and reading of vectors and matricies from text file.
 * Format stores whole imaginary and real parts (double),to preserve accuracy.
 */
class Storage {
public:
    static bool saveWorkspace(
        const std::string& filePath,
        const std::map<std::string, Vector>& vectors,
        const std::map<std::string, Matrix>& matrices);

    static bool loadWorkspace(
        const std::string& filePath,
        std::map<std::string, Vector>& vectors,
        std::map<std::string, Matrix>& matrices,
        std::string& statusMessage);
};

#endif // STORAGE_HPP