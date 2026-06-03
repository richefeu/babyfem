#pragma once
// problem_io.hpp — Lecture d'un problème babyFEM depuis un fichier texte.
//
// Le but pédagogique est de séparer les *données* (géométrie, matériau,
// conditions aux limites, sorties) du *moteur* de calcul. Un fichier texte
// décrit le problème ; ce module le lit dans une struct ProblemSpec, puis
// run_problem() construit le solveur, résout, et écrit les sorties SVG.
//
// Grammaire (un mot-clé par ligne, '#' = commentaire) :
//
//   mesh     width=<f> height=<f> nx=<i> ny=<i>
//   material E=<f> nu=<f>
//   solver   dense|sparse
//
//   fix u|v on <edge> [where x|y in [<min>,<max>]]
//   fix u|v at node <int>
//   set u|v=<f> on <edge> [where x|y in [<min>,<max>]]
//   set u|v=<f> at node <int>
//
//   output <field> [file=...] [scale=...] [width=...] [margin=...] [bc] [mesh]
//
// edge  ∈ {bottom, top, left, right}
// field ∈ {von_mises, stress_xx, stress_yy, stress_xy, mesh, deformed}
// composante : u = horizontal (0), v = vertical (1)

#include "fem_2d.hpp"
#include "svg_generator.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <iomanip>

// ---------------------------------------------------------------------------
// Structures de données décrivant un problème
// ---------------------------------------------------------------------------

struct BoundaryCondition {
    enum class Kind { Edge, Node };
    Kind kind = Kind::Edge;

    // Kind::Edge
    std::string edge;          // bottom / top / left / right
    bool   has_range = false;  // true si une clause "where ... in [a,b]" est donnée
    double cmin = 0.0, cmax = 0.0;

    // Kind::Node
    int node = 0;

    // Commun
    int    comp  = 0;          // 0 = u, 1 = v
    double value = 0.0;
};

struct Load {
    enum class Kind { Force, Traction };  // Force = ponctuelle (N) ; Traction = répartie (N/m)
    enum class Loc  { Node, Edge };
    Kind kind = Kind::Force;
    Loc  loc  = Loc::Node;

    // Loc::Node
    int node = 0;

    // Loc::Edge
    std::string edge;
    bool   has_range = false;
    double cmin = 0.0, cmax = 0.0;

    // Composantes : N (Force) ou N/m (Traction)
    double fx = 0.0, fy = 0.0;
};

struct OutputSpec {
    std::string field;         // von_mises / stress_xx / ... / mesh / deformed
    std::string file;          // nom du fichier SVG
    double scale  = 100.0;     // facteur d'amplification (champ "deformed")
    double width  = 600.0;     // largeur en pixels
    double margin = 0.05;      // marge relative
    bool   show_bc   = false;  // drapeau "bc"
    bool   show_mesh = false;  // drapeau "mesh"
};

struct ProblemSpec {
    double width = 0.0, height = 0.0;
    int    nx = 0, ny = 0;
    double E = 0.0, nu = 0.0;

    enum class Solver { Dense, Sparse };
    Solver solver = Solver::Sparse;

    std::vector<BoundaryCondition> bcs;
    std::vector<Load>              loads;
    std::vector<OutputSpec>        outputs;
};

// ---------------------------------------------------------------------------
// Helpers de parsing
// ---------------------------------------------------------------------------

