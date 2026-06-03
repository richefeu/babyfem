#pragma once
#include <vector>
#include <cmath>
#include <stdexcept>
#include <map>

// Sparse matrix in CSR (Compressed Sparse Row) format
class SparseMatrix {
private:
    int rows_;
    int cols_;
    std::vector<double> values_;   // Non-zero values
    std::vector<int> col_indices_; // Column indices for each value
    std::vector<int> row_ptr_;     // Pointer to start of each row in values_/col_indices_

public:
    SparseMatrix(int rows, int cols)
        : rows_(rows), cols_(cols) {
        row_ptr_.resize(rows + 1, 0);
    }

    int rows() const { return rows_; }
    int cols() const { return cols_; }
    int nnz() const { return static_cast<int>(values_.size()); }

    // Add value to matrix (for assembly - accumulates)
    void add(int i, int j, double val) {
        if (i < 0 || i >= rows_ || j < 0 || j >= cols_) {
            throw std::out_of_range("Matrix index out of range");
        }

        // Find or create entry
        for (int k = row_ptr_[i]; k < row_ptr_[i + 1]; ++k) {
            if (col_indices_[k] == j) {
                values_[k] += val;
                return;
            }
        }

        // Entry doesn't exist - this shouldn't happen in CSR assembly
        // For assembly, use triplet format first, then convert to CSR
        throw std::runtime_error("SparseMatrix::add requires pre-allocated structure");
    }

    // Convert from triplet format (for easy assembly).
    // Les triplets en double (même (i, j)) sont sommés, et chaque ligne est
    // stockée avec ses colonnes triées — quel que soit l'ordre d'entrée.
    static SparseMatrix from_triplet(int rows, int cols,
                                     const std::vector<int>& row_idx,
                                     const std::vector<int>& col_idx,
                                     const std::vector<double>& values) {
        SparseMatrix mat(rows, cols);

        // Accumuler par ligne : colonne -> valeur sommée
        std::vector<std::map<int, double>> rows_acc(rows);
        for (int k = 0; k < static_cast<int>(row_idx.size()); ++k) {
            rows_acc[row_idx[k]][col_idx[k]] += values[k];
        }

        // Construire le CSR (colonnes triées grâce à std::map)
        mat.row_ptr_[0] = 0;
        for (int i = 0; i < rows; ++i) {
            for (const auto& [col, val] : rows_acc[i]) {
                mat.col_indices_.push_back(col);
                mat.values_.push_back(val);
            }
            mat.row_ptr_[i + 1] = static_cast<int>(mat.values_.size());
        }

        return mat;
    }

    // Matrix-vector multiplication: y = A * x
    std::vector<double> multiply(const std::vector<double>& x) const {
        if (static_cast<int>(x.size()) != cols_) {
            throw std::invalid_argument("Vector size mismatch");
        }

        std::vector<double> y(rows_, 0.0);
        for (int i = 0; i < rows_; ++i) {
            for (int k = row_ptr_[i]; k < row_ptr_[i + 1]; ++k) {
                y[i] += values_[k] * x[col_indices_[k]];
            }
        }
        return y;
    }

    // Get value at (i, j) - slow, use multiply() instead
    double get(int i, int j) const {
        for (int k = row_ptr_[i]; k < row_ptr_[i + 1]; ++k) {
            if (col_indices_[k] == j) return values_[k];
        }
        return 0.0;
    }

    // Modify single element (slow, for testing only)
    void set(int i, int j, double val) {
        for (int k = row_ptr_[i]; k < row_ptr_[i + 1]; ++k) {
            if (col_indices_[k] == j) {
                values_[k] = val;
                return;
            }
        }
        throw std::runtime_error("Element not found in sparse matrix");
    }
};

using SparseVector = std::vector<double>;
