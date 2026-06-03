#pragma once
// gmsh_reader.hpp — lecture d'un maillage gmsh ASCII (format 2.2) -> Mesh.
//
// On ne supporte volontairement que le format 2.2 (le plus lisible). Pour
// l'obtenir depuis gmsh :  gmsh -2 plate.geo -format msh22 -o plate.msh
// (ou `Mesh.MshFileVersion = 2.2;` dans le .geo).
//
// Structure d'un .msh 2.2 :
//   $MeshFormat            2.2 0 8
//   $PhysicalNames         dim tag "nom"          -> noms des groupes
//   $Nodes                 id x y z
//   $Elements              id type ntags tags... noeuds...
// Types d'éléments utilisés :
//   1 = segment (2 nœuds)   -> arête de bord (rattachée à un groupe physique)
//   2 = triangle (3 nœuds)  -> élément de domaine
// Les autres types (points, quads, ...) sont ignorés : kidfem = triangles only.

#include "mesh.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

inline Mesh read_gmsh(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Impossible d'ouvrir le maillage : " + path);

    Mesh mesh;
    // (dim, tag) -> nom de groupe physique
    std::map<std::pair<int, int>, std::string> phys_names;
    // id gmsh (1-based, potentiellement non contigu) -> indice local 0-based
    std::unordered_map<int, int> id2local;

    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("$MeshFormat", 0) == 0) {
            std::getline(f, line);
            std::istringstream iss(line);
            std::string ver; iss >> ver;
            if (!ver.empty() && ver[0] != '2') {
                throw std::runtime_error(
                    "Format gmsh '" + ver + "' non supporté ; exportez en 2.2 "
                    "(gmsh ... -format msh22).");
            }
        }
        else if (line.rfind("$PhysicalNames", 0) == 0) {
            std::getline(f, line);
            int n = std::stoi(line);
            for (int k = 0; k < n; ++k) {
                std::getline(f, line);
                std::istringstream iss(line);
                int dim, tag; iss >> dim >> tag;
                // le nom est entre guillemets
                std::string name;
                std::getline(iss, name);
                auto a = name.find('"'), b = name.rfind('"');
                if (a != std::string::npos && b != std::string::npos && b > a)
                    name = name.substr(a + 1, b - a - 1);
                phys_names[{dim, tag}] = name;
            }
        }
        else if (line.rfind("$Nodes", 0) == 0) {
            std::getline(f, line);
            int n = std::stoi(line);
            mesh.nodes.reserve(n);
            for (int k = 0; k < n; ++k) {
                std::getline(f, line);
                std::istringstream iss(line);
                int id; double x, y, z; iss >> id >> x >> y >> z;
                id2local[id] = static_cast<int>(mesh.nodes.size());
                mesh.nodes.push_back({x, y});
            }
        }
        else if (line.rfind("$Elements", 0) == 0) {
            std::getline(f, line);
            int n = std::stoi(line);
            for (int k = 0; k < n; ++k) {
                std::getline(f, line);
                std::istringstream iss(line);
                int id, type, ntags;
                iss >> id >> type >> ntags;
                std::vector<int> tags(ntags);
                for (int t = 0; t < ntags; ++t) iss >> tags[t];
                int phys = (ntags >= 1) ? tags[0] : -1;

                if (type == 2) {                  // triangle de domaine
                    int a, b, c; iss >> a >> b >> c;
                    mesh.tris.push_back({id2local.at(a), id2local.at(b), id2local.at(c)});
                }
                else if (type == 1) {             // segment de bord
                    int a, b; iss >> a >> b;
                    auto it = phys_names.find({1, phys});
                    std::string name = (it != phys_names.end())
                                       ? it->second : ("phys_" + std::to_string(phys));
                    mesh.boundaries[name].push_back({id2local.at(a), id2local.at(b)});
                }
                // autres types ignorés
            }
        }
    }

    if (mesh.nodes.empty()) throw std::runtime_error("Maillage sans nœuds : " + path);
    if (mesh.tris.empty())  throw std::runtime_error("Maillage sans triangle : " + path);
    return mesh;
}
