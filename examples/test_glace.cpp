#include "../src/fem_2d.hpp"
#include "../src/svg_generator.hpp"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>

int main() {
    std::cout << std::string(80, '=') << "\n";
    std::cout << "SOLVEUR FEM 2D - C++17 - CAS TEST_GLACE: POUTRE APPUYÉE\n";
    std::cout << std::string(80, '=') << "\n\n";

    // Géométrie
    double L = 0.1;          // Longueur (m)
    double h = 0.02;         // Hauteur (m)
    int nx = 60 * 2;         // 120 nœuds en x
    int ny = 6 * 2;          // 12 nœuds en y
    int n_nodes = nx * ny;
    int n_dof = 2 * n_nodes;

    std::cout << "Géométrie:\n";
    std::cout << "  Longueur L: " << L << " m\n";
    std::cout << "  Hauteur h: " << h << " m\n";
    std::cout << "  Rapport: L/h = " << (L / h) << " (poutre mince)\n";
    std::cout << "  Discrétisation: " << nx << " × " << ny << " nœuds\n";
    std::cout << "  Total: " << n_nodes << " nœuds, " << n_dof << " DDLs\n\n";

    // Matériau
    double E = 210e9;        // Pa (acier)
    double nu = 0.3;         // Poisson

    std::cout << "Matériau:\n";
    std::cout << "  E = " << std::scientific << E << " Pa (acier)\n";
    std::cout << "  ν = " << std::fixed << nu << "\n\n";

    // Créer le problème
    ElasticityFEM2D problem(L, h, nx, ny, E, nu);

    // Conditions aux limites
    double Lappui = L / 3.0;
    double delta = -0.002;
    double depl = 0.0001;

    // Sélectionner les nœuds pour les appuis
    auto appui_gauche = problem.select_nodes_on_line("bottom", 0.0, Lappui);
    auto appui_droit = problem.select_nodes_on_line("bottom", L - Lappui, L);

    // Combiner les appuis
    std::vector<int> appuis = appui_gauche;
    appuis.insert(appuis.end(), appui_droit.begin(), appui_droit.end());

    // Sélectionner la zone de charge
    auto zone_charge = problem.select_nodes_on_line("top", Lappui + delta, L - Lappui - delta);

    std::cout << "Conditions aux limites:\n";
    std::cout << "  Appui gauche: nœuds [0, " << Lappui << "] en bas → v = 0\n";
    std::cout << "  Appui droit: nœuds [" << (L - Lappui) << ", " << L << "] en bas → v = 0\n";
    std::cout << "  Zone de charge: nœuds [" << (Lappui + delta) << ", " << (L - Lappui - delta)
              << "] en haut → v = -" << depl << "\n";
    std::cout << "  Blocage latéral: u = 0 au nœud (0,0)\n\n";

    std::cout << "Sélections:\n";
    std::cout << "  Appui gauche: " << appui_gauche.size() << " nœuds\n";
    std::cout << "  Appui droit: " << appui_droit.size() << " nœuds\n";
    std::cout << "  Total appuis: " << appuis.size() << " nœuds\n";
    std::cout << "  Zone de charge: " << zone_charge.size() << " nœuds\n";

    // CRITICAL: Vérifier que les zones ne sont pas vides
    if (appuis.empty() || zone_charge.empty()) {
        std::cerr << "ERREUR CRITIQUE: Appuis ou zone_charge est vide!\n";
        return 1;
    }

    std::cout << "  Premier appui: nœud " << appuis.front() << ", dernier: " << appuis.back() << "\n";
    std::cout << "  Première charge: nœud " << zone_charge.front() << ", dernière: " << zone_charge.back() << "\n\n";

    // Résoudre
    std::cout << "Assemblage et résolution...\n";
    // Conditions aux limites sur les bords
    std::map<std::string, std::vector<std::pair<int, double>>> bcs;
    // NOTE: Apply u=0 only at bottom-left corner to prevent rigid motion,
    // NOT on entire left edge (which would decouple u-v coupling)

    // Conditions aux limites sur nœuds spécifiques (node_idx, component, value)
    std::vector<std::tuple<int, int, double>> node_bcs;

    // FIX: Apply u=0 ONLY at bottom-left corner to prevent rigid motion
    // Applying on entire left edge destroys u-v coupling via penalty method
    node_bcs.push_back(std::make_tuple(0, 0, 0.0));  // u=0 at node (0,0)

    // Appuis: v = 0
    for (int node_idx : appuis) {
        node_bcs.push_back(std::make_tuple(node_idx, 1, 0.0));
    }

    // Zone de charge: v = -depl
    for (int node_idx : zone_charge) {
        node_bcs.push_back(std::make_tuple(node_idx, 1, -depl));
    }

    // Utiliser la méthode solve() du solveur avec les deux types de BC
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
            if (vm(j, i) > 1e6) {  // Compter les valeurs > 1 MPa
                vm_avg += vm(j, i);
                vm_nonzero++;
            }
        }
    }
    if (vm_nonzero > 0) vm_avg /= vm_nonzero;

    std::cout << "\nDéplacements:\n";
    std::cout << "  Max |u|: " << std::scientific << std::setprecision(3) << u_max << " m\n";
    std::cout << "  Max v: " << std::scientific << std::setprecision(3) << v_max << " m\n";
    std::cout << "  Min v: " << std::scientific << std::setprecision(3) << v_min << " m (flèche)\n";
    std::cout << "  Flèche max/L: " << std::fixed << std::setprecision(1)
              << (-v_min / L * 100) << " %\n";

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

    // Histogramme simple des contraintes von Mises
    std::cout << "\nHistogramme von Mises (par décile):\n";
    for (int decile = 0; decile <= 10; ++decile) {
        double threshold = vm_min + (vm_max - vm_min) * decile / 10.0;
        int count = 0;
        for (int j = 0; j < ny; ++j) {
            for (int i = 0; i < nx; ++i) {
                if (vm(j, i) >= threshold) count++;
            }
        }
        std::cout << "  > " << std::fixed << std::setprecision(2) << (threshold / 1e9)
                  << " GPa: " << count << " nœuds\n";
    }

    // Distribution détaillée par lignes (y)
    std::cout << "\nContrainte von Mises par ligne (y):\n";
    for (int j = ny - 1; j >= 0; j -= std::max(1, ny / 6)) {  // 6 lignes
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
    SVGGenerator::write_von_mises(problem, "test_glace_von_mises.svg");
    SVGGenerator::write_deformed(problem, "test_glace_deformed.svg", 500.0);

    std::cout << "\n✓ Cas test_glace résolu avec succès!\n";
    std::cout << std::string(80, '=') << "\n";

    return 0;
}
