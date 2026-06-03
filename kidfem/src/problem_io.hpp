#pragma once
// problem_io.hpp — lecture d'un problème kidfem depuis un fichier texte.
//
// Même esprit que babyfem (séparer données et moteur), mais le maillage vient
// d'un fichier gmsh et les bords sont des *groupes physiques nommés* (entre
// guillemets) au lieu des quatre côtés d'un rectangle.
//
// Grammaire (un mot-clé par ligne, '#' = commentaire) :
//
//   mesh     file=<chemin.msh>
//   material E=<f> nu=<f>
//   solver   dense|sparse                       (défaut : sparse)
//
//   fix u|v [u|v] on "groupe" [where x|y in [<min>,<max>]]
//   fix u|v [u|v] at node <id>
//   fix u|v [u|v] at node nearest x=<f> y=<f>        (nœud le plus proche d'un point)
//   set u=<f>|v=<f>  on "groupe" [where ...]
//   set u=<f>|v=<f>  at node <id> | at node nearest x=<f> y=<f>
//
//   force    fx=<f> fy=<f> on "groupe" [where ...]   (force totale répartie)
//   force    fx=<f> fy=<f> at node <id>              (force ponctuelle, N)
//   traction tx=<f> ty=<f> on "groupe" [where ...]   (effort linéique, N/m)
//   pressure p=<f>         on "groupe" [where ...]   (pression normale, Pa ; p>0 = compression)
//
//   output <field> [file=..] [scale=..] [width=..] [margin=..] [bc] [mesh]
//
// field ∈ {von_mises, stress_xx, stress_yy, stress_xy, mesh, deformed}
// composante : u = horizontal (0), v = vertical (1)

#include "fem_tri.hpp"
#include "gmsh_reader.hpp"
#include "svg_tri.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>

// ---------------------------------------------------------------------------
// Représentation du problème
// ---------------------------------------------------------------------------
struct KidBC {
    enum class Where { Group, Node } where = Where::Group;
    std::string group; int node = -1;
    int comp; double value = 0.0;
    bool has_range = false; char axis = 'x'; double cmin = 0, cmax = 0;
    bool nearest = false; double nx = 0, ny = 0;   // 'at node nearest x=.. y=..'
};

struct KidLoad {
    enum class Kind { Force, Traction, Pressure } kind = Kind::Force;
    enum class Where { Group, Node } where = Where::Group;
    std::string group; int node = -1;
    double fx = 0, fy = 0;
    double p = 0;                                   // pression (Pa), pour Pressure
    bool has_range = false; char axis = 'x'; double cmin = 0, cmax = 0;
};

struct KidOutput {
    std::string field, file = "out.svg";
    double scale = 100, width = 700, margin = 0.08;
    bool bc = false, mesh = false;
};

struct KidSpec {
    std::string mesh_file;
    double E = 0, nu = 0;
    bool sparse = true;
    std::vector<KidBC> bcs;
    std::vector<KidLoad> loads;
    std::vector<KidOutput> outputs;
};

