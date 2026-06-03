// babyfem — driver générique : lit un problème depuis un fichier texte,
// le résout et écrit les sorties demandées.
//
// Usage :  ./babyfem <fichier.txt>
//
// Voir src/problem_io.hpp pour la grammaire du fichier, et cases/
// pour des exemples complets.

#include "src/problem_io.hpp"

#include <iostream>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage : " << argv[0] << " <fichier.txt>\n";
        return 1;
    }

    try {
        ProblemSpec spec = parse_problem_file(argv[1]);
        run_problem(spec);
    } catch (const std::exception& e) {
        std::cerr << "Erreur : " << e.what() << "\n";
        return 1;
    }

    return 0;
}
