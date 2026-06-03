#pragma once
// problem_io.hpp — lecture d'un problème teensFEM depuis un fichier texte.
//
// Structure à barres/poutres 2D : on décrit la position des nœuds, les poutres
// (avec leurs extrémités encastrées ou articulées), les appuis (DDL bloqués) et
// les charges. Même esprit de fichier texte que les autres outils de la série.
//
// Grammaire (un mot-clé par ligne, '#' = commentaire) :
//
//   node <id> x=<f> y=<f>
//   section E=<f> A=<f> I=<f>                  # propriétés par défaut
//
//   beam <id1> <id2> [rigid=start|end|both|none]
//                    [E=<f>] [A=<f>] [I=<f>]   # surcharge de section
//                    [wx=<f>] [wy=<f>]         # charge répartie globale (N/m)
//                    [wn=<f>] [wt=<f>]         # charge répartie locale (N/m) :
//                                              # wn ⟂ barre, wt ∥ barre
//        # défaut : articulé-articulé (rotule aux 2 bouts = barre de treillis)
//        # rigid=both : encastré-encastré ; start/end : un seul bout encastré
//
//   fix u|v|r [u|v|r] at node <id>             # appui (u, v, r=rotation)
//   load fx=<f> fy=<f> m=<f> at node <id>      # force / moment nodal
//
//   output <field> [file=..] [scale=..] [width=..] [dscale=..]
//        # field ∈ {structure, deformed, moment, shear, normal}
//        # structure : schéma de chargement (liaisons, rotules, forces, moments,
//        #             charges réparties) ; scale : amplification de la déformée ;
//        #             dscale : échelle des diagrammes d'efforts

#include "frame.hpp"
#include "svg_frame.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <map>
#include <cctype>

// ---------------------------------------------------------------------------
struct TBeam {
    int id1, id2;
    bool rigid1 = false, rigid2 = false;
    double E = 0, A = 0, I = 0; bool hasE = false, hasA = false, hasI = false;
    double wx = 0, wy = 0;    // charge répartie, composantes GLOBALES (N/m)
    double wn = 0, wt = 0;    // charge répartie, repère LOCAL : wn perpendiculaire
                              // à la barre, wt le long de la barre (N/m)
};
struct TFix  { int node, comp; };
struct TLoad { int node; double fx = 0, fy = 0, m = 0; };
struct TOutput {
    std::string field, file = "out.svg";
    double scale = 0, width = 800, dscale = 0;   // scale/dscale = 0 -> auto
};
struct TSpec {
    double E = 0, A = 0, I = 0; bool has_section = false;
    std::map<int, std::array<double, 2>> nodes;   // id -> (x, y), conserve l'ordre via map
    std::vector<int> node_order;
    std::vector<TBeam> beams;
    std::vector<TFix> fixes;
    std::vector<TLoad> loads;
    std::vector<TOutput> outputs;
};

namespace teenio {

inline std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> out; std::istringstream iss(line); std::string t;
    while (iss >> t) out.push_back(t);
    return out;
}
inline bool split_kv(const std::string& tok, std::string& k, std::string& v) {
    auto p = tok.find('=');
    if (p == std::string::npos) return false;
    k = tok.substr(0, p); v = tok.substr(p + 1); return true;
}
[[noreturn]] inline void fail(int n, const std::string& msg) {
    throw std::runtime_error("ligne " + std::to_string(n) + " : " + msg);
}

} // namespace teenio

