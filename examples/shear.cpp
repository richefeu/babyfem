#include "../src/fem_2d.hpp"
#include "../src/svg_generator.hpp"
#include <iostream>
#include <iomanip>
#include <cmath>

int main() {
    std::cout << std::string(70, '=') << "\n";
    std::cout << "SOLVEUR FEM 2D - C++17 - EXEMPLE 2: CISAILLEMENT\n";
    std::cout << std::string(70, '=') << "\n\n";

    double width = 10.0;
    double height = 5.0;
    int nx = 31;
    int ny = 16;
    double E = 200e9;
    double nu = 0.3;

    std::cout << "Géométrie: " << width << " × " << height << " m\n";
    std::cout << "Discrétisation: " << nx << " × " << ny << " nœuds\n";
    std::cout << "Matériau: Acier (E = " << std::scientific << E << " Pa, ν = "
              << std::fixed << nu << ")\n\n";

    ElasticityFEM2D problem(width, height, nx, ny, E, nu);

    // Cisaillement: déplacement en u sur portions top/bottom
    int i_start = nx / 5;      // 20% du début
    int i_end = 4 * nx / 5;    // 80% de la fin

    std::cout << "Conditions aux limites (cisaillement):\n";
    std::cout << "  - Bas: u = -0.01 m (zone x ∈ [" << i_start << ", " << i_end << "])\n";
    std::cout << "  - Haut: u = +0.01 m (zone x ∈ [" << i_start << ", " << i_end << "])\n";
    std::cout << "  - Gauche: v = 0.0 m\n";
    std::cout << "  - Droite: libre\n\n";

    std::map<std::string, std::vector<std::pair<int, double>>> bcs;
    bcs["bottom"] = {{0, -0.01}};    // u = -0.01 m en bas
    bcs["top"] = {{0, 0.01}};        // u = +0.01 m en haut
    bcs["left"] = {{1, 0.0}};        // v = 0.0 à gauche

    std::cout << "Résolution...\n";
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

    double sxy_max = 0.0;
    for (int j = 0; j < ny; ++j) {
        for (int i = 0; i < nx; ++i) {
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
    std::cout << "  σxy max: " << std::scientific << std::setprecision(2) << sxy_max / 1e9 << " GPa\n";
    std::cout << "  von Mises max: " << vm_max / 1e9 << " GPa\n";

    // Export
    std::cout << "\nExport SVG...\n";
    SVGGenerator::write_von_mises(problem, "shear_von_mises.svg");
    SVGGenerator::write_deformed(problem, "shear_deformed.svg", 100.0);

    std::cout << "\n✓ Cisaillement résolu!\n";
    std::cout << std::string(70, '=') << "\n";

    return 0;
}
