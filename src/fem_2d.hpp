#pragma once
#include "matrix.hpp"
#include "solver.hpp"
#include <cmath>
#include <map>
#include <array>

class ElasticityFEM2D {
public:
    int nx, ny;
    double width, height;
    double E, nu;
    double dx, dy;
    double lambda, mu;

    int n_nodes, n_dof;

    Vector u, v;  // Déplacements
    Matrix stress_xx, stress_yy, stress_xy;  // Contraintes

    ElasticityFEM2D(double w, double h, int nx_, int ny_, double E_, double nu_)
        : nx(nx_), ny(ny_), width(w), height(h), E(E_), nu(nu_),
          dx(w / (nx_ - 1)), dy(h / (ny_ - 1)),
          n_nodes(nx_ * ny_), n_dof(2 * nx_ * ny_),
          stress_xx(ny_, nx_), stress_yy(ny_, nx_), stress_xy(ny_, nx_) {

        // Paramètres de Lamé
        lambda = E * nu / ((1.0 + nu) * (1.0 - 2.0 * nu));
        mu = E / (2.0 * (1.0 + nu));

        u.resize(ny * nx, 0.0);
        v.resize(ny * nx, 0.0);

        // Vérifier la cohérence de l'indexage
        if (!verify_indexing()) {
            throw std::runtime_error("Node indexing verification failed!");
        }
    }

    // Index global du nœud (i, j) - row-major: j*nx + i
    int node_idx(int i, int j) const {
        return j * nx + i;
    }

    // Inverse : (i, j) du nœud
    std::pair<int, int> get_coords(int node_idx_val) const {
        int j = node_idx_val / nx;
        int i = node_idx_val % nx;
        return {i, j};
    }

    // Vérification de cohérence
    bool verify_indexing() const {
        for (int j = 0; j < ny; ++j) {
            for (int i = 0; i < nx; ++i) {
                int idx = node_idx(i, j);
                auto [i_back, j_back] = get_coords(idx);
                if (i_back != i || j_back != j) return false;
            }
        }
        return true;
    }

    // DOF pour déplacement u au nœud (i, j)
    int dof_u(int i, int j) const {
        return 2 * node_idx(i, j);
    }

    // DOF pour déplacement v au nœud (i, j)
    int dof_v(int i, int j) const {
        return 2 * node_idx(i, j) + 1;
    }

    // Sélectionne les nœuds sur une limite entre deux positions
    std::vector<int> select_nodes_on_line(const std::string& boundary,
                                          double coord_min = -1e308,
                                          double coord_max = 1e308) const {
        std::vector<int> nodes;
        double x_min = coord_min, x_max = coord_max;
        double y_min = coord_min, y_max = coord_max;

        if (boundary == "bottom") {
            if (x_min < -1e308) x_min = 0.0;
            if (x_max > 1e308) x_max = width;
            for (int i = 0; i < nx; ++i) {
                double x = i * (width / (nx - 1));
                if (x >= x_min - 1e-10 && x <= x_max + 1e-10) {
                    nodes.push_back(node_idx(i, 0));
                }
            }
        }
        else if (boundary == "top") {
            if (x_min < -1e308) x_min = 0.0;
            if (x_max > 1e308) x_max = width;
            for (int i = 0; i < nx; ++i) {
                double x = i * (width / (nx - 1));
                if (x >= x_min - 1e-10 && x <= x_max + 1e-10) {
                    nodes.push_back(node_idx(i, ny - 1));
                }
            }
        }
        else if (boundary == "left") {
            if (y_min < -1e308) y_min = 0.0;
            if (y_max > 1e308) y_max = height;
            for (int j = 0; j < ny; ++j) {
                double y = j * (height / (ny - 1));
                if (y >= y_min - 1e-10 && y <= y_max + 1e-10) {
                    nodes.push_back(node_idx(0, j));
                }
            }
        }
        else if (boundary == "right") {
            if (y_min < -1e308) y_min = 0.0;
            if (y_max > 1e308) y_max = height;
            for (int j = 0; j < ny; ++j) {
                double y = j * (height / (ny - 1));
                if (y >= y_min - 1e-10 && y <= y_max + 1e-10) {
                    nodes.push_back(node_idx(nx - 1, j));
                }
            }
        }

        return nodes;
    }

