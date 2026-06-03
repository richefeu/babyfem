#pragma once
// frame.hpp — analyse de structures à barres/poutres 2D (teensFEM).
//
// Élément poutre-colonne 2D (Euler-Bernoulli + effort normal), 3 DDL par nœud :
//   u (horizontal), v (vertical), r (rotation).  DDL globaux : 3*node (+0,+1,+2).
//
// Connexions aux extrémités : chaque bout de barre est soit ENCASTRÉ (rigide,
// le moment passe : la rotation du bout = le r du nœud), soit ARTICULÉ (défaut,
// libre en rotation : moment nul, rotation condensée statiquement).
//   - les deux bouts articulés  -> il ne reste que l'axial = barre de treillis.
// Un nœud dont toutes les barres incidentes sont articulées a un r sans raideur
// (mécanisme local sans effet) : on le bloque automatiquement à 0.
//
// Charges : nodales (fx, fy, m) et réparties sur les barres (wx, wy en N/m,
// composantes globales) via le vecteur de charges équivalentes cohérentes.
//
// Réutilise le solveur direct (Gauss dense) du cœur partagé : les structures
// sont petites (quelques dizaines de nœuds).

#include "matrix.hpp"
#include "solver.hpp"
#include <vector>
#include <array>
#include <tuple>
#include <cmath>
#include <stdexcept>

struct FrameNode { double x, y; };

struct FrameMember {
    int n1, n2;                 // indices de nœuds
    double E, A, I;             // module, aire, inertie
    bool rigid1, rigid2;        // extrémité 1 / 2 encastrée (sinon articulée)
    double wx = 0, wy = 0;      // charge répartie globale (N/m)
};

class Frame {
public:
    std::vector<FrameNode> nodes;
    std::vector<FrameMember> members;
    std::vector<std::tuple<int, int, double>> supports;     // (node, ddl 0/1/2, valeur)
    std::vector<std::tuple<int, int, double>> nodal_loads;  // (node, ddl 0/1/2, valeur)

    Vector d;                                   // déplacements globaux (solution)
    std::vector<std::array<double, 6>> end_forces;  // efforts d'extrémité locaux par barre
                                                    // [N1, V1, M1, N2, V2, M2]

    int n_nodes() const { return static_cast<int>(nodes.size()); }
    int n_dof()   const { return 3 * n_nodes(); }
    static int dof_u(int n) { return 3 * n; }
    static int dof_v(int n) { return 3 * n + 1; }
    static int dof_r(int n) { return 3 * n + 2; }

    // Géométrie d'une barre : longueur et cosinus directeurs.
    void geom(const FrameMember& m, double& L, double& c, double& s) const {
        double dx = nodes[m.n2].x - nodes[m.n1].x;
        double dy = nodes[m.n2].y - nodes[m.n1].y;
        L = std::hypot(dx, dy);
        if (L < 1e-14) throw std::runtime_error("Barre de longueur nulle");
        c = dx / L; s = dy / L;
    }

    // Rigidité locale 6×6, DDL [u1,v1,r1,u2,v2,r2].
    static void local_stiffness(double E, double A, double I, double L, double k[6][6]) {
        for (int i = 0; i < 6; ++i) for (int j = 0; j < 6; ++j) k[i][j] = 0;
        double a = E * A / L;
        double b = 12 * E * I / (L*L*L);
        double cc = 6 * E * I / (L*L);
        double d4 = 4 * E * I / L;
        double e2 = 2 * E * I / L;
        k[0][0]= a;  k[0][3]=-a;
        k[3][0]=-a;  k[3][3]= a;
        k[1][1]= b;  k[1][2]= cc; k[1][4]=-b;  k[1][5]= cc;
        k[2][1]= cc; k[2][2]= d4; k[2][4]=-cc; k[2][5]= e2;
        k[4][1]=-b;  k[4][2]=-cc; k[4][4]= b;  k[4][5]=-cc;
        k[5][1]= cc; k[5][2]= e2; k[5][4]=-cc; k[5][5]= d4;
    }