namespace problem_io_detail {

inline std::runtime_error error(int line, const std::string& msg) {
    return std::runtime_error("Ligne " + std::to_string(line) + " : " + msg);
}

// Découpe une ligne en tokens séparés par des espaces, après suppression du
// commentaire ('#' jusqu'à la fin de ligne).
inline std::vector<std::string> tokenize(const std::string& raw) {
    std::string line = raw;
    auto hash = line.find('#');
    if (hash != std::string::npos) line.erase(hash);

    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string tok;
    while (iss >> tok) tokens.push_back(tok);
    return tokens;
}

// Extrait la valeur d'un token "clé=valeur". Renvoie true si la clé correspond.
inline bool split_kv(const std::string& tok, const std::string& key, std::string& out) {
    auto eq = tok.find('=');
    if (eq == std::string::npos) return false;
    if (tok.substr(0, eq) != key) return false;
    out = tok.substr(eq + 1);
    return true;
}

inline double to_double(int line, const std::string& s) {
    try {
        size_t pos = 0;
        double v = std::stod(s, &pos);
        if (pos != s.size()) throw std::invalid_argument(s);
        return v;
    } catch (...) {
        throw error(line, "nombre attendu, trouvé '" + s + "'");
    }
}

inline int to_int(int line, const std::string& s) {
    try {
        size_t pos = 0;
        int v = std::stoi(s, &pos);
        if (pos != s.size()) throw std::invalid_argument(s);
        return v;
    } catch (...) {
        throw error(line, "entier attendu, trouvé '" + s + "'");
    }
}

// Cherche un paramètre "clé=valeur" parmi les tokens [start..]. Renvoie true et
// remplit out si trouvé.
inline bool find_param(const std::vector<std::string>& t, size_t start,
                       const std::string& key, std::string& out) {
    for (size_t i = start; i < t.size(); ++i) {
        if (split_kv(t[i], key, out)) return true;
    }
    return false;
}

// Convertit "u"/"v" en composante 0/1.
inline int parse_comp(int line, const std::string& s) {
    if (s == "u") return 0;
    if (s == "v") return 1;
    throw error(line, "composante 'u' ou 'v' attendue, trouvé '" + s + "'");
}

// Analyse une clause "where <axis> in [<min>,<max>]" à partir du token d'indice
// `pos` (qui doit être "where"). Remplit has_range / cmin / cmax.
inline void parse_where(int line, const std::vector<std::string>& t, size_t pos,
                        bool& has_range, double& cmin, double& cmax) {
    // tokens attendus : where <axis> in [a, b]  (regroupés sans espace : [a,b])
    // On reconstruit la fin de ligne et on extrait les deux nombres entre crochets.
    std::string tail;
    for (size_t i = pos; i < t.size(); ++i) tail += t[i] + " ";

    auto lb = tail.find('[');
    auto rb = tail.find(']');
    if (lb == std::string::npos || rb == std::string::npos || rb < lb)
        throw error(line, "clause 'where' mal formée, attendu : where x in [min, max]");

    std::string inside = tail.substr(lb + 1, rb - lb - 1);
    std::replace(inside.begin(), inside.end(), ',', ' ');
    std::istringstream iss(inside);
    std::string a, b;
    if (!(iss >> a >> b))
        throw error(line, "deux bornes attendues dans 'where ... in [min, max]'");

    has_range = true;
    cmin = to_double(line, a);
    cmax = to_double(line, b);
}

// Analyse une condition aux limites (lignes "fix"/"set").
inline BoundaryCondition parse_bc(int line, const std::vector<std::string>& t,
                                  bool is_fix) {
    BoundaryCondition bc;

    // Token 1 : composante. Pour "set" c'est "u=val" ou "v=val" ; pour "fix"
    // c'est juste "u" ou "v" (valeur imposée = 0).
    if (t.size() < 2) throw error(line, "condition aux limites incomplète");

    if (is_fix) {
        bc.comp  = parse_comp(line, t[1]);
        bc.value = 0.0;
    } else {
        auto eq = t[1].find('=');
        if (eq == std::string::npos)
            throw error(line, "attendu 'set u=<val>' ou 'set v=<val>'");
        bc.comp  = parse_comp(line, t[1].substr(0, eq));
        bc.value = to_double(line, t[1].substr(eq + 1));
    }

    // Localisation : "on <edge> [where ...]" ou "at node <int>".
    if (t.size() < 3) throw error(line, "localisation manquante ('on' ou 'at node')");

    if (t[2] == "on") {
        if (t.size() < 4) throw error(line, "bord manquant après 'on'");
        bc.kind = BoundaryCondition::Kind::Edge;
        bc.edge = t[3];
        if (bc.edge != "bottom" && bc.edge != "top" &&
            bc.edge != "left"   && bc.edge != "right")
            throw error(line, "bord inconnu '" + bc.edge +
                              "' (attendu bottom/top/left/right)");
        // Clause optionnelle "where ..."
        for (size_t i = 4; i < t.size(); ++i) {
            if (t[i] == "where") { parse_where(line, t, i, bc.has_range, bc.cmin, bc.cmax); break; }
        }
    } else if (t[2] == "at") {
        if (t.size() < 5 || t[3] != "node")
            throw error(line, "attendu 'at node <int>'");
        bc.kind = BoundaryCondition::Kind::Node;
        bc.node = to_int(line, t[4]);
    } else {
        throw error(line, "attendu 'on <edge>' ou 'at node <int>', trouvé '" + t[2] + "'");
    }

    return bc;
}

// Analyse une charge de Neumann (lignes "force"/"traction").
//   force    fx=.. fy=.. at node <int> | on <edge> [where ...]
//   traction tx=.. ty=.. on <edge> [where ...]
inline Load parse_load(int line, const std::vector<std::string>& t,
                       Load::Kind kind) {
    Load ld;
    ld.kind = kind;
    const bool is_traction = (kind == Load::Kind::Traction);
    const char* cx = is_traction ? "tx" : "fx";
    const char* cy = is_traction ? "ty" : "fy";

    std::string v;
    if (find_param(t, 1, cx, v)) ld.fx = to_double(line, v);
    if (find_param(t, 1, cy, v)) ld.fy = to_double(line, v);
    if (ld.fx == 0.0 && ld.fy == 0.0)
        throw error(line, std::string("charge nulle : préciser ") + cx + "= et/ou " + cy + "=");

    // Localisation : "on <edge> [where ...]" ou "at node <int>" (force seulement).
    size_t on  = 0, at = 0;
    for (size_t i = 1; i < t.size(); ++i) {
        if (t[i] == "on") on = i;
        if (t[i] == "at") at = i;
    }

    if (on != 0) {
        if (on + 1 >= t.size()) throw error(line, "bord manquant après 'on'");
        ld.loc  = Load::Loc::Edge;
        ld.edge = t[on + 1];
        if (ld.edge != "bottom" && ld.edge != "top" &&
            ld.edge != "left"   && ld.edge != "right")
            throw error(line, "bord inconnu '" + ld.edge + "' (attendu bottom/top/left/right)");
        for (size_t i = on + 2; i < t.size(); ++i)
            if (t[i] == "where") { parse_where(line, t, i, ld.has_range, ld.cmin, ld.cmax); break; }
    } else if (at != 0) {
        if (is_traction)
            throw error(line, "une 'traction' s'applique sur un bord ('on <edge>'), pas sur un noeud");
        if (at + 2 >= t.size() || t[at + 1] != "node")
            throw error(line, "attendu 'at node <int>'");
        ld.loc  = Load::Loc::Node;
        ld.node = to_int(line, t[at + 2]);
    } else {
        throw error(line, is_traction
            ? "localisation manquante ('on <edge>')"
            : "localisation manquante ('on <edge>' ou 'at node <int>')");
    }

    return ld;
}

inline OutputSpec parse_output(int line, const std::vector<std::string>& t) {
    if (t.size() < 2) throw error(line, "champ manquant après 'output'");

    OutputSpec o;
    o.field = t[1];
    if (o.field != "von_mises" && o.field != "stress_xx" && o.field != "stress_yy" &&
        o.field != "stress_xy" && o.field != "mesh"      && o.field != "deformed")
        throw error(line, "champ inconnu '" + o.field + "'");

    std::string val;
    if (find_param(t, 2, "file",   val)) o.file   = val;
    if (find_param(t, 2, "scale",  val)) o.scale  = to_double(line, val);
    if (find_param(t, 2, "width",  val)) o.width  = to_double(line, val);
    if (find_param(t, 2, "margin", val)) o.margin = to_double(line, val);

    for (size_t i = 2; i < t.size(); ++i) {
        if (t[i] == "bc")   o.show_bc   = true;
        if (t[i] == "mesh") o.show_mesh = true;
    }

    if (o.file.empty()) o.file = o.field + ".svg";
    return o;
}

} // namespace problem_io_detail