inline TSpec parse_teens_problem(std::istream& in) {
    using namespace teenio;
    TSpec spec; std::string line; int lineno = 0;

    while (std::getline(in, line)) {
        ++lineno;
        if (auto h = line.find('#'); h != std::string::npos) line = line.substr(0, h);
        auto t = tokenize(line);
        if (t.empty()) continue;
        const std::string& kw = t[0];
        std::string k, v;

        if (kw == "node") {
            if (t.size() < 2) fail(lineno, "node attend <id>");
            int id = std::stoi(t[1]); double x = 0, y = 0;
            for (size_t i = 2; i < t.size(); ++i)
                if (split_kv(t[i], k, v)) { if (k == "x") x = std::stod(v); else if (k == "y") y = std::stod(v); }
            if (!spec.nodes.count(id)) spec.node_order.push_back(id);
            spec.nodes[id] = {x, y};
        }
        else if (kw == "section") {
            for (size_t i = 1; i < t.size(); ++i)
                if (split_kv(t[i], k, v)) {
                    if (k == "E") spec.E = std::stod(v);
                    else if (k == "A") spec.A = std::stod(v);
                    else if (k == "I") spec.I = std::stod(v);
                }
            spec.has_section = true;
        }
        else if (kw == "beam") {
            if (t.size() < 3) fail(lineno, "beam attend <id1> <id2>");
            TBeam b; b.id1 = std::stoi(t[1]); b.id2 = std::stoi(t[2]);
            for (size_t i = 3; i < t.size(); ++i) {
                if (split_kv(t[i], k, v)) {
                    if (k == "rigid") {
                        if (v == "both")      { b.rigid1 = b.rigid2 = true; }
                        else if (v == "start") b.rigid1 = true;
                        else if (v == "end")   b.rigid2 = true;
                        else if (v == "none")  { b.rigid1 = b.rigid2 = false; }
                        else fail(lineno, "rigid attend start|end|both|none");
                    }
                    else if (k == "E") { b.E = std::stod(v); b.hasE = true; }
                    else if (k == "A") { b.A = std::stod(v); b.hasA = true; }
                    else if (k == "I") { b.I = std::stod(v); b.hasI = true; }
                    else if (k == "wx") b.wx = std::stod(v);
                    else if (k == "wy") b.wy = std::stod(v);
                    else if (k == "wn") b.wn = std::stod(v);
                    else if (k == "wt") b.wt = std::stod(v);
                }
            }
            spec.beams.push_back(b);
        }
        else if (kw == "fix") {
            std::vector<int> comps;
            size_t i = 1;
            for (; i < t.size() && t[i] != "at"; ++i) {
                if (t[i] == "u") comps.push_back(0);
                else if (t[i] == "v") comps.push_back(1);
                else if (t[i] == "r") comps.push_back(2);
                else fail(lineno, "composante inconnue '" + t[i] + "' (u, v ou r)");
            }
            if (i + 2 >= t.size() || t[i] != "at" || t[i + 1] != "node")
                fail(lineno, "syntaxe 'fix … at node <id>'");
            int id = std::stoi(t[i + 2]);
            for (int c : comps) spec.fixes.push_back({id, c});
        }
        else if (kw == "load") {
            TLoad ld; size_t i = 1;
            for (; i < t.size() && t[i] != "at"; ++i)
                if (split_kv(t[i], k, v)) {
                    if (k == "fx") ld.fx = std::stod(v);
                    else if (k == "fy") ld.fy = std::stod(v);
                    else if (k == "m") ld.m = std::stod(v);
                }
            if (i + 2 >= t.size() || t[i] != "at" || t[i + 1] != "node")
                fail(lineno, "syntaxe 'load … at node <id>'");
            ld.node = std::stoi(t[i + 2]);
            spec.loads.push_back(ld);
        }
        else if (kw == "output") {
            if (t.size() < 2) fail(lineno, "output attend un champ");
            TOutput o; o.field = t[1];
            for (size_t i = 2; i < t.size(); ++i)
                if (split_kv(t[i], k, v)) {
                    if (k == "file") o.file = v;
                    else if (k == "scale")  o.scale  = std::stod(v);
                    else if (k == "width")  o.width  = std::stod(v);
                    else if (k == "dscale") o.dscale = std::stod(v);
                }
            spec.outputs.push_back(o);
        }
        else fail(lineno, "mot-clé inconnu '" + kw + "'");
    }

    if (spec.nodes.empty()) throw std::runtime_error("aucun nœud défini");
    if (spec.beams.empty()) throw std::runtime_error("aucune poutre définie");
    return spec;
}

inline TSpec parse_teens_problem_file(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Impossible d'ouvrir : " + path);
    return parse_teens_problem(f);
}

