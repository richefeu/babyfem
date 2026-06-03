#pragma once
// fem_tri.hpp — élasticité linéaire 2D sur maillage triangulaire (kidfem).
//
// Élément : triangle linéaire à 3 nœuds (CST, Constant Strain Triangle). La
// matrice de déformation B est *constante* sur l'élément (pas de quadrature),
// d'où une rigidité élémentaire analytique :  K_e = aire · Bᵀ D B  (épaisseur 1).
//
// Hypothèse : déformations planes (même loi que babyfem), isotrope.
// 2 DDL par nœud : dof_u(n) = 2n (horizontal), dof_v(n) = 2n+1 (vertical).
//
// Le solveur direct (Gauss dense) et le solveur itératif (CG creux) du cœur
// partagé sont réutilisés tels quels — seul l'assemblage change.

#include "mesh.hpp"
#include "matrix.hpp"
#include "solver.hpp"
#include "sparse_matrix.hpp"
#include "sparse_solver.hpp"
#include <array>
#include <vector>
#include <tuple>
#include <cmath>
#include <stdexcept>

class ElasticityFEM_T3 {
public:
    const Mesh& mesh;
    double E, nu, lambda, mu;
    int n_nodes, n_dof;

    Vector u, v;                          // déplacements nodaux
    // Contraintes constantes par élément (CST) :
    std::vector<double> e_sxx, e_syy, e_sxy, e_vm;
    // Contraintes moyennées aux nœuds (pour le résumé) :
    Vector n_vm;

    ElasticityFEM_T3(const Mesh& m, double E_, double nu_)
        : mesh(m), E(E_), nu(nu_),
          n_nodes(m.n_nodes()), n_dof(2 * m.n_nodes()) {
        lambda = E * nu / ((1.0 + nu) * (1.0 - 2.0 * nu));   // déformations planes
        mu     = E / (2.0 * (1.0 + nu));
        u.assign(n_nodes, 0.0);
        v.assign(n_nodes, 0.0);
    }

    static int dof_u(int node) { return 2 * node; }
    static int dof_v(int node) { return 2 * node + 1; }

    // Géométrie d'un triangle : coefficients b_i, c_i (gradients × 2·aire) et
    // aire signée. dN_i/dx = b_i / (2A) , dN_i/dy = c_i / (2A).
    void element_geom(int e, std::array<double, 3>& b, std::array<double, 3>& c,
                      double& two_area) const {
        const auto& t = mesh.tris[e];
        double x0 = mesh.nodes[t[0]][0], y0 = mesh.nodes[t[0]][1];
        double x1 = mesh.nodes[t[1]][0], y1 = mesh.nodes[t[1]][1];
        double x2 = mesh.nodes[t[2]][0], y2 = mesh.nodes[t[2]][1];
        b = { y1 - y2, y2 - y0, y0 - y1 };
        c = { x2 - x1, x0 - x2, x1 - x0 };
        two_area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0);  // signé
    }

    // Matrice constitutive (déformations planes) : σ = D ε.
    void constitutive(double D[3][3]) const {
        D[0][0] = lambda + 2 * mu; D[0][1] = lambda;          D[0][2] = 0;
        D[1][0] = lambda;          D[1][1] = lambda + 2 * mu; D[1][2] = 0;
        D[2][0] = 0;               D[2][1] = 0;               D[2][2] = mu;
    }

    // Matrice B (3×6) de l'élément e, ainsi que l'aire (positive).
    void element_B(int e, double B[3][6], double& area) const {
        std::array<double, 3> b, c; double two_area;
        element_geom(e, b, c, two_area);
        if (std::abs(two_area) < 1e-300)
            throw std::runtime_error("Triangle dégénéré (aire nulle)");
        area = 0.5 * std::abs(two_area);
        for (int i = 0; i < 3; ++i) {
            double dNdx = b[i] / two_area;
            double dNdy = c[i] / two_area;
            B[0][2*i] = dNdx; B[0][2*i+1] = 0;
            B[1][2*i] = 0;    B[1][2*i+1] = dNdy;
            B[2][2*i] = dNdy; B[2][2*i+1] = dNdx;
        }
    }

    // Rigidité élémentaire 6×6 :  K_e = aire · Bᵀ D B.
    void element_stiffness(int e, double K[6][6]) const {
        double B[3][6], D[3][3], area;
        element_B(e, B, area);
        constitutive(D);
        // DB = D · B  (3×6)
        double DB[3][6];
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 6; ++j) {
                double s = 0;
                for (int k = 0; k < 3; ++k) s += D[i][k] * B[k][j];
                DB[i][j] = s;
            }
        for (int i = 0; i < 6; ++i)
            for (int j = 0; j < 6; ++j) {
                double s = 0;
                for (int k = 0; k < 3; ++k) s += B[k][i] * DB[k][j];
                K[i][j] = s * area;
            }
    }

    std::array<int, 6> element_dofs(int e) const {
        const auto& t = mesh.tris[e];
        return { dof_u(t[0]), dof_v(t[0]),
                 dof_u(t[1]), dof_v(t[1]),
                 dof_u(t[2]), dof_v(t[2]) };
    }

    // node_bcs    : (node, comp 0=u/1=v, valeur imposée) — Dirichlet
    // nodal_forces: (node, comp, valeur en N)            — Neumann
    void solve(const std::vector<std::tuple<int, int, double>>& node_bcs,
               const std::vector<std::tuple<int, int, double>>& nodal_forces,
               bool use_sparse) {
        if (use_sparse) solve_sparse(node_bcs, nodal_forces);
        else            solve_dense(node_bcs, nodal_forces);
        compute_stress();
    }

