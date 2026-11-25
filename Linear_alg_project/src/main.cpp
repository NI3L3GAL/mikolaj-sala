#include "Scalar.hpp"
#include "Vector.hpp"
#include "Matrix.hpp"
#include "AlgEntity.hpp"
#include "GaussianSolver.hpp"
#include "LUSolver.hpp"
#include "Storage.hpp"
#include "InputHelper.hpp"

#include <iostream>
#include <iomanip>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace {

constexpr const char* kStateFile = "workspace_state.txt";

void printVectorMap(const std::map<std::string, Vector>& vectors) {
    std::cout << "\n--- Zapisane wektory (" << vectors.size() << ") ---\n";
    if (vectors.empty()) {
        std::cout << "(brak wektorow)\n";
        return;
    }
    for (const auto& entry : vectors) {
        std::cout << "  " << entry.first << " [dim=" << entry.second.size() << "] = "
                  << entry.second.toString() << "\n";
    }
}

void printMatrixMap(const std::map<std::string, Matrix>& matrices) {
    std::cout << "\n--- Zapisane macierze (" << matrices.size() << ") ---\n";
    if (matrices.empty()) {
        std::cout << "(brak macierzy)\n";
        return;
    }
    for (const auto& entry : matrices) {
        std::cout << "  " << entry.first << " [" << entry.second.rows()
                  << "x" << entry.second.cols() << "]:\n"
                  << entry.second.toString();
    }
}

bool pickVectorName(const std::map<std::string, Vector>& vectors, const std::string& label, std::string& outName) {
    if (vectors.empty()) {
        std::cout << "[Info] Brak zapisanych wektorow. Najpierw utworz wektor.\n";
        return false;
    }
    printVectorMap(vectors);
    while (true) {
        std::string name = InputHelper::readIdentifier("Podaj nazwe wektora (" + label + ") lub wpisz 'cancel': ");
        if (name == "cancel") {
            return false;
        }
        if (vectors.find(name) != vectors.end()) {
            outName = name;
            return true;
        }
        std::cout << "[Blad] Nie znaleziono wektora o nazwie '" << name << "'. Sprobuj ponownie.\n";
    }
}

bool pickMatrixName(const std::map<std::string, Matrix>& matrices, const std::string& label, std::string& outName) {
    if (matrices.empty()) {
        std::cout << "[Info] Brak zapisanych macierzy. Najpierw utworz macierz.\n";
        return false;
    }
    printMatrixMap(matrices);
    while (true) {
        std::string name = InputHelper::readIdentifier("Podaj nazwe macierzy (" + label + ") lub wpisz 'cancel': ");
        if (name == "cancel") {
            return false;
        }
        if (matrices.find(name) != matrices.end()) {
            outName = name;
            return true;
        }
        std::cout << "[Blad] Nie znaleziono macierzy o nazwie '" << name << "'. Sprobuj ponownie.\n";
    }
}

void createOrEditVector(std::map<std::string, Vector>& vectors) {
    std::string name = InputHelper::readIdentifier("Nazwa wektora: ");
    int dim = InputHelper::readIntInRange("Wymiar wektora (1..32): ", 1, 32);
    std::vector<Scalar> values;
    values.reserve(static_cast<std::size_t>(dim));
    for (int i = 0; i < dim; ++i) {
        Scalar v = InputHelper::readScalar("  element [" + std::to_string(i) + "]: ");
        values.push_back(v);
    }
    vectors[name] = Vector(values);
    std::cout << "[OK] Zapisano wektor '" << name << "' = " << vectors[name].toString() << "\n";
}

void createOrEditMatrix(std::map<std::string, Matrix>& matrices) {
    std::string name = InputHelper::readIdentifier("Nazwa macierzy: ");
    int r = InputHelper::readIntInRange("Liczba wierszy (1..16): ", 1, 16);
    int c = InputHelper::readIntInRange("Liczba kolumn (1..16): ", 1, 16);
    std::vector<std::vector<Scalar>> grid(
        static_cast<std::size_t>(r),
        std::vector<Scalar>(static_cast<std::size_t>(c), Scalar(0.0, 0.0)));

    for (int i = 0; i < r; ++i) {
        for (int j = 0; j < c; ++j) {
            grid[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] =
                InputHelper::readScalar("  element [" + std::to_string(i) + "][" + std::to_string(j) + "]: ");
        }
    }
    matrices[name] = Matrix(grid);
    std::cout << "[OK] Zapisano macierz '" << name << "':\n" << matrices[name].toString();
}

