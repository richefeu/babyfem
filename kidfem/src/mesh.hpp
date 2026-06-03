#pragma once
// mesh.hpp — maillage 2D non structuré (kidfem).
//
// Contrairement à babyfem (grille régulière implicite indexée par (i, j)),
// kidfem manipule un maillage *explicite* : une liste de nœuds (coordonnées)
// et une liste de triangles (connectivité). Les bords ne sont plus les quatre
// côtés d'un rectangle mais des *groupes nommés* (les groupes physiques d'un
// maillage gmsh), chacun étant un ensemble d'arêtes de bord.

#include <array>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <limits>

struct Mesh {
    std::vector<std::array<double, 2>> nodes;        // coordonnées (x, y), 0-based
    std::vector<std::array<int, 3>>    tris;         // connectivité des triangles
    // Groupes de bord nommés : nom -> arêtes (paires d'indices de nœuds)
    std::map<std::string, std::vector<std::array<int, 2>>> boundaries;

    int n_nodes() const { return static_cast<int>(nodes.size()); }
    int n_tris()  const { return static_cast<int>(tris.size()); }

    // Nœuds (uniques) appartenant à un groupe de bord.
    std::vector<int> group_nodes(const std::string& name) const {
        std::vector<int> ns;
        auto it = boundaries.find(name);
        if (it == boundaries.end()) return ns;
        for (const auto& e : it->second) { ns.push_back(e[0]); ns.push_back(e[1]); }
        std::sort(ns.begin(), ns.end());
        ns.erase(std::unique(ns.begin(), ns.end()), ns.end());
        return ns;
    }

    bool has_group(const std::string& name) const {
        return boundaries.find(name) != boundaries.end();
    }

    // Boîte englobante du maillage.
    void bbox(double& xmin, double& xmax, double& ymin, double& ymax) const {
        xmin = ymin =  std::numeric_limits<double>::max();
        xmax = ymax = -std::numeric_limits<double>::max();
        for (const auto& p : nodes) {
            xmin = std::min(xmin, p[0]); xmax = std::max(xmax, p[0]);
            ymin = std::min(ymin, p[1]); ymax = std::max(ymax, p[1]);
        }
    }
};
