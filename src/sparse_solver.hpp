#pragma once
#include "sparse_matrix.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

class SparseSolver {
public:
    struct Options {
        int max_iterations;
        double tolerance;
        bool verbose;

        Options()
            : max_iterations(10000), tolerance(1e-9), verbose(false) {}
    };

    // Solve A*x = b using Conjugate Gradient method
    // A must be symmetric positive definite
    static SparseVector solve_cg(const SparseMatrix& A, const SparseVector& b,
                                 const Options& opts = Options()) {
        int n = A.rows();
        if (static_cast<int>(b.size()) != n) {
            throw std::invalid_argument("System size mismatch");
        }

        // Initialize x = 0
        SparseVector x(n, 0.0);
        return solve_cg(A, b, x, opts);
    }

    // Solve A*x = b with initial guess x0
    static SparseVector solve_cg(const SparseMatrix& A, const SparseVector& b,
                                 const SparseVector& x0,
                                 const Options& opts = Options()) {
        int n = A.rows();
        SparseVector x = x0;

        // r = b - A*x
        SparseVector r = subtract(b, A.multiply(x));
        double rsold = dot(r, r);

        if (rsold < opts.tolerance * opts.tolerance) {
            return x; // Already converged
        }

        SparseVector p = r;
        double rsnew = rsold;

        for (int iter = 0; iter < opts.max_iterations; ++iter) {
            // Ap = A * p
            SparseVector Ap = A.multiply(p);
            double pAp = dot(p, Ap);

            if (std::abs(pAp) < 1e-15) {
                if (opts.verbose) {
                    std::cout << "CG: pAp too small at iteration " << iter << "\n";
                }
                break;
            }

            // alpha = rsold / (p^T * A * p)
            double alpha = rsold / pAp;

            // x = x + alpha * p
            for (int i = 0; i < n; ++i) {
                x[i] += alpha * p[i];
            }

            // r = r - alpha * A * p
            for (int i = 0; i < n; ++i) {
                r[i] -= alpha * Ap[i];
            }

            rsnew = dot(r, r);

            if (opts.verbose && iter % 100 == 0) {
                std::cout << "CG iteration " << iter << ": residual = "
                          << std::sqrt(rsnew) << "\n";
            }

            // Check convergence
            if (rsnew < opts.tolerance * opts.tolerance) {
                if (opts.verbose) {
                    std::cout << "CG converged in " << (iter + 1) << " iterations\n";
                }
                return x;
            }

            // beta = rsnew / rsold
            double beta = rsnew / rsold;

            // p = r + beta * p
            for (int i = 0; i < n; ++i) {
                p[i] = r[i] + beta * p[i];
            }

            rsold = rsnew;
        }

        if (opts.verbose) {
            std::cout << "CG did not converge. Final residual: " << std::sqrt(rsnew) << "\n";
        }

        return x;
    }

private:
    // Dot product: a^T * b
    static double dot(const SparseVector& a, const SparseVector& b) {
        double sum = 0.0;
        for (int i = 0; i < static_cast<int>(a.size()); ++i) {
            sum += a[i] * b[i];
        }
        return sum;
    }

    // Vector subtraction: a - b
    static SparseVector subtract(const SparseVector& a, const SparseVector& b) {
        SparseVector c(a.size());
        for (int i = 0; i < static_cast<int>(a.size()); ++i) {
            c[i] = a[i] - b[i];
        }
        return c;
    }
};