void vectorSubmenu(std::map<std::string, Vector>& vectors) {
    while (true) {
        std::cout << "\n================ PODMENU: WEKTORY ================\n";
        std::cout << "1. Dodaj / nadpisz wektor\n";
        std::cout << "2. Wyswietl wszystkie wektory\n";
        std::cout << "3. Dodawanie i odejmowanie wektorow (u + v, u - v)\n";
        std::cout << "4. Mnozenie przez skalar\n";
        std::cout << "5. Iloczyn skalarny (hermitowski) oraz wektorowy (3D)\n";
        std::cout << "6. Normy (L2, L-inf) oraz normalizacja\n";
        std::cout << "0. Powrot do menu glownego\n";

        int choice = InputHelper::readIntInRange("Wybor: ", 0, 6);
        if (choice == 0) {
            return;
        }

        try {
            if (choice == 1) {
                createOrEditVector(vectors);
            } else if (choice == 2) {
                printVectorMap(vectors);
            } else if (choice == 3) {
                std::string aName, bName;
                if (!pickVectorName(vectors, "pierwszy wektor", aName) ||
                    !pickVectorName(vectors, "drugi wektor", bName)) {
                    continue;
                }
                Vector sum = vectors.at(aName) + vectors.at(bName);
                Vector diff = vectors.at(aName) - vectors.at(bName);
                std::cout << aName << " + " << bName << " = " << sum.toString() << "\n";
                std::cout << aName << " - " << bName << " = " << diff.toString() << "\n";
            } else if (choice == 4) {
                std::string vName;
                if (!pickVectorName(vectors, "wektor", vName)) {
                    continue;
                }
                Scalar s = InputHelper::readScalar("Podaj skalar: ");
                Vector scaled = vectors.at(vName) * s;
                std::cout << scalar_utils::formatScalar(s) << " * " << vName << " = " << scaled.toString() << "\n";
            } else if (choice == 5) {
                std::string aName, bName;
                if (!pickVectorName(vectors, "pierwszy wektor", aName) ||
                    !pickVectorName(vectors, "drugi wektor", bName)) {
                    continue;
                }
                Scalar dot = vectors.at(aName).dotProduct(vectors.at(bName));
                std::cout << "Iloczyn skalarny <" << aName << ", " << bName << "> = "
                          << scalar_utils::formatScalar(dot) << "\n";
                if (vectors.at(aName).size() == 3 && vectors.at(bName).size() == 3) {
                    Vector cross = vectors.at(aName).crossProduct3D(vectors.at(bName));
                    std::cout << "Iloczyn wektorowy " << aName << " x " << bName << " = "
                              << cross.toString() << "\n";
                } else {
                    std::cout << "[Info] Iloczyn wektorowy dostepny dla wektorow 3D.\n";
                }
            } else if (choice == 6) {
                std::string vName;
                if (!pickVectorName(vectors, "wektor", vName)) {
                    continue;
                }
                const Vector& v = vectors.at(vName);
                std::cout << std::fixed << std::setprecision(4);
                std::cout << "Norma L2 (euklidesowa) = " << v.normL2() << "\n";
                std::cout << "Norma L-inf (maksimum) = " << v.normMax() << "\n";
                Vector unit = v.normalized();
                std::cout << "Wektor znormalizowany = " << unit.toString() << "\n";
            }
        } catch (const std::exception& ex) {
            std::cout << "[Blad operacji] " << ex.what() << "\n";
        }
    }
}

