#pragma once
#include <vector>
#include <cmath>
#include <stdexcept>
#include <iostream>

class Matrix {
    int rows_, cols_;
    std::vector<double> data_;

public:
    Matrix(int rows, int cols) : rows_(rows), cols_(cols), data_(rows * cols, 0.0) {}

    int rows() const { return rows_; }
    int cols() const { return cols_; }

    double& operator()(int i, int j) {
        if (i < 0 || i >= rows_ || j < 0 || j >= cols_) {
            throw std::out_of_range("Matrix index out of bounds");
        }
        return data_[i * cols_ + j];
    }

    double operator()(int i, int j) const {
        if (i < 0 || i >= rows_ || j < 0 || j >= cols_) {
            throw std::out_of_range("Matrix index out of bounds");
        }
        return data_[i * cols_ + j];
    }

    void set_zero() {
        std::fill(data_.begin(), data_.end(), 0.0);
    }

    // Ajout de valeur (pour assemblage)
    void add(int i, int j, double val) {
        (*this)(i, j) += val;
    }

    // Multiplication matrice-vecteur: y = A*x
    std::vector<double> multiply(const std::vector<double>& x) const {
        if (x.size() != static_cast<size_t>(cols_)) {
            throw std::invalid_argument("Vector size mismatch");
        }
        std::vector<double> y(rows_, 0.0);
        for (int i = 0; i < rows_; ++i) {
            for (int j = 0; j < cols_; ++j) {
                y[i] += (*this)(i, j) * x[j];
            }
        }
        return y;
    }

    // Norme du vecteur
    static double norm(const std::vector<double>& x) {
        double sum = 0.0;
        for (double val : x) {
            sum += val * val;
        }
        return std::sqrt(sum);
    }

    // Résidu: r = b - A*x
    std::vector<double> residual(const std::vector<double>& b,
                                 const std::vector<double>& x) const {
        auto Ax = multiply(x);
        std::vector<double> r(rows_);
        for (int i = 0; i < rows_; ++i) {
            r[i] = b[i] - Ax[i];
        }
        return r;
    }

    void print(const std::string& name = "Matrix") const {
        std::cout << name << " (" << rows_ << "x" << cols_ << "):\n";
        for (int i = 0; i < std::min(rows_, 5); ++i) {
            for (int j = 0; j < std::min(cols_, 5); ++j) {
                std::cout << (*this)(i, j) << " ";
            }
            if (cols_ > 5) std::cout << "...";
            std::cout << "\n";
        }
        if (rows_ > 5) std::cout << "...\n";
    }
};

// Vecteur comme alias pour std::vector<double>
using Vector = std::vector<double>;
