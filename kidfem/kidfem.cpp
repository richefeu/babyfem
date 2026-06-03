// kidfem — driver générique sur maillage triangulaire (gmsh).
//
// Usage :  ./kidfem <fichier.txt>
//
// Voir src/problem_io.hpp pour la grammaire du fichier, et cases/ pour des
// exemples complets (le maillage est un .msh gmsh au format 2.2).

#include "src/problem_io.hpp"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage : " << argv[0] << " <fichier.txt>\n";
        return 1;
    }
    std::string path = argv[1];
    std::string base_dir;
    if (auto p = path.find_last_of('/'); p != std::string::npos)
        base_dir = path.substr(0, p);

    try {
        KidSpec spec = parse_kid_problem_file(path);
        run_kid_problem(spec, base_dir);
    } catch (const std::exception& e) {
        std::cerr << "Erreur : " << e.what() << "\n";
        return 1;
    }
    return 0;
}