    // Matrice de rigidité locale pour élément Q1 bilinéaire
    Matrix local_stiffness_q1() const {
        Matrix k_local(8, 8);
        double a = dx / 2.0;
        double b = dy / 2.0;

        // Points de Gauss: 2x2
        double gp = 1.0 / std::sqrt(3.0);
        std::array<double, 2> xi_pts = {-gp, gp};
        std::array<double, 2> eta_pts = {-gp, gp};

        for (double xi : xi_pts) {
            for (double eta : eta_pts) {
                // Dérivées des fonctions de forme par rapport à xi, eta
                std::array<double, 4> dN_dxi = {
                    -(1.0 - eta) / 4.0, (1.0 - eta) / 4.0,
                    (1.0 + eta) / 4.0, -(1.0 + eta) / 4.0
                };

                std::array<double, 4> dN_deta = {
                    -(1.0 - xi) / 4.0, -(1.0 + xi) / 4.0,
                    (1.0 + xi) / 4.0, (1.0 - xi) / 4.0
                };

                // Jacobien
                double dx_dxi = dN_dxi[0] * 0 + dN_dxi[1] * dx + dN_dxi[2] * dx + dN_dxi[3] * 0;
                double dy_dxi = dN_dxi[0] * 0 + dN_dxi[1] * 0 + dN_dxi[2] * dy + dN_dxi[3] * dy;
                double dx_deta = dN_deta[0] * 0 + dN_deta[1] * dx + dN_deta[2] * dx + dN_deta[3] * 0;
                double dy_deta = dN_deta[0] * 0 + dN_deta[1] * 0 + dN_deta[2] * dy + dN_deta[3] * dy;

                double det_J = dx_dxi * dy_deta - dx_deta * dy_dxi;

                if (std::abs(det_J) < 1e-14) {
                    throw std::runtime_error("Singular Jacobian");
                }

                // Dérivées par rapport à x, y
                std::array<double, 4> dN_dx, dN_dy;
                for (int k = 0; k < 4; ++k) {
                    dN_dx[k] = (dy_deta * dN_dxi[k] - dy_dxi * dN_deta[k]) / det_J;
                    dN_dy[k] = (dx_dxi * dN_deta[k] - dx_deta * dN_dxi[k]) / det_J;
                }

                // Matrice constitutive D
                Matrix D(3, 3);
                D(0, 0) = lambda + 2.0 * mu;
                D(0, 1) = lambda;
                D(1, 0) = lambda;
                D(1, 1) = lambda + 2.0 * mu;
                D(2, 2) = mu;

                // Matrice de déformation B
                Matrix B(3, 8);
                for (int i = 0; i < 4; ++i) {
                    B(0, 2*i) = dN_dx[i];
                    B(1, 2*i+1) = dN_dy[i];
                    B(2, 2*i) = dN_dy[i];
                    B(2, 2*i+1) = dN_dx[i];
                }

                // Contribution locale: k += B^T * D * B * det_J
                double w = 1.0;
                for (int i = 0; i < 8; ++i) {
                    for (int j = 0; j < 8; ++j) {
                        double val = 0.0;
                        for (int k = 0; k < 3; ++k) {
                            for (int l = 0; l < 3; ++l) {
                                val += B(k, i) * D(k, l) * B(l, j);
                            }
                        }
                        k_local.add(i, j, val * det_J * w);
                    }
                }
            }
        }

        return k_local;
    }