    // Vecteur de charges équivalentes locales pour une charge répartie globale.
    static void equivalent_load(double wx, double wy, double L, double c, double s,
                                double r[6]) {
        double wa = wx * c + wy * s;        // composante axiale (local x)
        double wt = -wx * s + wy * c;       // composante transverse (local y)
        r[0] = wa * L / 2;  r[1] = wt * L / 2;  r[2] = wt * L*L / 12;
        r[3] = wa * L / 2;  r[4] = wt * L / 2;  r[5] = -wt * L*L / 12;
    }

    // Condensation statique d'un DDL relâché (moment nul) : met à jour k et r.
    static void condense(double k[6][6], double r[6], int dof) {
        double krr = k[dof][dof];
        if (std::abs(krr) < 1e-30) return;
        for (int i = 0; i < 6; ++i) if (i != dof) {
            double f = k[i][dof] / krr;
            r[i] -= f * r[dof];
            for (int j = 0; j < 6; ++j) if (j != dof) k[i][j] -= f * k[dof][j];
        }
        for (int i = 0; i < 6; ++i) { k[dof][i] = 0; k[i][dof] = 0; }
        r[dof] = 0;
    }

    // Matrice de transformation locale->global appliquée : renvoie kg = Tᵀ k T,
    // rg = Tᵀ r. T tourne (u,v) de l'angle de la barre, laisse r inchangé.
    static void to_global(const double k[6][6], const double r[6],
                          double c, double s, double kg[6][6], double rg[6]) {
        double T[6][6] = {{0}};
        double blk[3][3] = {{ c, s, 0}, {-s, c, 0}, {0, 0, 1}};
        for (int b = 0; b < 2; ++b)
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j) T[3*b+i][3*b+j] = blk[i][j];
        // kg = Tᵀ k T
        double tmp[6][6];
        for (int i = 0; i < 6; ++i) for (int j = 0; j < 6; ++j) {
            double v = 0; for (int p = 0; p < 6; ++p) v += k[i][p] * T[p][j]; tmp[i][j] = v;
        }
        for (int i = 0; i < 6; ++i) for (int j = 0; j < 6; ++j) {
            double v = 0; for (int p = 0; p < 6; ++p) v += T[p][i] * tmp[p][j]; kg[i][j] = v;
        }
        // rg = Tᵀ r
        for (int i = 0; i < 6; ++i) {
            double v = 0; for (int p = 0; p < 6; ++p) v += T[p][i] * r[p]; rg[i] = v;
        }
    }

    std::array<int, 6> member_dofs(const FrameMember& m) const {
        return { dof_u(m.n1), dof_v(m.n1), dof_r(m.n1),
                 dof_u(m.n2), dof_v(m.n2), dof_r(m.n2) };
    }

    void solve() {
        int N = n_dof();
        Matrix K(N, N);
        Vector F(N, 0.0);

        // Assemblage
        for (const auto& m : members) {
            double L, c, s; geom(m, L, c, s);
            double k[6][6], r[6];
            local_stiffness(m.E, m.A, m.I, L, k);
            equivalent_load(m.wx, m.wy, L, c, s, r);
            if (!m.rigid1) condense(k, r, 2);   // rotation du nœud 1 relâchée
            if (!m.rigid2) condense(k, r, 5);   // rotation du nœud 2 relâchée
            double kg[6][6], rg[6];
            to_global(k, r, c, s, kg, rg);
            auto dofs = member_dofs(m);
            for (int i = 0; i < 6; ++i) {
                F[dofs[i]] += rg[i];
                for (int j = 0; j < 6; ++j) K.add(dofs[i], dofs[j], kg[i][j]);
            }
        }

        // Charges nodales
        for (const auto& [n, comp, val] : nodal_loads) F[3*n + comp] += val;

        // Blocages : utilisateur + rotations sans raideur (nœuds tout-articulés)
        std::vector<std::tuple<int, int, double>> all_bc = supports;
        double maxdiag = 0;
        for (int i = 0; i < N; ++i) maxdiag = std::max(maxdiag, std::abs(K(i, i)));
        for (int n = 0; n < n_nodes(); ++n) {
            int rd = dof_r(n);
            if (std::abs(K(rd, rd)) < 1e-9 * maxdiag)
                all_bc.emplace_back(n, 2, 0.0);
        }

        for (const auto& [n, comp, val] : all_bc) apply_dirichlet(K, F, 3*n + comp, val);

        d = GaussSolver::solve(K, F);
        recover_end_forces();
    }

