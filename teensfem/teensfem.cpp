// teensfem — analyse de structures à barres/poutres 2D.
//
// Usage :  ./teensfem <fichier.txt>
//
// Voir src/problem_io.hpp pour la grammaire (noeuds, poutres avec extrémités
// encastrées/articulées, appuis, charges) et cases/ pour des exemples.

#include "src/problem_io.hpp"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage : " << argv[0] << " <fichier.txt>\n";
        return 1;
    }
    std::string path = argv[1], base_dir;
    if (auto p = path.find_last_of('/'); p != std::string::npos)
        base_dir = path.substr(0, p);
    try {
        TSpec spec = parse_teens_problem_file(path);
        run_teens_problem(spec, base_dir);
    } catch (const std::exception& e) {
        std::cerr << "Erreur : " << e.what() << "\n";
        return 1;
    }
    return 0;
}