void matrixSubmenu(std::map<std::string, Vector>& vectors, std::map<std::string, Matrix>& matrices) {
    while (true) {
        std::cout << "\n================ PODMENU: MACIERZE ================\n";
        std::cout << "1. Dodaj / nadpisz macierz\n";
        std::cout << "2. Wyswietl wszystkie macierze\n";
        std::cout << "3. Dodawanie i odejmowanie macierzy (A + B, A - B)\n";
        std::cout << "4. Mnozenie macierzy: macierz * macierz, macierz * wektor, macierz * skalar\n";
        std::cout << "5. Transpozycja, sprzezenie hermitowskie i slad macierzy\n";
        std::cout << "6. Wyznacznik (rekurencyjny Laplace'a oraz eliminacja)\n";
        std::cout << "7. Macierz odwrotna (A^-1)\n";
        std::cout << "0. Powrot do menu glownego\n";

        int choice = InputHelper::readIntInRange("Wybor: ", 0, 7);
        if (choice == 0) {
            return;
        }

        try {
            if (choice == 1) {
                createOrEditMatrix(matrices);
            } else if (choice == 2) {
                printMatrixMap(matrices);
            } else if (choice == 3) {
                std::string aName, bName;
                if (!pickMatrixName(matrices, "macierz A", aName) ||
                    !pickMatrixName(matrices, "macierz B", bName)) {
                    continue;
                }
                Matrix sum = matrices.at(aName) + matrices.at(bName);
                Matrix diff = matrices.at(aName) - matrices.at(bName);
                std::cout << "Wynik " << aName << " + " << bName << ":\n" << sum.toString();
                std::cout << "Wynik " << aName << " - " << bName << ":\n" << diff.toString();
            } else if (choice == 4) {
                std::cout << "  4a. Macierz * Macierz\n";
                std::cout << "  4b. Macierz * Wektor\n";
                std::cout << "  4c. Macierz * Skalar\n";
                std::cout << "Wybierz tryb (1=A*B, 2=A*v, 3=A*s): ";
                int mode = InputHelper::readIntInRange("", 1, 3);
                std::string aName;
                if (!pickMatrixName(matrices, "macierz A", aName)) {
                    continue;
                }
                if (mode == 1) {
                    std::string bName;
                    if (!pickMatrixName(matrices, "macierz B", bName)) {
                        continue;
                    }
                    Matrix prod = matrices.at(aName) * matrices.at(bName);
                    std::cout << "Wynik " << aName << " * " << bName << ":\n" << prod.toString();
                } else if (mode == 2) {
                    std::string vName;
                    if (!pickVectorName(vectors, "wektor v", vName)) {
                        continue;
                    }
                    Vector resVec = matrices.at(aName) * vectors.at(vName);
                    std::cout << "Wynik " << aName << " * " << vName << " = " << resVec.toString() << "\n";
                } else {
                    Scalar s = InputHelper::readScalar("Podaj skalar: ");
                    Matrix scaled = matrices.at(aName) * s;
                    std::cout << "Wynik " << aName << " * " << scalar_utils::formatScalar(s) << ":\n"
                              << scaled.toString();
                }
            } else if (choice == 5) {
                std::string aName;
                if (!pickMatrixName(matrices, "macierz", aName)) {
                    continue;
                }
                const Matrix& m = matrices.at(aName);
                std::cout << "Transpozycja (" << aName << "^T):\n" << m.transpose().toString();
                std::cout << "Sprzezenie hermitowskie (" << aName << "^H):\n"
                          << m.conjugateTranspose().toString();
                if (m.isSquare()) {
                    std::cout << "Slad tr(" << aName << ") = " << scalar_utils::formatScalar(m.trace()) << "\n";
                } else {
                    std::cout << "[Info] Slad jest zdefiniowany tylko dla macierzy kwadratowych.\n";
                }
            } else if (choice == 6) {
                std::string aName;
                if (!pickMatrixName(matrices, "macierz kwadratowa", aName)) {
                    continue;
                }
                const Matrix& m = matrices.at(aName);
                Scalar detElim = m.determinantElimination();
                std::cout << "Wyznacznik (eliminacja z pivotingiem) det(" << aName << ") = "
                          << scalar_utils::formatScalar(detElim) << "\n";
                try {
                    Scalar detRec = m.determinantRecursive();
                    std::cout << "Wyznacznik (rekurencyjnie Laplace)   det(" << aName << ") = "
                              << scalar_utils::formatScalar(detRec) << "\n";
                } catch (const std::domain_error& ex) {
                    std::cout << "[Info] " << ex.what() << "\n";
                }
            } else if (choice == 7) {
                std::string aName;
                if (!pickMatrixName(matrices, "macierz kwadratowa nieosobliwa", aName)) {
                    continue;
                }
                Matrix inv = matrices.at(aName).inverse();
                std::cout << "Macierz odwrotna (" << aName << "^-1):\n" << inv.toString();
            }
        } catch (const std::exception& ex) {
            std::cout << "[Blad operacji] " << ex.what() << "\n";
        }
    }
}

void solverSubmenu(
    const std::map<std::string, Vector>& vectors,
    const std::map<std::string, Matrix>& matrices,
    const std::vector<std::unique_ptr<AlgEntity>>& solvers) {
    while (true) {
        std::cout << "\n============= PODMENU: UKLADY ROWNAN (Ax = b) =============\n";
        for (std::size_t i = 0; i < solvers.size(); ++i) {
            std::cout << (i + 1) << ". Rozwiaz uklad uzywajac: " << solvers[i]->getName() << "\n";
        }
        std::cout << "3. Pokaz rozklad PA = LU macierzy A\n";
        std::cout << "0. Powrot do menu glownego\n";

        int choice = InputHelper::readIntInRange("Wybor: ", 0, 3);
        if (choice == 0) {
            return;
        }

        try {
            std::string aName;
            if (!pickMatrixName(matrices, "macierz wspolczynnikow A", aName)) {
                continue;
            }
            const Matrix& matA = matrices.at(aName);

            if (choice == 3) {
                LUSolver luEngine;
                LUSolver::DecompositionResult decomp = luEngine.decompose(matA.data());
                Matrix matL(decomp.lower);
                Matrix matU(decomp.upper);
                std::cout << "\nMacierz L (dolnotrojkatna):\n" << matL.toString();
                std::cout << "Macierz U (gornotrojkatna):\n" << matU.toString();
                std::cout << "Wektor permutacji wierszy P: [";
                for (std::size_t i = 0; i < decomp.permutation.size(); ++i) {
                    std::cout << decomp.permutation[i];
                    if (i + 1 < decomp.permutation.size()) {
                        std::cout << ", ";
                    }
                }
                std::cout << "]\n";
                continue;
            }

            std::string bName;
            if (!pickVectorName(vectors, "wektor wyrazow wolnych b", bName)) {
                continue;
            }
            const Vector& vecB = vectors.at(bName);

            const AlgEntity& solver = *solvers[static_cast<std::size_t>(choice - 1)];
            std::vector<Scalar> solValues = solver.solve(matA.data(), vecB.data());
            Vector solVec(solValues);

            std::cout << "\n[Sukces] Metoda: " << solver.getName() << "\n";
            std::cout << "Rozwiazanie x = " << solVec.toString() << "\n";

            // Weryfikacja residuum ||A*x - b||_2
            Vector residual = (matA * solVec) - vecB;
            std::cout << std::fixed << std::setprecision(6);
            std::cout << "Blad residuum ||A*x - b||_2 = " << residual.normL2() << "\n";
        } catch (const std::exception& ex) {
            std::cout << "[Blad rozwiazywania] " << ex.what() << "\n";
        }
    }
}

