#include "fem_2d.hpp"
#include "svg_generator.hpp"
#include <iostream>

int main() {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║     SOLVEUR FEM 2D - C++17 SANS BIBLIOTHÈQUE EXTERNE     ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    std::cout << "Exécutables disponibles:\n";
    std::cout << "  • ./example_traction   - Exemple de traction uniaxiale\n";
    std::cout << "  • ./example_shear      - Exemple de cisaillement\n\n";

    std::cout << "Usage:\n";
    std::cout << "  cd build && make\n";
    std::cout << "  ./example_traction\n";
    std::cout << "  ./example_shear\n\n";

    std::cout << "Structure C++17 (pur, sans dépendances):\n";
    std::cout << "  • matrix.hpp      - Classe Matrix (matrices denses)\n";
    std::cout << "  • solver.hpp      - Élimination Gauss + Gauss-Seidel\n";
    std::cout << "  • fem_2d.hpp      - Solveur FEM 2D complet\n";
    std::cout << "  • svg_generator.hpp - Export SVG pour visualisation\n\n";

    return 0;
}
