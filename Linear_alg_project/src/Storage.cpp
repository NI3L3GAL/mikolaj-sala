#include "Storage.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>

bool Storage::saveWorkspace(
    const std::string& filePath,
    const std::map<std::string, Vector>& vectors,
    const std::map<std::string, Matrix>& matrices) {
    std::ofstream out(filePath);
    if (!out.is_open()) {
        return false;
    }

    out << std::setprecision(17);
    out << "LINEAR_ALGEBRA_WORKSPACE_V1\n";

    out << "VECTORS " << vectors.size() << "\n";
    for (const auto& entry : vectors) {
        const std::string& name = entry.first;
        const Vector& vec = entry.second;
        out << "VECTOR " << name << " " << vec.size() << "\n";
        for (std::size_t i = 0; i < vec.size(); ++i) {
            Scalar v = vec.at(i);
            out << v.real() << " " << v.imag() << "\n";
        }
    }

    out << "MATRICES " << matrices.size() << "\n";
    for (const auto& entry : matrices) {
        const std::string& name = entry.first;
        const Matrix& mat = entry.second;
        out << "MATRIX " << name << " " << mat.rows() << " " << mat.cols() << "\n";
        for (std::size_t i = 0; i < mat.rows(); ++i) {
            for (std::size_t j = 0; j < mat.cols(); ++j) {
                Scalar v = mat.at(i, j);
                out << v.real() << " " << v.imag();
                if (j + 1 < mat.cols()) {
                    out << " ";
                }
            }
            out << "\n";
        }
    }

    return out.good();
}

bool Storage::loadWorkspace(
    const std::string& filePath,
    std::map<std::string, Vector>& vectors,
    std::map<std::string, Matrix>& matrices,
    std::string& statusMessage) {
    std::ifstream in(filePath);
    if (!in.is_open()) {
        statusMessage = "No state file found '" + filePath + "' - start with empty workspace.";
        return false;
    }

    std::string header;
    if (!std::getline(in, header) || header != "LINEAR_ALGEBRA_WORKSPACE_V1") {
        statusMessage = "File:'" + filePath + "'has error in the header - read was skipped.";
        return false;
    }

    std::map<std::string, Vector> loadedVectors;
    std::map<std::string, Matrix> loadedMatrices;

    std::string token;
    std::size_t vecCount = 0;
    if (!(in >> token >> vecCount) || token != "VECTORS") {
        statusMessage = "Section VECTORS is damaged in state file.";
        return false;
    }

    for (std::size_t k = 0; k < vecCount; ++k) {
        std::string kind;
        std::string name;
        std::size_t len = 0;
        if (!(in >> kind >> name >> len) || kind != "VECTOR") {
            statusMessage = "VECTOR: Has error in the header in state file.";
            return false;
        }
        std::vector<Scalar> elems;
        elems.reserve(len);
        for (std::size_t i = 0; i < len; ++i) {
            double re = 0.0;
            double im = 0.0;
            if (!(in >> re >> im) || !std::isfinite(re) || !std::isfinite(im)) {
                statusMessage = "VECTOR: Error occure reading element '" + name + "'.";
                return false;
            }
            elems.emplace_back(re, im);
        }
        loadedVectors[name] = Vector(elems);
    }

    std::size_t matCount = 0;
    if (!(in >> token >> matCount) || token != "MATRICES") {
        statusMessage = "Section MATRICES is damaged in state file.";
        return false;
    }

    for (std::size_t k = 0; k < matCount; ++k) {
        std::string kind;
        std::string name;
        std::size_t r = 0;
        std::size_t c = 0;
        if (!(in >> kind >> name >> r >> c) || kind != "MATRIX") {
            statusMessage = "MATRICES: Has error in the header in state file.";
            return false;
        }
        std::vector<std::vector<Scalar>> rowsData(r, std::vector<Scalar>(c, Scalar(0.0, 0.0)));
        for (std::size_t i = 0; i < r; ++i) {
            for (std::size_t j = 0; j < c; ++j) {
                double re = 0.0;
                double im = 0.0;
                if (!(in >> re >> im) || !std::isfinite(re) || !std::isfinite(im)) {
                    statusMessage = "MATRICES: Error occure reading element '" + name + "'.";
                    return false;
                }
                rowsData[i][j] = Scalar(re, im);
            }
        }
        loadedMatrices[name] = Matrix(rowsData);
    }

    vectors = std::move(loadedVectors);
    matrices = std::move(loadedMatrices);
    statusMessage = "State read from '" + filePath + "' (vectors: " +
                    std::to_string(vectors.size()) + ", matrices: " +
                    std::to_string(matrices.size()) + ").";
    return true;
}