    // Assemble la matrice de rigidité globale
    Matrix assemble_global_matrix() const {
        Matrix K(n_dof, n_dof);
        Matrix k_ref = local_stiffness_q1();

        for (int j = 0; j < ny - 1; ++j) {
            for (int i = 0; i < nx - 1; ++i) {
                // Nœuds de l'élément: (i,j), (i+1,j), (i+1,j+1), (i,j+1)
                std::array<int, 8> global_dofs = {
                    dof_u(i, j), dof_v(i, j),
                    dof_u(i+1, j), dof_v(i+1, j),
                    dof_u(i+1, j+1), dof_v(i+1, j+1),
                    dof_u(i, j+1), dof_v(i, j+1)
                };

                // Assembly
                for (int i_loc = 0; i_loc < 8; ++i_loc) {
                    for (int j_loc = 0; j_loc < 8; ++j_loc) {
                        K.add(global_dofs[i_loc], global_dofs[j_loc],
                              k_ref(i_loc, j_loc));
                    }
                }
            }
        }

        return K;
    }

    // Applique une condition aux limites (DDL fixe) avec CONSTRAINT ELIMINATION
    // au lieu de la méthode des pénalités (qui casse le couplage)
    void apply_dirichlet(Matrix& K, Vector& F, int dof, double value) const {
        // FIX: Use constraint elimination instead of penalty method
        // Subtract the contribution of this constrained DOF from all equations
        // F[i] -= K[i, dof] * value for all i
        // Then zero row and column to implement the constraint

        for (int i = 0; i < K.rows(); ++i) {
            if (i != dof) {
                F[i] -= K(i, dof) * value;
            }
        }

        // Now apply the penalty method ON TOP of constraint elimination
        // This preserves the coupling better
        for (int j = 0; j < K.cols(); ++j) {
            K(dof, j) = 0.0;
        }
        for (int i = 0; i < K.rows(); ++i) {
            K(i, dof) = 0.0;
        }
        K(dof, dof) = 1.0;
        F[dof] = value;
    }

    // Résout le système avec conditions aux limites
    void solve(const std::map<std::string, std::vector<std::pair<int, double>>>& bcs,
               const std::vector<std::tuple<int, int, double>>& node_bcs = {}) {
        Matrix K = assemble_global_matrix();
        Vector F(n_dof, 0.0);

        // Diagnostic: Vérifier quelques entrées de la matrice avant BCs
        double K_sample_uu = K(dof_u(nx/2, ny/2), dof_u(nx/2, ny/2));  // Stiffness at interior u DOF
        std::cout << "DEBUG: K(" << dof_u(nx/2, ny/2) << "," << dof_u(nx/2, ny/2) << ") = " << K_sample_uu << "\n";

        // Appliquer les conditions aux limites sur les bords
        for (const auto& [boundary, bc_list] : bcs) {
            if (boundary == "nodes") continue;  // Traité séparément

            if (boundary == "bottom") {
                int j = 0;
                for (const auto& [comp, value] : bc_list) {
                    for (int i = 0; i < nx; ++i) {
                        int dof = (comp == 0) ? dof_u(i, j) : dof_v(i, j);
                        apply_dirichlet(K, F, dof, value);
                    }
                }
            }
            else if (boundary == "top") {
                int j = ny - 1;
                for (const auto& [comp, value] : bc_list) {
                    for (int i = 0; i < nx; ++i) {
                        int dof = (comp == 0) ? dof_u(i, j) : dof_v(i, j);
                        apply_dirichlet(K, F, dof, value);
                    }
                }
            }
            else if (boundary == "left") {
                int i = 0;
                for (const auto& [comp, value] : bc_list) {
                    for (int j = 0; j < ny; ++j) {
                        int dof = (comp == 0) ? dof_u(i, j) : dof_v(i, j);
                        apply_dirichlet(K, F, dof, value);
                    }
                }
            }
            else if (boundary == "right") {
                int i = nx - 1;
                for (const auto& [comp, value] : bc_list) {
                    for (int j = 0; j < ny; ++j) {
                        int dof = (comp == 0) ? dof_u(i, j) : dof_v(i, j);
                        apply_dirichlet(K, F, dof, value);
                    }
                }
            }
        }

        // Appliquer les BC sur nœuds du paramètre
        for (const auto& [node_idx, comp, value] : node_bcs) {
            auto [i, j] = get_coords(node_idx);
            int dof = (comp == 0) ? dof_u(i, j) : dof_v(i, j);
            apply_dirichlet(K, F, dof, value);
        }

        // Résoudre
        Vector u_full = GaussSolver::solve(K, F);

        // Extraire u et v
        if (u_full.size() != n_dof) {
            throw std::runtime_error("Solver result size mismatch");
        }

        for (int j = 0; j < ny; ++j) {
            for (int i = 0; i < nx; ++i) {
                int node = node_idx(i, j);
                u[node] = u_full[dof_u(i, j)];
                v[node] = u_full[dof_v(i, j)];
            }
        }

    }