// ---------------------------------------------------------------------------
// Petits utilitaires de tokenisation
// ---------------------------------------------------------------------------
namespace kidio {

// Découpe une ligne en tokens, en gardant les chaînes "entre guillemets" en un
// seul token (guillemets retirés).
inline std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> out; std::string cur; bool inq = false;
    for (char ch : line) {
        if (ch == '"') { inq = !inq; continue; }
        if (!inq && std::isspace(static_cast<unsigned char>(ch))) {
            if (!cur.empty()) { out.push_back(cur); cur.clear(); }
        } else cur.push_back(ch);
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

inline bool split_kv(const std::string& tok, std::string& key, std::string& val) {
    auto p = tok.find('=');
    if (p == std::string::npos) return false;
    key = tok.substr(0, p); val = tok.substr(p + 1); return true;
}

[[noreturn]] inline void fail(int lineno, const std::string& msg) {
    throw std::runtime_error("ligne " + std::to_string(lineno) + " : " + msg);
}

// Analyse un suffixe « where x in [a,b] » à partir des tokens, à l'indice i.
inline void parse_where(const std::vector<std::string>& t, size_t i, int lineno,
                        bool& has_range, char& axis, double& cmin, double& cmax) {
    if (i >= t.size() || t[i] != "where") return;
    if (i + 2 >= t.size() || t[i + 2] != "in") fail(lineno, "syntaxe 'where <axe> in [min,max]'");
    axis = t[i + 1].empty() ? 'x' : t[i + 1][0];
    std::string rest;
    for (size_t k = i + 3; k < t.size(); ++k) rest += t[k];
    for (char& ch : rest) if (ch == '[' || ch == ']') ch = ' ';
    std::replace(rest.begin(), rest.end(), ',', ' ');
    std::istringstream iss(rest);
    if (!(iss >> cmin >> cmax)) fail(lineno, "intervalle [min,max] illisible");
    has_range = true;
}

} // namespace kidio

// ---------------------------------------------------------------------------
// Parsing
// ---------------------------------------------------------------------------
inline KidSpec parse_kid_problem(std::istream& in) {
    using namespace kidio;
    KidSpec spec; std::string line; int lineno = 0;
    bool has_mesh = false, has_material = false;

    while (std::getline(in, line)) {
        ++lineno;
        if (auto h = line.find('#'); h != std::string::npos) line = line.substr(0, h);
        auto t = tokenize(line);
        if (t.empty()) continue;
        const std::string& kw = t[0];

        if (kw == "mesh") {
            std::string k, v;
            for (size_t i = 1; i < t.size(); ++i)
                if (split_kv(t[i], k, v) && k == "file") spec.mesh_file = v;
            if (spec.mesh_file.empty()) fail(lineno, "mesh attend file=<chemin>");
            has_mesh = true;
        }
        else if (kw == "material") {
            std::string k, v;
            for (size_t i = 1; i < t.size(); ++i)
                if (split_kv(t[i], k, v)) {
                    if (k == "E") spec.E = std::stod(v);
                    else if (k == "nu") spec.nu = std::stod(v);
                }
            has_material = true;
        }
        else if (kw == "solver") {
            if (t.size() < 2) fail(lineno, "solver attend dense|sparse");
            spec.sparse = (t[1] != "dense");
        }
        else if (kw == "fix" || kw == "set") {
            // Composantes : 'u'/'v' (fix) ou 'u=val'/'v=val' (set), jusqu'à 'on'/'at'.
            std::vector<std::pair<int, double>> comps;
            size_t i = 1;
            for (; i < t.size() && t[i] != "on" && t[i] != "at"; ++i) {
                std::string c = t[i]; double val = 0.0;
                std::string k, v;
                if (split_kv(c, k, v)) { c = k; val = std::stod(v); }
                if (c == "u") comps.push_back({0, val});
                else if (c == "v") comps.push_back({1, val});
                else fail(lineno, "composante inconnue '" + c + "' (attendu u ou v)");
            }
            if (comps.empty()) fail(lineno, kw + " sans composante");
            if (i >= t.size()) fail(lineno, kw + " attend 'on \"groupe\"' ou 'at node <id>'");

            KidBC base;
            if (t[i] == "at") {
                if (i + 2 >= t.size() || t[i + 1] != "node") fail(lineno, "syntaxe 'at node <id>'");
                base.where = KidBC::Where::Node;
                if (t[i + 2] == "nearest") {
                    // 'at node nearest x=<f> y=<f>' : nœud le plus proche d'un point
                    base.nearest = true;
                    std::string k, v;
                    for (size_t j = i + 3; j < t.size(); ++j)
                        if (split_kv(t[j], k, v)) {
                            if (k == "x") base.nx = std::stod(v);
                            else if (k == "y") base.ny = std::stod(v);
                        }
                } else {
                    base.node = std::stoi(t[i + 2]);
                }
            } else { // "on"
                if (i + 1 >= t.size()) fail(lineno, "'on' attend un nom de groupe");
                base.where = KidBC::Where::Group; base.group = t[i + 1];
                parse_where(t, i + 2, lineno, base.has_range, base.axis, base.cmin, base.cmax);
            }
            for (auto [comp, val] : comps) {
                KidBC bc = base; bc.comp = comp; bc.value = val;
                spec.bcs.push_back(bc);
            }
        }
        else if (kw == "force" || kw == "traction") {
            KidLoad ld;
            ld.kind = (kw == "force") ? KidLoad::Kind::Force : KidLoad::Kind::Traction;
            size_t i = 1;
            std::string k, v;
            for (; i < t.size() && t[i] != "on" && t[i] != "at"; ++i)
                if (split_kv(t[i], k, v)) {
                    if (k == "fx" || k == "tx") ld.fx = std::stod(v);
                    else if (k == "fy" || k == "ty") ld.fy = std::stod(v);
                }
            if (i >= t.size()) fail(lineno, kw + " attend 'on \"groupe\"' ou 'at node <id>'");
            if (t[i] == "at") {
                if (i + 2 >= t.size() || t[i + 1] != "node") fail(lineno, "syntaxe 'at node <id>'");
                ld.where = KidLoad::Where::Node; ld.node = std::stoi(t[i + 2]);
            } else {
                if (i + 1 >= t.size()) fail(lineno, "'on' attend un nom de groupe");
                ld.where = KidLoad::Where::Group; ld.group = t[i + 1];
                parse_where(t, i + 2, lineno, ld.has_range, ld.axis, ld.cmin, ld.cmax);
            }
            spec.loads.push_back(ld);
        }
        else if (kw == "pressure") {
            // pression normale (Pa) sur un bord : p>0 = compression (vers l'intérieur)
            KidLoad ld; ld.kind = KidLoad::Kind::Pressure;
            size_t i = 1;
            std::string k, v;
            for (; i < t.size() && t[i] != "on"; ++i)
                if (split_kv(t[i], k, v) && k == "p") ld.p = std::stod(v);
            if (i >= t.size() || t[i] != "on" || i + 1 >= t.size())
                fail(lineno, "pressure attend 'on \"groupe\"'");
            ld.where = KidLoad::Where::Group; ld.group = t[i + 1];
            parse_where(t, i + 2, lineno, ld.has_range, ld.axis, ld.cmin, ld.cmax);
            spec.loads.push_back(ld);
        }
        else if (kw == "output") {
            if (t.size() < 2) fail(lineno, "output attend un champ");
            KidOutput o; o.field = t[1];
            std::string k, v;
            for (size_t i = 2; i < t.size(); ++i) {
                if (split_kv(t[i], k, v)) {
                    if (k == "file") o.file = v;
                    else if (k == "scale")  o.scale  = std::stod(v);
                    else if (k == "width")  o.width  = std::stod(v);
                    else if (k == "margin") o.margin = std::stod(v);
                } else if (t[i] == "bc")   o.bc   = true;
                else if (t[i] == "mesh")   o.mesh = true;
            }
            spec.outputs.push_back(o);
        }
        else fail(lineno, "mot-clé inconnu '" + kw + "'");
    }

    if (!has_mesh)     throw std::runtime_error("directive 'mesh' manquante");
    if (!has_material) throw std::runtime_error("directive 'material' manquante");
    return spec;
}

// ---------------------------------------------------------------------------
// Driver
// ---------------------------------------------------------------------------
inline std::string resolve_path(const std::string& base_dir, const std::string& p) {
    if (p.empty() || p[0] == '/') return p;
    return base_dir.empty() ? p : base_dir + "/" + p;
}

inline void require_group(const Mesh& mesh, const std::string& name) {
    if (mesh.has_group(name)) return;
    std::string avail;
    for (const auto& [g, _] : mesh.boundaries) avail += " \"" + g + "\"";
    throw std::runtime_error("groupe de bord \"" + name + "\" absent du maillage ;"
                             " groupes disponibles :" + (avail.empty() ? " (aucun)" : avail));
}

inline int nearest_node(const Mesh& mesh, double x, double y) {
    int best = 0; double best_d2 = std::numeric_limits<double>::max();
    for (int n = 0; n < mesh.n_nodes(); ++n) {
        double dx = mesh.nodes[n][0] - x, dy = mesh.nodes[n][1] - y;
        double d2 = dx * dx + dy * dy;
        if (d2 < best_d2) { best_d2 = d2; best = n; }
    }
    return best;
}

// Pour chaque arête (non orientée), le sommet opposé du triangle qui la porte.
// Une arête de bord n'appartient qu'à un seul triangle -> permet d'orienter sa
// normale vers l'extérieur du domaine (sens opposé au sommet opposé).
inline std::map<std::pair<int, int>, int> build_edge_apex(const Mesh& mesh) {
    std::map<std::pair<int, int>, int> apex;
    for (const auto& t : mesh.tris) {
        int v[3] = {t[0], t[1], t[2]};
        for (int e = 0; e < 3; ++e) {
            int a = v[e], b = v[(e + 1) % 3], c = v[(e + 2) % 3];
            apex[{std::min(a, b), std::max(a, b)}] = c;
        }
    }
    return apex;
}

// Normale unitaire sortante de l'arête (a, b), connaissant le sommet opposé.
inline std::array<double, 2> outward_normal(const Mesh& mesh, int a, int b, int apex) {
    const auto& pa = mesh.nodes[a]; const auto& pb = mesh.nodes[b];
    double dx = pb[0] - pa[0], dy = pb[1] - pa[1];
    double nx = dy, ny = -dx;                       // perpendiculaire à l'arête
    double mx = 0.5 * (pa[0] + pb[0]) - mesh.nodes[apex][0];
    double my = 0.5 * (pa[1] + pb[1]) - mesh.nodes[apex][1];
    if (nx * mx + ny * my < 0) { nx = -nx; ny = -ny; }   // pointe loin de l'intérieur
    double len = std::hypot(nx, ny);
    return { nx / len, ny / len };
}

inline void run_kid_problem(const KidSpec& spec, const std::string& base_dir = "") {
    Mesh mesh = read_gmsh(resolve_path(base_dir, spec.mesh_file));
    ElasticityFEM_T3 fem(mesh, spec.E, spec.nu);

    std::cout << std::string(72, '=') << "\n";
    std::cout << "kidFEM — " << mesh.n_nodes() << " noeuds, " << mesh.n_tris()
              << " triangles, " << fem.n_dof << " DDL\n";
    std::cout << "  materiau : E = " << std::scientific << std::setprecision(3)
              << spec.E << " Pa, nu = " << std::fixed << std::setprecision(3) << spec.nu << "\n";
    std::cout << "  groupes  :";
    for (const auto& [g, edges] : mesh.boundaries) std::cout << " \"" << g << "\"(" << edges.size() << ")";
    std::cout << "\n" << std::string(72, '=') << "\n\n";

    auto in_range = [](const KidBC& bc, const std::array<double, 2>& p) {
        if (!bc.has_range) return true;
        double q = (bc.axis == 'y') ? p[1] : p[0];
        return q >= bc.cmin - 1e-9 && q <= bc.cmax + 1e-9;
    };
    auto in_range_l = [](const KidLoad& ld, const std::array<double, 2>& p) {
        if (!ld.has_range) return true;
        double q = (ld.axis == 'y') ? p[1] : p[0];
        return q >= ld.cmin - 1e-9 && q <= ld.cmax + 1e-9;
    };

    std::vector<std::tuple<int, int, double>> node_bcs, nodal_forces;
    std::vector<int> support_nodes, load_nodes;
    auto register_node = [&](int n, double val) {
        (std::abs(val) < 1e-300 ? support_nodes : load_nodes).push_back(n);
    };

    // --- Conditions de Dirichlet ---
    for (const auto& bc : spec.bcs) {
        if (bc.where == KidBC::Where::Node) {
            int node = bc.nearest ? nearest_node(mesh, bc.nx, bc.ny) : bc.node;
            node_bcs.emplace_back(node, bc.comp, bc.value);
            register_node(node, bc.value);
        } else {
            require_group(mesh, bc.group);
            for (int n : mesh.group_nodes(bc.group))
                if (in_range(bc, mesh.nodes[n])) {
                    node_bcs.emplace_back(n, bc.comp, bc.value);
                    register_node(n, bc.value);
                }
        }
    }

    // --- Chargements de Neumann ---
    auto edge_apex = build_edge_apex(mesh);   // pour orienter les normales (pression)
    for (const auto& ld : spec.loads) {
        if (ld.where == KidLoad::Where::Node) {
            if (ld.fx != 0.0) nodal_forces.emplace_back(ld.node, 0, ld.fx);
            if (ld.fy != 0.0) nodal_forces.emplace_back(ld.node, 1, ld.fy);
            load_nodes.push_back(ld.node);
        } else {
            require_group(mesh, ld.group);
            if (ld.kind == KidLoad::Kind::Force) {
                // Force totale répartie également sur les nœuds sélectionnés.
                std::vector<int> ns;
                for (int n : mesh.group_nodes(ld.group))
                    if (in_range_l(ld, mesh.nodes[n])) ns.push_back(n);
                if (ns.empty()) continue;
                double fx = ld.fx / ns.size(), fy = ld.fy / ns.size();
                for (int n : ns) {
                    if (fx != 0.0) nodal_forces.emplace_back(n, 0, fx);
                    if (fy != 0.0) nodal_forces.emplace_back(n, 1, fy);
                    load_nodes.push_back(n);
                }
            } else if (ld.kind == KidLoad::Kind::Traction) {
                // Traction linéique (N/m) -> forces nodales cohérentes : chaque
                // arête de longueur L verse t·L/2 à chacun de ses deux nœuds.
                for (const auto& edge : mesh.boundaries.at(ld.group)) {
                    const auto& pa = mesh.nodes[edge[0]];
                    const auto& pb = mesh.nodes[edge[1]];
                    if (!in_range_l(ld, pa) || !in_range_l(ld, pb)) continue;
                    double L = std::hypot(pb[0] - pa[0], pb[1] - pa[1]);
                    double fx = ld.fx * L / 2.0, fy = ld.fy * L / 2.0;
                    for (int n : {edge[0], edge[1]}) {
                        if (fx != 0.0) nodal_forces.emplace_back(n, 0, fx);
                        if (fy != 0.0) nodal_forces.emplace_back(n, 1, fy);
                        load_nodes.push_back(n);
                    }
                }
            } else {
                // Pression normale (Pa). p>0 = compression : traction = -p·n_ext,
                // dirigée vers l'intérieur. Chaque arête verse t·L/2 à ses 2 nœuds.
                for (const auto& edge : mesh.boundaries.at(ld.group)) {
                    const auto& pa = mesh.nodes[edge[0]];
                    const auto& pb = mesh.nodes[edge[1]];
                    if (!in_range_l(ld, pa) || !in_range_l(ld, pb)) continue;
                    int apex = edge_apex.at({std::min(edge[0], edge[1]),
                                             std::max(edge[0], edge[1])});
                    auto nrm = outward_normal(mesh, edge[0], edge[1], apex);
                    double L = std::hypot(pb[0] - pa[0], pb[1] - pa[1]);
                    double fx = -ld.p * nrm[0] * L / 2.0;
                    double fy = -ld.p * nrm[1] * L / 2.0;
                    for (int n : {edge[0], edge[1]}) {
                        nodal_forces.emplace_back(n, 0, fx);
                        nodal_forces.emplace_back(n, 1, fy);
                        load_nodes.push_back(n);
                    }
                }
            }
        }
    }

    std::cout << "Assemblage et resolution (" << (spec.sparse ? "CG creux" : "Gauss dense") << ")...\n";
    fem.solve(node_bcs, nodal_forces, spec.sparse);

    // Résumé
    double u_max = 0, v_min = 0, v_max = 0, vm_max = 0;
    for (int n = 0; n < fem.n_nodes; ++n) {
        u_max = std::max(u_max, std::abs(fem.u[n]));
        v_min = std::min(v_min, fem.v[n]);
        v_max = std::max(v_max, fem.v[n]);
    }
    for (double vm : fem.e_vm) vm_max = std::max(vm_max, vm);

    std::cout << "\n" << std::string(72, '-') << "\n";
    std::cout << "Resultats :\n";
    std::cout << "  max |u|       : " << std::scientific << std::setprecision(3) << u_max << " m\n";
    std::cout << "  v (min, max)  : " << v_min << ", " << v_max << " m\n";
    std::cout << "  von Mises max : " << (vm_max / 1e6) << " MPa\n";
    std::cout << std::string(72, '-') << "\n\n";

    std::cout << "Export SVG...\n";
    for (const auto& o : spec.outputs) {
        SVGTri viz(fem);
        viz.width(o.width).margin(o.margin).deform_scale(o.scale);
        if (o.field == "deformed")      viz.show_deformed();
        else if (o.field != "mesh")     viz.field(o.field);
        if (o.mesh || o.field == "mesh") viz.show_mesh();
        if (o.bc) viz.boundary_conditions(support_nodes, load_nodes);
        viz.write(resolve_path(base_dir, o.file));
    }
    std::cout << "\nTermine.\n";
}

inline KidSpec parse_kid_problem_file(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Impossible d'ouvrir : " + path);
    return parse_kid_problem(f);
}