// ---------------------------------------------------------------------------
// Parseur principal
// ---------------------------------------------------------------------------

inline ProblemSpec parse_problem(std::istream& in) {
    using namespace problem_io_detail;
    ProblemSpec spec;
    bool has_mesh = false, has_material = false;

    std::string raw;
    int line = 0;
    while (std::getline(in, raw)) {
        ++line;
        auto t = tokenize(raw);
        if (t.empty()) continue;

        const std::string& kw = t[0];

        if (kw == "mesh") {
            std::string v;
            if (!find_param(t, 1, "width",  v)) throw error(line, "mesh : 'width=' manquant");
            spec.width = to_double(line, v);
            if (!find_param(t, 1, "height", v)) throw error(line, "mesh : 'height=' manquant");
            spec.height = to_double(line, v);
            if (!find_param(t, 1, "nx", v)) throw error(line, "mesh : 'nx=' manquant");
            spec.nx = to_int(line, v);
            if (!find_param(t, 1, "ny", v)) throw error(line, "mesh : 'ny=' manquant");
            spec.ny = to_int(line, v);
            has_mesh = true;
        }
        else if (kw == "material") {
            std::string v;
            if (!find_param(t, 1, "E",  v)) throw error(line, "material : 'E=' manquant");
            spec.E = to_double(line, v);
            if (!find_param(t, 1, "nu", v)) throw error(line, "material : 'nu=' manquant");
            spec.nu = to_double(line, v);
            has_material = true;
        }
        else if (kw == "solver") {
            if (t.size() < 2) throw error(line, "solver : 'dense' ou 'sparse' attendu");
            if      (t[1] == "dense")  spec.solver = ProblemSpec::Solver::Dense;
            else if (t[1] == "sparse") spec.solver = ProblemSpec::Solver::Sparse;
            else throw error(line, "solveur inconnu '" + t[1] + "' (dense/sparse)");
        }
        else if (kw == "fix") {
            spec.bcs.push_back(parse_bc(line, t, /*is_fix=*/true));
        }
        else if (kw == "set") {
            spec.bcs.push_back(parse_bc(line, t, /*is_fix=*/false));
        }
        else if (kw == "force") {
            spec.loads.push_back(parse_load(line, t, Load::Kind::Force));
        }
        else if (kw == "traction") {
            spec.loads.push_back(parse_load(line, t, Load::Kind::Traction));
        }
        else if (kw == "output") {
            spec.outputs.push_back(parse_output(line, t));
        }
        else {
            throw error(line, "mot-clé inconnu '" + kw + "'");
        }
    }

    if (!has_mesh)     throw std::runtime_error("Directive 'mesh' manquante");
    if (!has_material) throw std::runtime_error("Directive 'material' manquante");

    return spec;
}