    // Calcule les contraintes à partir des déplacements
    void compute_stress() {
        stress_xx.set_zero();
        stress_yy.set_zero();
        stress_xy.set_zero();

        for (int j = 0; j < ny; ++j) {
            for (int i = 0; i < nx; ++i) {
                int i_left = (i > 0) ? i - 1 : i;
                int i_right = (i < nx - 1) ? i + 1 : i;
                int j_down = (j > 0) ? j - 1 : j;
                int j_up = (j < ny - 1) ? j + 1 : j;

                // Dérivées des déplacements
                double du_dx, du_dy, dv_dx, dv_dy;

                if (i == 0) {
                    du_dx = (u[node_idx(i_right, j)] - u[node_idx(i, j)]) / dx;
                    dv_dx = (v[node_idx(i_right, j)] - v[node_idx(i, j)]) / dx;
                } else if (i == nx - 1) {
                    du_dx = (u[node_idx(i, j)] - u[node_idx(i_left, j)]) / dx;
                    dv_dx = (v[node_idx(i, j)] - v[node_idx(i_left, j)]) / dx;
                } else {
                    du_dx = (u[node_idx(i_right, j)] - u[node_idx(i_left, j)]) / (2.0 * dx);
                    dv_dx = (v[node_idx(i_right, j)] - v[node_idx(i_left, j)]) / (2.0 * dx);
                }

                if (j == 0) {
                    du_dy = (u[node_idx(i, j_up)] - u[node_idx(i, j)]) / dy;
                    dv_dy = (v[node_idx(i, j_up)] - v[node_idx(i, j)]) / dy;
                } else if (j == ny - 1) {
                    du_dy = (u[node_idx(i, j)] - u[node_idx(i, j_down)]) / dy;
                    dv_dy = (v[node_idx(i, j)] - v[node_idx(i, j_down)]) / dy;
                } else {
                    du_dy = (u[node_idx(i, j_up)] - u[node_idx(i, j_down)]) / (2.0 * dy);
                    dv_dy = (v[node_idx(i, j_up)] - v[node_idx(i, j_down)]) / (2.0 * dy);
                }

                // Déformations
                double eps_xx = du_dx;
                double eps_yy = dv_dy;
                double eps_xy = 0.5 * (du_dy + dv_dx);

                // Contraintes
                stress_xx(j, i) = lambda * (eps_xx + eps_yy) + 2.0 * mu * eps_xx;
                stress_yy(j, i) = lambda * (eps_xx + eps_yy) + 2.0 * mu * eps_yy;
                stress_xy(j, i) = 2.0 * mu * eps_xy;
            }
        }
    }

    // Contrainte von Mises
    Matrix von_mises() const {
        Matrix vm(ny, nx);
        for (int j = 0; j < ny; ++j) {
            for (int i = 0; i < nx; ++i) {
                double sxx = stress_xx(j, i);
                double syy = stress_yy(j, i);
                double sxy = stress_xy(j, i);
                vm(j, i) = std::sqrt(sxx*sxx + syy*syy - sxx*syy + 3.0*sxy*sxy);
            }
        }
        return vm;
    }
};