void storageSubmenu(
    std::map<std::string, Vector>& vectors,
    std::map<std::string, Matrix>& matrices) {
    while (true) {
        std::cout << "\n================ PODMENU: ZAPIS I ODCZYT ================\n";
        std::cout << "1. Zapisz aktualny stan do pliku (" << kStateFile << ")\n";
        std::cout << "2. Wczytaj stan z pliku (" << kStateFile << ")\n";
        std::cout << "3. Wyczysc biezace wektory i macierze w pamieci\n";
        std::cout << "0. Powrot do menu glownego\n";

        int choice = InputHelper::readIntInRange("Wybor: ", 0, 3);
        if (choice == 0) {
            return;
        }
        if (choice == 1) {
            if (Storage::saveWorkspace(kStateFile, vectors, matrices)) {
                std::cout << "[OK] Zapisano stan do '" << kStateFile << "'.\n";
            } else {
                std::cout << "[Blad] Nie udalo sie zapisac pliku '" << kStateFile << "'.\n";
            }
        } else if (choice == 2) {
            std::string msg;
            Storage::loadWorkspace(kStateFile, vectors, matrices, msg);
            std::cout << "[Info] " << msg << "\n";
        } else if (choice == 3) {
            vectors.clear();
            matrices.clear();
            std::cout << "[OK] Wyczysczono pamiec robocza.\n";
        }
    }
}

} // namespace

int main() {
    std::map<std::string, Vector> vectors;
    std::map<std::string, Matrix> matrices;

    // Polimorficzna kolekcja solwerow zarzadzana przez smart wskazniki (bez new/delete)
    std::vector<std::unique_ptr<AlgEntity>> solvers;
    solvers.push_back(std::make_unique<GaussianSolver>());
    solvers.push_back(std::make_unique<LUSolver>());

    // Automatyczne odtworzenie stanu po uruchomieniu programu
    std::string loadStatus;
    Storage::loadWorkspace(kStateFile, vectors, matrices, loadStatus);
    std::cout << "=== ALGEBRA LINIOWA (double + liczby zespolone) ===\n";
    std::cout << "[Start] " << loadStatus << "\n";

    while (true) {
        std::cout << "\n==================== MENU GLOWNE ====================\n";
        std::cout << "1. Operacje na wektorach\n";
        std::cout << "2. Operacje na macierzach (det, odwrotnosc, *, +, -, transpozycja)\n";
        std::cout << "3. Rozwiazywanie ukladow rownan Ax = b (Gauss / Rozklad LU)\n";
        std::cout << "4. Zarzadzanie plikiem stanu (zapis / odczyt)\n";
        std::cout << "0. Zapisz stan i zakoncz program\n";

        int mainChoice = InputHelper::readIntInRange("Wybor: ", 0, 4);
        if (mainChoice == 0) {
            if (Storage::saveWorkspace(kStateFile, vectors, matrices)) {
                std::cout << "[OK] Stan zapisany do '" << kStateFile << "'. Do zobaczenia!\n";
            } else {
                std::cout << "[Uwaga] Nie udalo sie zapisac pliku '" << kStateFile << "' przy wyjsciu.\n";
            }
            break;
        }

        switch (mainChoice) {
            case 1:
                vectorSubmenu(vectors);
                break;
            case 2:
                matrixSubmenu(vectors, matrices);
                break;
            case 3:
                solverSubmenu(vectors, matrices, solvers);
                break;
            case 4:
                storageSubmenu(vectors, matrices);
                break;
            default:
                break;
        }
    }

    return 0;
}