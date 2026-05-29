#include "../src/fem_2d.hpp"
#include "../src/svg_generator.hpp"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>

int main() {
    std::cout << std::string(80, '=') << "\n";
    std::cout << "SOLVEUR FEM 2D - C++17 - CAS: POUTRE 3 APPUIS AVEC CHARGES PONCTUELLES\n";
    std::cout << std::string(80, '=') << "\n\n";

    // Géométrie: Poutre longue et mince sur 3 appuis
    double L = 1.0;           // Longueur 1.0 m (longue)
    double h = 0.1;           // Hauteur 0.1 m (mince)
    int nx = 100, ny = 10;    // Discrétisation
    int n_nodes = nx * ny;
    int n_dof = 2 * n_nodes;

    std::cout << "Géométrie:\n";
    std::cout << "  Longueur L: " << L << " m\n";
    std::cout << "  Hauteur h: " << h << " m\n";
    std::cout << "  Rapport: L/h = " << (L / h) << " (poutre mince)\n";
    std::cout << "  Discrétisation: " << nx << " × " << ny << " nœuds\n";
    std::cout << "  Total: " << n_nodes << " nœuds, " << n_dof << " DDLs\n\n";

    // Matériau
    double E = 210e9;         // Pa (acier)
    double nu = 0.3;          // Poisson

    std::cout << "Matériau:\n";
    std::cout << "  E = " << std::scientific << E << " Pa (acier)\n";
    std::cout << "  ν = " << std::fixed << nu << "\n\n";

    // Créer le problème
    ElasticityFEM2D problem(L, h, nx, ny, E, nu);

    // Conditions aux limites
    // 3 appuis simples (v=0) à x = 0, 0.5, 1.0
    double x_appui_1 = 0.0;
    double x_appui_2 = L / 2.0;
    double x_appui_3 = L;
    double appui_tol = L / (nx - 1) * 1.5;  // tolerance for node selection

    auto appui_1 = problem.select_nodes_on_line("bottom", x_appui_1 - appui_tol, x_appui_1 + appui_tol);
    auto appui_2 = problem.select_nodes_on_line("bottom", x_appui_2 - appui_tol, x_appui_2 + appui_tol);
    auto appui_3 = problem.select_nodes_on_line("bottom", x_appui_3 - appui_tol, x_appui_3 + appui_tol);

    // Zones de charge: au milieu de chaque travée
    // Travée 1: x ∈ [0, 0.5] → charge à x = 0.25
    // Travée 2: x ∈ [0.5, 1.0] → charge à x = 0.75
    double x_charge_1 = L / 4.0;
    double x_charge_2 = 3.0 * L / 4.0;
    double delta = 0.02;  // Largeur de la zone de charge

    auto zone_charge_1 = problem.select_nodes_on_line("top", x_charge_1 - delta, x_charge_1 + delta);
    auto zone_charge_2 = problem.select_nodes_on_line("top", x_charge_2 - delta, x_charge_2 + delta);

    std::cout << "Conditions aux limites:\n";
    std::cout << "  Appui 1 (x=" << x_appui_1 << "): " << appui_1.size() << " nœuds → v = 0\n";
    std::cout << "  Appui 2 (x=" << x_appui_2 << "): " << appui_2.size() << " nœuds → v = 0\n";
    std::cout << "  Appui 3 (x=" << x_appui_3 << "): " << appui_3.size() << " nœuds → v = 0\n";
    std::cout << "  Charge 1 (x=" << x_charge_1 << "): " << zone_charge_1.size() << " nœuds → v = -0.0002 m\n";
    std::cout << "  Charge 2 (x=" << x_charge_2 << "): " << zone_charge_2.size() << " nœuds → v = -0.0002 m\n";
    std::cout << "  Blocage latéral: u = 0 au nœud (0,0)\n\n";

    // Assembler les BCs
    std::vector<std::tuple<int, int, double>> node_bcs;

    // Pincer au coin bas-gauche (prévention du mouvement rigide)
    node_bcs.push_back(std::make_tuple(0, 0, 0.0));

    // Appuis simples: v = 0
    for (int node_idx : appui_1) {
        node_bcs.push_back(std::make_tuple(node_idx, 1, 0.0));
    }
    for (int node_idx : appui_2) {
        node_bcs.push_back(std::make_tuple(node_idx, 1, 0.0));
    }
    for (int node_idx : appui_3) {
        node_bcs.push_back(std::make_tuple(node_idx, 1, 0.0));
    }

    // Charges ponctuelles: v = -0.0002 m
    double depl = 0.0002;
    for (int node_idx : zone_charge_1) {
        node_bcs.push_back(std::make_tuple(node_idx, 1, -depl));
    }
    for (int node_idx : zone_charge_2) {
        node_bcs.push_back(std::make_tuple(node_idx, 1, -depl));
    }

    // Résoudre
    std::cout << "Assemblage et résolution...\n";
    std::map<std::string, std::vector<std::pair<int, double>>> bcs;
    problem.solve(bcs, node_bcs);

    std::cout << "Calcul des contraintes...\n";
    problem.compute_stress();

    // Résultats
    std::cout << "\n" << std::string(80, '-') << "\n";
    std::cout << "RÉSULTATS\n";
    std::cout << std::string(80, '-') << "\n";

    double u_max = 0.0, v_max = 0.0, v_min = 0.0;
    for (int i = 0; i < n_nodes; ++i) {
        u_max = std::max(u_max, std::abs(problem.u[i]));
        v_max = std::max(v_max, problem.v[i]);
        v_min = std::min(v_min, problem.v[i]);
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
    double vm_max = 0.0, vm_min = 1e308, vm_avg = 0.0;
    int vm_nonzero = 0;

    for (int j = 0; j < ny; ++j) {
        for (int i = 0; i < nx; ++i) {
            vm_max = std::max(vm_max, vm(j, i));
            vm_min = std::min(vm_min, vm(j, i));
            if (vm(j, i) > 1e6) {
                vm_avg += vm(j, i);
                vm_nonzero++;
            }
        }
    }
    if (vm_nonzero > 0) vm_avg /= vm_nonzero;

    std::cout << "\nDéplacements:\n";
    std::cout << "  Max |u|: " << std::scientific << std::setprecision(3) << u_max << " m\n";
    std::cout << "  Max v: " << v_max << " m\n";
    std::cout << "  Min v: " << v_min << " m (flèche max)\n";

    std::cout << "\nContraintes - Détail:\n";
    std::cout << "  σxx: [" << std::scientific << std::setprecision(2) << (sxx_min / 1e9)
              << ", " << (sxx_max / 1e9) << "] GPa\n";
    std::cout << "  σyy: [" << (syy_min / 1e9) << ", " << (syy_max / 1e9) << "] GPa\n";
    std::cout << "  σxy max: " << (sxy_max / 1e9) << " GPa\n";

    std::cout << "\nVon Mises - Distribution:\n";
    std::cout << "  Min: " << std::scientific << (vm_min / 1e9) << " GPa\n";
    std::cout << "  Max: " << (vm_max / 1e9) << " GPa\n";
    std::cout << "  Moy (non-zéro): " << (vm_avg / 1e9) << " GPa\n";
    std::cout << "  Points > 1 MPa: " << vm_nonzero << " / " << (nx*ny) << "\n";

    // Distribution par lignes (y)
    std::cout << "\nContrainte von Mises par ligne (y):\n";
    for (int j = ny - 1; j >= 0; j -= std::max(1, ny / 5)) {
        double max_on_line = 0.0;
        for (int i = 0; i < nx; ++i) {
            max_on_line = std::max(max_on_line, vm(j, i));
        }
        double y_pos = j * (h / (ny - 1));
        std::cout << "  y=" << std::fixed << std::setprecision(4) << y_pos
                  << " m: max σ = " << std::scientific << (max_on_line / 1e9) << " GPa\n";
    }

    // Export SVG
    std::cout << "\nExport SVG...\n";
    SVGGenerator::write_von_mises(problem, "three_span_von_mises.svg");
    SVGGenerator::write_deformed(problem, "three_span_deformed.svg", 500.0);

    std::cout << "\n✓ Cas three_span_beam résolu avec succès!\n";
    std::cout << std::string(80, '=') << "\n";

    return 0;
}