private:
    void scatter_forces(Vector& F,
                        const std::vector<std::tuple<int, int, double>>& nodal_forces) const {
        for (const auto& [node, comp, value] : nodal_forces)
            F[comp == 0 ? dof_u(node) : dof_v(node)] += value;
    }

    // --- Solveur direct : Gauss dense + élimination de contrainte ----------
    void solve_dense(const std::vector<std::tuple<int, int, double>>& node_bcs,
                     const std::vector<std::tuple<int, int, double>>& nodal_forces) {
        Matrix K(n_dof, n_dof);
        Vector F(n_dof, 0.0);
        for (int e = 0; e < mesh.n_tris(); ++e) {
            double Ke[6][6]; element_stiffness(e, Ke);
            auto d = element_dofs(e);
            for (int i = 0; i < 6; ++i)
                for (int j = 0; j < 6; ++j)
                    K.add(d[i], d[j], Ke[i][j]);
        }
        scatter_forces(F, nodal_forces);

        for (const auto& [node, comp, value] : node_bcs) {
            int dof = (comp == 0) ? dof_u(node) : dof_v(node);
            apply_dirichlet(K, F, dof, value);
        }

        Vector x = GaussSolver::solve(K, F);
        store_displacements(x);
    }

    // Élimination de contrainte (comme babyfem) : reporte au second membre,
    // annule ligne/colonne, fixe la diagonale.
    void apply_dirichlet(Matrix& K, Vector& F, int dof, double value) const {
        for (int i = 0; i < K.rows(); ++i)
            if (i != dof) F[i] -= K(i, dof) * value;
        for (int j = 0; j < K.cols(); ++j) K(dof, j) = 0.0;
        for (int i = 0; i < K.rows(); ++i) K(i, dof) = 0.0;
        K(dof, dof) = 1.0;
        F[dof] = value;
    }

    // --- Solveur itératif : assemblage creux + CG + Dirichlet par pénalité --
    void solve_sparse(const std::vector<std::tuple<int, int, double>>& node_bcs,
                      const std::vector<std::tuple<int, int, double>>& nodal_forces) {
        std::vector<int> rows, cols; std::vector<double> vals;
        for (int e = 0; e < mesh.n_tris(); ++e) {
            double Ke[6][6]; element_stiffness(e, Ke);
            auto d = element_dofs(e);
            for (int i = 0; i < 6; ++i)
                for (int j = 0; j < 6; ++j)
                    if (std::abs(Ke[i][j]) > 1e-15) {
                        rows.push_back(d[i]); cols.push_back(d[j]);
                        vals.push_back(Ke[i][j]);
                    }
        }
        SparseMatrix K = SparseMatrix::from_triplet(n_dof, n_dof, rows, cols, vals);

        Vector F(n_dof, 0.0);
        scatter_forces(F, nodal_forces);

        const double penalty = 1e12;
        for (const auto& [node, comp, value] : node_bcs) {
            int dof = (comp == 0) ? dof_u(node) : dof_v(node);
            K.set(dof, dof, K.get(dof, dof) + penalty);
            F[dof] += penalty * value;
        }

        SparseSolver::Options opt; opt.max_iterations = 20000;
        Vector x = SparseSolver::solve_cg(K, F, opt);
        store_displacements(x);
    }

    void store_displacements(const Vector& x) {
        if (static_cast<int>(x.size()) != n_dof)
            throw std::runtime_error("Taille du résultat incohérente");
        for (int n = 0; n < n_nodes; ++n) { u[n] = x[dof_u(n)]; v[n] = x[dof_v(n)]; }
    }

public:
    // Contraintes : constantes par élément (ε = B u_e, σ = D ε), puis von Mises
    // moyennée aux nœuds pour le résumé.
    void compute_stress() {
        e_sxx.assign(mesh.n_tris(), 0.0);
        e_syy.assign(mesh.n_tris(), 0.0);
        e_sxy.assign(mesh.n_tris(), 0.0);
        e_vm.assign(mesh.n_tris(), 0.0);
        n_vm.assign(n_nodes, 0.0);
        std::vector<int> count(n_nodes, 0);

        for (int e = 0; e < mesh.n_tris(); ++e) {
            double B[3][6], D[3][3], area; element_B(e, B, area); constitutive(D);
            double ue[6];
            const auto& t = mesh.tris[e];
            for (int k = 0; k < 3; ++k) { ue[2*k] = u[t[k]]; ue[2*k+1] = v[t[k]]; }
            double eps[3] = {0, 0, 0};
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 6; ++j) eps[i] += B[i][j] * ue[j];
            double sxx = D[0][0]*eps[0] + D[0][1]*eps[1];
            double syy = D[1][0]*eps[0] + D[1][1]*eps[1];
            double sxy = D[2][2]*eps[2];
            e_sxx[e] = sxx; e_syy[e] = syy; e_sxy[e] = sxy;
            double vm = std::sqrt(sxx*sxx + syy*syy - sxx*syy + 3*sxy*sxy);
            e_vm[e] = vm;
            for (int k = 0; k < 3; ++k) { n_vm[t[k]] += vm; count[t[k]]++; }
        }
        for (int n = 0; n < n_nodes; ++n) if (count[n]) n_vm[n] /= count[n];
    }
};