inline ProblemSpec parse_problem_file(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Impossible d'ouvrir le fichier : " + path);
    return parse_problem(f);
}

// ---------------------------------------------------------------------------
// Driver : construit le solveur, résout, écrit les sorties
// ---------------------------------------------------------------------------

inline void run_problem(const ProblemSpec& spec) {
    ElasticityFEM2D problem(spec.width, spec.height, spec.nx, spec.ny, spec.E, spec.nu);

    std::cout << std::string(72, '=') << "\n";
    std::cout << "babyFEM — " << spec.nx << " x " << spec.ny << " noeuds, "
              << problem.n_dof << " DDL\n";
    std::cout << "  geometrie : " << spec.width << " x " << spec.height << " m\n";
    std::cout << "  materiau  : E = " << std::scientific << std::setprecision(3)
              << spec.E << " Pa, nu = " << std::fixed << std::setprecision(3)
              << spec.nu << "\n";
    std::cout << std::string(72, '=') << "\n\n";

    // Traduire toutes les conditions aux limites en BC nodales.
    // On garde aussi la trace des nœuds "support" (valeur 0) et "charge"
    // (valeur != 0) pour la visualisation optionnelle.
    std::vector<std::tuple<int, int, double>> node_bcs;
    std::vector<int> support_nodes, load_nodes;

    auto register_node = [&](int node, double value) {
        if (std::abs(value) < 1e-300) support_nodes.push_back(node);
        else                          load_nodes.push_back(node);
    };

    for (const auto& bc : spec.bcs) {
        if (bc.kind == BoundaryCondition::Kind::Node) {
            node_bcs.emplace_back(bc.node, bc.comp, bc.value);
            register_node(bc.node, bc.value);
        } else {
            std::vector<int> nodes = bc.has_range
                ? problem.select_nodes_on_line(bc.edge, bc.cmin, bc.cmax)
                : problem.select_nodes_on_line(bc.edge);
            for (int n : nodes) {
                node_bcs.emplace_back(n, bc.comp, bc.value);
                register_node(n, bc.value);
            }
        }
    }

    // Traduire les charges de Neumann en forces nodales (node, comp, valeur en N).
    std::vector<std::tuple<int, int, double>> nodal_forces;

    auto add_force = [&](int node, double fx, double fy) {
        if (fx != 0.0) nodal_forces.emplace_back(node, 0, fx);
        if (fy != 0.0) nodal_forces.emplace_back(node, 1, fy);
        load_nodes.push_back(node);
    };

    for (const auto& ld : spec.loads) {
        if (ld.loc == Load::Loc::Node) {
            // Force ponctuelle : appliquée telle quelle.
            add_force(ld.node, ld.fx, ld.fy);
        } else {
            std::vector<int> nodes = ld.has_range
                ? problem.select_nodes_on_line(ld.edge, ld.cmin, ld.cmax)
                : problem.select_nodes_on_line(ld.edge);
            if (nodes.empty()) continue;

            if (ld.kind == Load::Kind::Force) {
                // Force totale répartie également sur les nœuds sélectionnés.
                double fx = ld.fx / nodes.size();
                double fy = ld.fy / nodes.size();
                for (int n : nodes) add_force(n, fx, fy);
            } else {
                // Traction répartie (N/m) -> forces nodales cohérentes.
                // Pas de maillage le long du bord : dx (bottom/top) ou dy (left/right).
                double h = (ld.edge == "left" || ld.edge == "right")
                           ? problem.dy : problem.dx;
                // Poids : h/2 aux extrémités de la sélection, h aux nœuds intérieurs
                // (résultante = traction × longueur couverte).
                for (size_t k = 0; k < nodes.size(); ++k) {
                    bool is_end = (k == 0 || k + 1 == nodes.size());
                    double w = is_end ? h / 2.0 : h;
                    add_force(nodes[k], ld.fx * w, ld.fy * w);
                }
            }
        }
    }

    std::map<std::string, std::vector<std::pair<int, double>>> empty_edge_bcs;

    std::cout << "Assemblage et resolution...\n";
    if (spec.solver == ProblemSpec::Solver::Dense)
        problem.solve(empty_edge_bcs, node_bcs, nodal_forces);
    else
        problem.solve_sparse(empty_edge_bcs, node_bcs, nodal_forces);

    std::cout << "Calcul des contraintes...\n";
    problem.compute_stress();

    // Résumé concis des résultats
    double u_max = 0.0, v_min = 0.0, v_max = 0.0;
    for (int i = 0; i < problem.n_nodes; ++i) {
        u_max = std::max(u_max, std::abs(problem.u[i]));
        v_min = std::min(v_min, problem.v[i]);
        v_max = std::max(v_max, problem.v[i]);
    }
    Matrix vm = problem.von_mises();
    double vm_max = 0.0;
    for (int j = 0; j < spec.ny; ++j)
        for (int i = 0; i < spec.nx; ++i)
            vm_max = std::max(vm_max, vm(j, i));

    std::cout << "\n" << std::string(72, '-') << "\n";
    std::cout << "Resultats :\n";
    std::cout << "  max |u|       : " << std::scientific << std::setprecision(3) << u_max << " m\n";
    std::cout << "  v (min, max)  : " << v_min << ", " << v_max << " m\n";
    std::cout << "  von Mises max : " << (vm_max / 1e9) << " GPa\n";
    std::cout << std::string(72, '-') << "\n\n";

    // Écriture des sorties
    std::cout << "Export SVG...\n";
    for (const auto& o : spec.outputs) {
        SVGVisualization viz(problem);
        viz.margin(o.margin).width(o.width);

        if      (o.field == "von_mises") viz.von_mises();
        else if (o.field == "stress_xx") viz.stress_xx();
        else if (o.field == "stress_yy") viz.stress_yy();
        else if (o.field == "stress_xy") viz.stress_xy();
        else if (o.field == "deformed")  viz.deform_scale(o.scale).deformed();
        // "mesh" : aucun champ, juste le maillage (géré ci-dessous)

        if (o.show_mesh || o.field == "mesh") viz.mesh();
        if (o.show_bc) viz.boundary_conditions(support_nodes, load_nodes);

        viz.write(o.file);
    }

    std::cout << "\nTermine.\n";
}
