#include "Scalar.hpp"
#include "Vector.hpp"
#include "Matrix.hpp"
#include "AlgEntity.hpp"
#include "GaussianSolver.hpp"
#include "LUSolver.hpp"
#include "Storage.hpp"

#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <memory>
#include <string>
#include <cmath>
#include <stdexcept>
#include <cstdio>

namespace {

int g_passed = 0;
int g_failed = 0;

bool approxEqual(const Scalar& a, const Scalar& b, double tol = 1e-7) {
    return std::abs(a - b) <= tol;
}

bool approxEqualDouble(double a, double b, double tol = 1e-7) {
    return std::abs(a - b) <= tol;
}

void check(bool condition, const std::string& testName) {
    if (condition) {
        ++g_passed;
        std::cout << "[PASS] " << testName << "\n";
    } else {
        ++g_failed;
        std::cout << "[FAIL] " << testName << "\n";
    }
}

template <typename Func>
bool expectThrows(Func&& fn) {
    try {
        fn();
        return false;
    } catch (const std::exception&) {
        return true;
    }
}

void testVectors() {
    std::cout << "\n=== TESTY: Vector (zespolone + normy + iloczyny) ===\n";

    Vector u({Scalar(1.0, 2.0), Scalar(3.0, -1.0)});
    Vector v({Scalar(2.0, -1.0), Scalar(-1.0, 4.0)});

    Vector sum = u + v;
    check(approxEqual(sum.at(0), Scalar(3.0, 1.0)) &&
          approxEqual(sum.at(1), Scalar(2.0, 3.0)),
          "Dodawanie wektorow zespolonych (u + v)");

    Vector diff = u - v;
    check(approxEqual(diff.at(0), Scalar(-1.0, 3.0)) &&
          approxEqual(diff.at(1), Scalar(4.0, -5.0)),
          "Odejmowanie wektorow zespolonych (u - v)");

    // Iloczyn hermitowski: conj(1+2i)*(2-i) + conj(3-i)*(-1+4i) = (1-2i)(2-i) + (3+i)(-1+4i) = -7 - 16i
    Scalar dot = u.dotProduct(v);
    check(approxEqual(dot, Scalar(-7.0, -6.0)),
          "Iloczyn skalarny hermitowski <u, v>");

    // Norma L2 dla [3, 4] = 5.0
    Vector w({Scalar(3.0, 0.0), Scalar(4.0, 0.0)});
    check(approxEqualDouble(w.normL2(), 5.0),
          "Norma L2 wektora [3, 4] == 5.0");

    // Iloczyn wektorowy 3D: [1, 0, 0] x [0, 1, 0] = [0, 0, 1]
    Vector e1({Scalar(1.0, 0.0), Scalar(0.0, 0.0), Scalar(0.0, 0.0)});
    Vector e2({Scalar(0.0, 0.0), Scalar(1.0, 0.0), Scalar(0.0, 0.0)});
    Vector e3 = e1.crossProduct3D(e2);
    check(approxEqual(e3.at(0), Scalar(0.0, 0.0)) &&
          approxEqual(e3.at(1), Scalar(0.0, 0.0)) &&
          approxEqual(e3.at(2), Scalar(1.0, 0.0)),
          "Iloczyn wektorowy 3D (e1 x e2 == e3)");

    Vector wrongDim({Scalar(1.0, 0.0)});
    check(expectThrows([&]() { (void)(u + wrongDim); }),
          "Wyjatek przy dodawaniu wektorow o roznych wymiarach");
    check(expectThrows([&]() { (void)e1.crossProduct3D(wrongDim); }),
          "Wyjatek przy iloczynie wektorowym dla wymiaru != 3");
}

void testMatrices() {
    std::cout << "\n=== TESTY: Matrix (det, odwrotnosc, mnozenie, wyjatki) ===\n";

    // Macierz zespolona 2x2:
    // [ 1+i   2   ]
    // [ 3     1-i ]
    // det = (1+i)(1-i) - 2*3 = (1 + 1) - 6 = -4
    Matrix m({
        {Scalar(1.0, 1.0), Scalar(2.0, 0.0)},
        {Scalar(3.0, 0.0), Scalar(1.0, -1.0)}
    });

    Scalar detRec = m.determinantRecursive();
    Scalar detElim = m.determinantElimination();
    check(approxEqual(detRec, Scalar(-4.0, 0.0)),
          "Wyznacznik rekurencyjny (Laplace) dla macierzy zespolonej 2x2");
    check(approxEqual(detElim, Scalar(-4.0, 0.0)),
          "Wyznacznik eliminacja z pivotingiem dla macierzy zespolonej 2x2");

    // Sprawdzenie A * A^(-1) == I
    Matrix inv = m.inverse();
    Matrix prod = m * inv;
    Matrix ident = Matrix::identity(2);
    bool isIdentity = true;
    for (std::size_t i = 0; i < 2; ++i) {
        for (std::size_t j = 0; j < 2; ++j) {
            if (!approxEqual(prod.at(i, j), ident.at(i, j))) {
                isIdentity = false;
            }
        }
    }
    check(isIdentity, "Ilosc macierzy A * A^(-1) daje macierz jednostkowa I");

    // Macierz osobliwa (wiersze proporcjonalne)
    Matrix singular({
        {Scalar(1.0, 0.0), Scalar(2.0, 0.0)},
        {Scalar(2.0, 0.0), Scalar(4.0, 0.0)}
    });
    check(approxEqual(singular.determinantElimination(), Scalar(0.0, 0.0)),
          "Wyznacznik macierzy osobliwej == 0");
    check(expectThrows([&]() { (void)singular.inverse(); }),
          "Wyjatek przy probie odwrocenia macierzy osobliwej");

    // Limit rekurencji Laplace'a (> 9)
    Matrix bigMat(10, 10, Scalar(1.0, 0.0));
    check(expectThrows([&]() { (void)bigMat.determinantRecursive(); }),
          "Wyjatek przy probie wyznacznika rekurencyjnego dla macierzy 10x10");
}

void testSolvers() {
    std::cout << "\n=== TESTY: Solvery (GaussianSolver oraz LUSolver przez AlgEntity) ===\n";

    // Uklad:
    // (2 + i)x + (1 - i)y = 5 + 2i
    // (1 + 2i)x + (3 - i)y = 4 + 7i
    std::vector<std::vector<Scalar>> matA = {
        {Scalar(2.0, 1.0), Scalar(1.0, -1.0)},
        {Scalar(1.0, 2.0), Scalar(3.0, -1.0)}
    };
    std::vector<Scalar> vecB = {
        Scalar(5.0, 2.0),
        Scalar(4.0, 7.0)
    };

    std::vector<std::unique_ptr<AlgEntity>> solvers;
    solvers.push_back(std::make_unique<GaussianSolver>());
    solvers.push_back(std::make_unique<LUSolver>());

    Matrix A(matA);
    Vector b(vecB);

    for (const auto& solverPtr : solvers) {
        std::vector<Scalar> sol = solverPtr->solve(matA, vecB);
        Vector x(sol);
        double residualNorm = ((A * x) - b).normL2();
        check(residualNorm < 1e-7,
              "Rozwiazanie ukladu Ax = b dla solwera: " + solverPtr->getName());
    }

    // Sprawdzenie rozkladu PA = LU (weryfikacja residuum ||P*A - L*U||_F)
    LUSolver lu;
    LUSolver::DecompositionResult decomp = lu.decompose(matA);
    const std::size_t n = matA.size();
    Matrix L(decomp.lower);
    Matrix U(decomp.upper);
    Matrix LU = L * U;

    double maxDecompErr = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        std::size_t permRow = decomp.permutation[i];
        for (std::size_t j = 0; j < n; ++j) {
            double err = std::abs(matA[permRow][j] - LU.at(i, j));
            if (err > maxDecompErr) {
                maxDecompErr = err;
            }
        }
    }
    check(maxDecompErr < 1e-7, "Poprawnosc rozkladu PA = L * U");
}

