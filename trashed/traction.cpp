#include "../src/fem_2d.hpp"
#include "../src/svg_generator.hpp"
#include <iostream>
#include <iomanip>
#include <cmath>

int main() {
    std::cout << std::string(70, '=') << "\n";
    std::cout << "SOLVEUR FEM 2D - C++17 - EXEMPLE 1: TRACTION UNIAXIALE\n";
    std::cout << std::string(70, '=') << "\n\n";

    // Paramètres du problème
    double width = 10.0;   // m
    double height = 5.0;   // m
    int nx = 31;           // nœuds en x
    int ny = 16;           // nœuds en y
    double E = 200e9;      // Pa (acier)
    double nu = 0.3;       // Poisson

    std::cout << "Géométrie: " << width << " × " << height << " m\n";
    std::cout << "Discrétisation: " << nx << " × " << ny << " nœuds\n";
    std::cout << "Total: " << nx * ny << " nœuds, " << 2 * nx * ny << " DDLs\n";
    std::cout << "Matériau: Acier (E = " << std::scientific << E << " Pa, ν = "
              << std::fixed << nu << ")\n\n";

    // Créer le problème
    ElasticityFEM2D problem(width, height, nx, ny, E, nu);

    // Conditions aux limites
    std::map<std::string, std::vector<std::pair<int, double>>> bcs;
    bcs["bottom"] = {{1, 0.0}};      // v = 0 en bas
    bcs["top"] = {{1, 0.02}};        // v = 0.02 m en haut (traction)
    bcs["left"] = {{0, 0.0}};        // u = 0 à gauche
    // "right" n'est pas contraint

    std::cout << "Conditions aux limites:\n";
    std::cout << "  - Bas (y=0): v = 0.0 m\n";
    std::cout << "  - Haut (y=" << height << "): v = 0.02 m\n";
    std::cout << "  - Gauche (x=0): u = 0.0 m\n";
    std::cout << "  - Droite: libre\n\n";

    // Résoudre
    std::cout << "Assemblage de la matrice de rigidité...\n";
    std::cout << "Résolution du système linéaire...\n";
    problem.solve(bcs);

    std::cout << "Calcul des contraintes...\n";
    problem.compute_stress();

    // Résultats
    std::cout << "\n" << std::string(70, '-') << "\n";
    std::cout << "RÉSULTATS\n";
    std::cout << std::string(70, '-') << "\n";

    double u_max = 0.0, v_max = 0.0;
    for (int i = 0; i < nx * ny; ++i) {
        u_max = std::max(u_max, std::abs(problem.u[i]));
        v_max = std::max(v_max, std::abs(problem.v[i]));
    }

    double sxx_max = 0.0, sxx_min = 1e308;
    double syy_max = 0.0, syy_min = 1e308;
    double sxy_max = 0.0;

    for (int j = 0; j < ny; ++j) {
        for (int i = 0; i < nx; ++i) {
            sxx_max = std::max(sxx_max, problem.stress_xx(j, i));
            sxx_min = std::min(sxx_min, problem.stress_xx(j, i));
            syy_max = std::max(syy_max, problem.stress_yy(j, i));
            syy_min = std::min(syy_min, problem.stress_yy(j, i));
            sxy_max = std::max(sxy_max, std::abs(problem.stress_xy(j, i)));
        }
    }

    Matrix vm = problem.von_mises();
    double vm_max = 0.0;
    for (int j = 0; j < ny; ++j) {
        for (int i = 0; i < nx; ++i) {
            vm_max = std::max(vm_max, vm(j, i));
        }
    }

    std::cout << "\nDéplacements:\n";
    std::cout << "  Max |u|: " << std::scientific << std::setprecision(3) << u_max << " m\n";
    std::cout << "  Max |v|: " << std::scientific << std::setprecision(3) << v_max << " m\n";

    std::cout << "\nContraintes:\n";
    std::cout << "  σxx: [" << std::scientific << std::setprecision(2) << sxx_min / 1e9
              << ", " << sxx_max / 1e9 << "] GPa\n";
    std::cout << "  σyy: [" << syy_min / 1e9 << ", " << syy_max / 1e9 << "] GPa\n";
    std::cout << "  σxy: [" << -sxy_max / 1e9 << ", " << sxy_max / 1e9 << "] GPa\n";
    std::cout << "  von Mises max: " << vm_max / 1e9 << " GPa\n";

    // Export SVG
    std::cout << "\nExport SVG...\n";
    SVGGenerator::write_von_mises(problem, "results_von_mises.svg");
    SVGGenerator::write_deformed(problem, "results_deformed.svg", 100.0);

    std::cout << "\n✓ Problème résolu avec succès!\n";
    std::cout << std::string(70, '=') << "\n";

    return 0;
}
