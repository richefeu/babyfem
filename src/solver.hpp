#pragma once
#include "matrix.hpp"
#include <cmath>
#include <algorithm>

class GaussSolver {
public:
    // Résout K*u = F avec élimination Gauss et pivot partiel
    static Vector solve(Matrix K, Vector F) {
        int n = K.rows();
        if (K.cols() != n || F.size() != n) {
            throw std::invalid_argument("Dimension mismatch");
        }

        // Forward elimination avec pivot partiel
        for (int col = 0; col < n; ++col) {
            // Trouver le pivot
            int pivot_row = col;
            double max_val = std::abs(K(col, col));

            for (int row = col + 1; row < n; ++row) {
                if (std::abs(K(row, col)) > max_val) {
                    max_val = std::abs(K(row, col));
                    pivot_row = row;
                }
            }

            if (std::abs(max_val) < 1e-14) {
                throw std::runtime_error("Singular matrix (zero pivot)");
            }

            // Échange les lignes
            if (pivot_row != col) {
                for (int j = col; j < n; ++j) {
                    std::swap(K(col, j), K(pivot_row, j));
                }
                std::swap(F[col], F[pivot_row]);
            }

            // Élimination
            for (int row = col + 1; row < n; ++row) {
                double factor = K(row, col) / K(col, col);
                for (int j = col; j < n; ++j) {
                    K(row, j) -= factor * K(col, j);
                }
                F[row] -= factor * F[col];
            }
        }

        // Back substitution
        Vector u(n);
        for (int i = n - 1; i >= 0; --i) {
            u[i] = F[i];
            for (int j = i + 1; j < n; ++j) {
                u[i] -= K(i, j) * u[j];
            }
            u[i] /= K(i, i);
        }

        return u;
    }

    // Solveur itératif Gauss-Seidel (alternative pour matrices creuses)
    // Plus lent mais pédagogique
    static Vector gauss_seidel(const Matrix& K, const Vector& F,
                               int max_iter = 1000, double tol = 1e-6) {
        int n = K.rows();
        Vector u(n, 0.0);

        for (int iter = 0; iter < max_iter; ++iter) {
            Vector u_old = u;

            for (int i = 0; i < n; ++i) {
                double sum = F[i];
                for (int j = 0; j < i; ++j) {
                    sum -= K(i, j) * u[j];
                }
                for (int j = i + 1; j < n; ++j) {
                    sum -= K(i, j) * u_old[j];
                }
                u[i] = sum / K(i, i);
            }

            // Critère de convergence
            double error = 0.0;
            for (int i = 0; i < n; ++i) {
                error = std::max(error, std::abs(u[i] - u_old[i]));
            }

            if (error < tol) {
                return u;
            }
        }

        std::cerr << "Warning: Gauss-Seidel did not converge\n";
        return u;
    }
};