private:
    void apply_dirichlet(Matrix& K, Vector& F, int dof, double value) const {
        for (int i = 0; i < K.rows(); ++i) if (i != dof) F[i] -= K(i, dof) * value;
        for (int j = 0; j < K.cols(); ++j) K(dof, j) = 0.0;
        for (int i = 0; i < K.rows(); ++i) K(i, dof) = 0.0;
        K(dof, dof) = 1.0;
        F[dof] = value;
    }

    // Efforts d'extrémité locaux : s = k_orig · d_local - r_orig, après avoir
    // récupéré les rotations relâchées (condition de moment nul au bout).
    void recover_end_forces() {
        end_forces.assign(members.size(), {});
        for (size_t e = 0; e < members.size(); ++e) {
            const auto& m = members[e];
            double L, c, s; geom(m, L, c, s);
            double k[6][6], r[6];
            local_stiffness(m.E, m.A, m.I, L, k);
            equivalent_load(m.wx, m.wy, L, c, s, r);

            // d_local = T · d_global
            auto dofs = member_dofs(m);
            double dg[6]; for (int i = 0; i < 6; ++i) dg[i] = d[dofs[i]];
            double blk[3][3] = {{ c, s, 0}, {-s, c, 0}, {0, 0, 1}};
            double dl[6];
            for (int b = 0; b < 2; ++b) for (int i = 0; i < 3; ++i) {
                double v = 0; for (int j = 0; j < 3; ++j) v += blk[i][j] * dg[3*b+j];
                dl[3*b+i] = v;
            }
            // Récupère les rotations relâchées (moment nul) : k_R·dl_R = rhs_R.
            // Si les deux bouts sont articulés, leurs rotations sont couplées
            // (k[2][5] ≠ 0) : on résout le système simultanément, pas l'un après
            // l'autre.
            std::vector<int> rel;
            if (!m.rigid1) rel.push_back(2);
            if (!m.rigid2) rel.push_back(5);
            if (rel.size() == 1) {
                int a = rel[0]; double rhs = r[a];
                for (int j = 0; j < 6; ++j) if (j != a) rhs -= k[a][j] * dl[j];
                dl[a] = rhs / k[a][a];
            } else if (rel.size() == 2) {
                int a = rel[0], b = rel[1];
                double ra = r[a], rb = r[b];
                for (int j = 0; j < 6; ++j) if (j != a && j != b) {
                    ra -= k[a][j] * dl[j]; rb -= k[b][j] * dl[j];
                }
                double det = k[a][a] * k[b][b] - k[a][b] * k[b][a];
                dl[a] = (ra * k[b][b] - k[a][b] * rb) / det;
                dl[b] = (k[a][a] * rb - ra * k[b][a]) / det;
            }

            double sloc[6];
            for (int i = 0; i < 6; ++i) {
                double v = -r[i];
                for (int j = 0; j < 6; ++j) v += k[i][j] * dl[j];
                sloc[i] = v;
            }
            end_forces[e] = { sloc[0], sloc[1], sloc[2], sloc[3], sloc[4], sloc[5] };
        }
    }

public:
    // Échantillonne les efforts internes le long d'une barre, à l'abscisse
    // x ∈ [0, L] depuis le nœud 1. Conventions :
    //   N = effort normal (traction > 0), V = tranchant, M = moment fléchissant.
    void internal_at(int e, double x, double& N, double& V, double& M) const {
        const auto& m = members[e];
        double L, c, s; geom(m, L, c, s);
        double wt = -m.wx * s + m.wy * c;   // charge transverse locale
        double wa = m.wx * c + m.wy * s;    // charge axiale locale
        const auto& f = end_forces[e];      // [N1,V1,M1,N2,V2,M2] (efforts locaux au nœud 1)
        // Équilibre d'un tronçon [0, x] coupé depuis le nœud 1.
        // Conventions : traction > 0 ; moment de flexion positif = fibre
        // inférieure tendue (sagging) ; V = dM/dx.
        N = -(f[0] + wa * x);
        V =   f[1] + wt * x;
        M =  -f[2] + f[1] * x + 0.5 * wt * x * x;
    }
};