void testStorageRoundtrip() {
    std::cout << "\n=== TESTY: Storage (zapis i odczyt stanu z pliku) ===\n";

    const std::string tempFile = "test_workspace_state.tmp";
    std::map<std::string, Vector> outVectors;
    std::map<std::string, Matrix> outMatrices;

    outVectors["v_test"] = Vector({Scalar(1.5, -2.25), Scalar(0.0, 4.0)});
    outMatrices["m_test"] = Matrix({
        {Scalar(2.0, 1.0), Scalar(-3.5, 0.0)},
        {Scalar(0.0, -1.0), Scalar(5.25, 2.5)}
    });

    bool saved = Storage::saveWorkspace(tempFile, outVectors, outMatrices);
    check(saved, "Zapis stanu do pliku tymczasowego");

    std::map<std::string, Vector> inVectors;
    std::map<std::string, Matrix> inMatrices;
    std::string statusMsg;
    bool loaded = Storage::loadWorkspace(tempFile, inVectors, inMatrices, statusMsg);
    check(loaded, "Odczyt stanu z pliku tymczasowego (" + statusMsg + ")");

    bool vectorsMatch = (inVectors.count("v_test") == 1) &&
                        (inVectors.at("v_test").size() == 2) &&
                        approxEqual(inVectors.at("v_test").at(0), Scalar(1.5, -2.25)) &&
                        approxEqual(inVectors.at("v_test").at(1), Scalar(0.0, 4.0));

    bool matricesMatch = (inMatrices.count("m_test") == 1) &&
                         (inMatrices.at("m_test").rows() == 2) &&
                         (inMatrices.at("m_test").cols() == 2) &&
                         approxEqual(inMatrices.at("m_test").at(0, 0), Scalar(2.0, 1.0)) &&
                         approxEqual(inMatrices.at("m_test").at(1, 1), Scalar(5.25, 2.5));

    check(vectorsMatch && matricesMatch,
          "Zgodnosc danych zespolonych po zapisie i ponownym wczytaniu");

    // Sprzatanie pliku tymczasowego
    std::remove(tempFile.c_str());
}

} // namespace

int main() {
    std::cout << "=== URUCHAMIANIE TESTBENCHU PROJEKTU ALGEBRY LINIOWEJ ===\n";
    testVectors();
    testMatrices();
    testSolvers();
    testStorageRoundtrip();

    std::cout << "\nPodsumowanie: zaliczone = " << g_passed
              << ", bledy = " << g_failed << "\n";
    return (g_failed == 0) ? 0 : 1;
}