// ---------------------------------------------------------------------------
// Driver
// ---------------------------------------------------------------------------
inline void run_teens_problem(const TSpec& spec, const std::string& base_dir = "") {
    // Indexation des nœuds (id du fichier -> indice 0-based, dans l'ordre vu)
    std::map<int, int> idx;
    Frame fr;
    for (int id : spec.node_order) {
        idx[id] = fr.n_nodes();
        fr.nodes.push_back({spec.nodes.at(id)[0], spec.nodes.at(id)[1]});
    }
    auto need = [&](int id) -> int {
        auto it = idx.find(id);
        if (it == idx.end()) throw std::runtime_error("nœud " + std::to_string(id) + " inconnu");
        return it->second;
    };

    for (const auto& b : spec.beams) {
        FrameMember m;
        m.n1 = need(b.id1); m.n2 = need(b.id2);
        m.rigid1 = b.rigid1; m.rigid2 = b.rigid2;
        m.E = b.hasE ? b.E : spec.E;
        m.A = b.hasA ? b.A : spec.A;
        m.I = b.hasI ? b.I : spec.I;
        m.wx = b.wx; m.wy = b.wy;
        // Charge donnée en repère local (wn ⟂ barre, wt ∥ barre) -> on la
        // convertit en composantes globales et on l'ajoute à wx/wy.
        if (b.wn != 0 || b.wt != 0) {
            double dx = fr.nodes[m.n2].x - fr.nodes[m.n1].x;
            double dy = fr.nodes[m.n2].y - fr.nodes[m.n1].y;
            double L = std::hypot(dx, dy), c = dx / L, s = dy / L;
            m.wx += b.wt * c - b.wn * s;   // tangente (c,s) + normale gauche (-s,c)
            m.wy += b.wt * s + b.wn * c;
        }
        if (m.E <= 0 || m.A <= 0 || m.I <= 0)
            throw std::runtime_error("poutre " + std::to_string(b.id1) + "-" +
                std::to_string(b.id2) + " sans section (définir 'section E A I' ou surcharger)");
        fr.members.push_back(m);
    }
    for (const auto& f : spec.fixes)  fr.supports.emplace_back(need(f.node), f.comp, 0.0);
    for (const auto& l : spec.loads) {
        int n = need(l.node);
        if (l.fx != 0) fr.nodal_loads.emplace_back(n, 0, l.fx);
        if (l.fy != 0) fr.nodal_loads.emplace_back(n, 1, l.fy);
        if (l.m  != 0) fr.nodal_loads.emplace_back(n, 2, l.m);
    }

    std::cout << std::string(72, '=') << "\n";
    std::cout << "teensFEM — " << fr.n_nodes() << " noeuds, " << fr.members.size()
              << " poutres, " << fr.n_dof() << " DDL\n";
    std::cout << std::string(72, '=') << "\n\n";
    std::cout << "Resolution...\n";
    fr.solve();

    // Résumé
    double dmax = 0, mmax = 0, nmax = 0;
    for (int n = 0; n < fr.n_nodes(); ++n)
        dmax = std::max(dmax, std::hypot(fr.d[Frame::dof_u(n)], fr.d[Frame::dof_v(n)]));
    for (size_t e = 0; e < fr.members.size(); ++e) {
        double L, c, s; fr.geom(fr.members[e], L, c, s);
        for (int k = 0; k <= 20; ++k) {
            double N, V, M; fr.internal_at((int)e, L * k / 20.0, N, V, M);
            mmax = std::max(mmax, std::abs(M)); nmax = std::max(nmax, std::abs(N));
        }
    }
    std::cout << "\n" << std::string(72, '-') << "\n";
    std::cout << "Resultats :\n";
    std::cout << std::scientific << std::setprecision(3);
    std::cout << "  deplacement max : " << dmax << " m\n";
    std::cout << "  |M| max         : " << mmax << " N.m\n";
    std::cout << "  |N| max         : " << nmax << " N\n";
    std::cout << std::string(72, '-') << "\n\n";

    std::cout << "Export SVG...\n";
    for (const auto& o : spec.outputs) {
        std::string file = (o.file.size() && o.file[0] != '/' && !base_dir.empty())
                           ? base_dir + "/" + o.file : o.file;
        SVGFrame viz(fr);
        viz.width(o.width);
        if (o.scale  > 0) viz.deform_scale(o.scale);
        if (o.dscale > 0) viz.diagram_scale(o.dscale);
        viz.render(o.field, file);
    }
    std::cout << "\nTermine.\n";
}
