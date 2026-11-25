#include "InputHelper.hpp"
#include <iostream>
#include <sstream>
#include <limits>
#include <cctype>
#include <algorithm>

namespace {

std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }
    return s.substr(start, end - start);
}

bool readRawLine(const std::string& prompt, std::string& outLine) {
    while (true) {
        std::cout << prompt;
        if (!std::getline(std::cin, outLine)) {
            if (std::cin.eof()) {
                std::cin.clear();
                std::cout << "\n[Warning] End of file detected(EOF).Write data again.\n";
                continue;
            }
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "[Error] Error with input stream.Try again.\n";
            continue;
        }
        return true;
    }
}

/**
 * @brief Parser for reading complex number:
 * eg. : "5", "-2.5", "3 -4", "(3, -4)", "2+3i", "-i", "+2i", "1-2j".
 */
bool parseComplexString(const std::string& rawInput, Scalar& result) {
    std::string s = trim(rawInput);
    if (s.empty()) {
        return false;
    }

    // Trim non essential parenthesis "(a, b)" lub "(a+bi)"
    if (s.front() == '(' && s.back() == ')' && s.size() >= 2) {
        s = trim(s.substr(1, s.size() - 2));
        if (s.empty()) {
            return false;
        }
        auto commaPos = s.find(',');
        if (commaPos != std::string::npos) {
            if (s.find(',', commaPos + 1) != std::string::npos) {
                return false;
            }
            std::string reStr = trim(s.substr(0, commaPos));
            std::string imStr = trim(s.substr(commaPos + 1));
            char* endRe = nullptr;
            char* endIm = nullptr;
            double re = std::strtod(reStr.c_str(), &endRe);
            double im = std::strtod(imStr.c_str(), &endIm);
            if (reStr.empty() || imStr.empty() || *endRe != '\0' || *endIm != '\0' ||
                !std::isfinite(re) || !std::isfinite(im)) {
                return false;
            }
            result = Scalar(re, im);
            return true;
        }
    }

    // two numbers separated with space eg. "3.5 -2.0"
    {
        std::istringstream iss(s);
        double re = 0.0;
        double im = 0.0;
        std::string extra;
        if ((iss >> re >> im) && !(iss >> extra)) {
            if (std::isfinite(re) && std::isfinite(im)) {
                result = Scalar(re, im);
                return true;
            }
        }
    }

    // Singular number eg. "42" lub "-3.14"
    {
        char* endPtr = nullptr;
        double val = std::strtod(s.c_str(), &endPtr);
        if (*endPtr == '\0' && std::isfinite(val)) {
            result = Scalar(val, 0.0);
            return true;
        }
    }


    // Algebraic form with 'i' or 'j' eg. "3+4i", "2-i", "+i", "-i", "5i"
    char lastChar = static_cast<char>(std::tolower(static_cast<unsigned char>(s.back())));
    if (lastChar == 'i' || lastChar == 'j') {
        std::string body = trim(s.substr(0, s.size() - 1));

        std::string compactBody;
        compactBody.reserve(body.size());
        for (char ch : body) {
            if (!std::isspace(static_cast<unsigned char>(ch))) {
                compactBody.push_back(ch);
            }
        }

        if (compactBody.empty() || compactBody == "+") {
            result = Scalar(0.0, 1.0);
            return true;
        }
        if (compactBody == "-") {
            result = Scalar(0.0, -1.0);
            return true;
        }

        // Seek for last char '+' lub '-' (not at pos 0,there can be sign of real part)
        size_t splitPos = std::string::npos;
        for (size_t i = 1; i < compactBody.size(); ++i) {
            if (compactBody[i] == '+' || compactBody[i] == '-') {
                splitPos = i;
            }
        }

        if (splitPos == std::string::npos) {
            // only imaginary part, eg. "4i" lub "-2.5i"
            char* endIm = nullptr;
            double im = std::strtod(compactBody.c_str(), &endIm);
            if (*endIm == '\0' && std::isfinite(im)) {
                result = Scalar(0.0, im);
                return true;
            }
            return false;
        } else {
            std::string rePart = trim(compactBody.substr(0, splitPos));
            std::string imPart = trim(compactBody.substr(splitPos));
            if (imPart == "+") {
                imPart = "1.0";
            } else if (imPart == "-") {
                imPart = "-1.0";
            }

            char* endRe = nullptr;
            char* endIm = nullptr;
            double re = std::strtod(rePart.c_str(), &endRe);
            double im = std::strtod(imPart.c_str(), &endIm);
            if (rePart.empty() || imPart.empty() || *endRe != '\0' || *endIm != '\0' ||
                !std::isfinite(re) || !std::isfinite(im)) {
                return false;
            }
            result = Scalar(re, im);
            return true;
        }
    }

    return false;
}

} // namespace

int InputHelper::readIntInRange(const std::string& prompt, int minVal, int maxVal) {
    while (true) {
        std::string line;
        readRawLine(prompt, line);
        std::string trimmed = trim(line);
        if (trimmed.empty()) {
            std::cout << "[Error] Blank entry. Enter Integer from range ["
                      << minVal << ", " << maxVal << "].\n";
            continue;
        }

        char* endPtr = nullptr;
        long parsed = std::strtol(trimmed.c_str(), &endPtr, 10);
        if (*endPtr != '\0' || parsed < static_cast<long>(minVal) ||
            parsed > static_cast<long>(maxVal)) {
            std::cout << "[Error] Invalid value. Enter Integer from range ["
                      << minVal << ", " << maxVal << "].\n";
            continue;
        }
        return static_cast<int>(parsed);
    }
}

double InputHelper::readDouble(const std::string& prompt) {
    while (true) {
        std::string line;
        readRawLine(prompt, line);
        std::string trimmed = trim(line);
        if (trimmed.empty()) {
            std::cout << "[Error] Invalid entry. Enter floating point number.\n";
            continue;
        }

        char* endPtr = nullptr;
        double val = std::strtod(trimmed.c_str(), &endPtr);
        if (*endPtr != '\0' || !std::isfinite(val)) {
            std::cout << "[Error] Invalid number. Try again (eq. 2.5 or -1.0).\n";
            continue;
        }
        return val;
    }
}

Scalar InputHelper::readScalar(const std::string& prompt) {
    while (true) {
        std::string line;
        readRawLine(prompt, line);
        Scalar parsed;
        if (!parseComplexString(line, parsed)) {
            std::cout << "[Error] Invalid number format. Enter eq. '5', '-2.25', '3 -4' or '2+3i'.\n";
            continue;
        }
        return scalar_utils::cleanZero(parsed);
    }
}

std::string InputHelper::readIdentifier(const std::string& prompt) {
    while (true) {
        std::string line;
        readRawLine(prompt, line);
        std::string name = trim(line);
        if (name.empty()) {
            std::cout << "[Error] Identifier cannot be empty.\n";
            continue;
        }
        bool valid = true;
        for (char ch : name) {
            unsigned char uch = static_cast<unsigned char>(ch);
            if (!std::isalnum(uch) && ch != '_') {
                valid = false;
                break;
            }
        }
        if (!valid) {
            std::cout << "[Error] Use only letters, numbers or underscore '_'.\n";
            continue;
        }
        return name;
    }
}

std::string InputHelper::readLine(const std::string& prompt) {
    std::string line;
    readRawLine(prompt, line);
    return line;
}