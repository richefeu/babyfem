#!/usr/bin/env python3
"""
Exemples simples d'utilisation du solveur d'élasticité 2D.
Démontre les cas de chargement courants.
"""

import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import numpy as np
import matplotlib.pyplot as plt
from elasticity import ElasticityFEM2D, ElasticityVisualizer


def example_1_traction():
    """Exemple 1: Traction uniaxiale simple."""
    print("=" * 60)
    print("EXEMPLE 1: Traction uniaxiale")
    print("=" * 60)

    # Définir le problème
    width, height = 10.0, 5.0
    nx, ny = 31, 16
    E = 200e9      # Acier
    nu = 0.3

    print(f"Géométrie: {width}×{height} m")
    print(f"Discrétisation: {nx}×{ny} nœuds")
    print(f"Matériau: Acier (E={E:.0e} Pa, ν={nu})")

    # Créer le solveur
    problem = ElasticityFEM2D(width, height, nx, ny, E, nu)

    # Définir les conditions aux limites
    # Format: (component, value, [indices])
    # component: 0=u, 1=v
    boundary_conditions = {
        'bottom': [(1, 0.0)],           # v=0 sur toute la face bas
        'top': [(1, 0.02)],             # v=0.02 sur toute la face haut
        'left': [(0, 0.0)],             # u=0 sur toute la face gauche
        'right': None                   # Rien sur la face droite
    }

    print("\nConditions aux limites:")
    print("  - Bas: v = 0.0 m")
    print("  - Haut: v = 0.02 m")
    print("  - Gauche: u = 0.0 m")
    print("  - Droite: libre")

    # Résoudre
    print("\nRésolution...")
    problem.solve(boundary_conditions)
    problem.compute_stress()

    # Afficher les résultats
    print(f"\n✓ Problème résolu avec succès")
    print(f"  Max déplacement v: {np.max(problem.v):.6e} m")
    print(f"  Max contrainte σyy: {np.max(problem.stress_yy):.3e} Pa")
    print(f"  Min contrainte σyy: {np.min(problem.stress_yy):.3e} Pa")

    # Visualiser
    ElasticityVisualizer.plot_displacement_and_stress(problem, "Traction uniaxiale")
    ElasticityVisualizer.plot_deformed_geometry(problem, scale=100, title="Traction uniaxiale")
    ElasticityVisualizer.plot_von_mises(problem, "Traction uniaxiale")

    return problem


def example_2_cisaillement():
    """Exemple 2: Cisaillement avec déplacement sur portions."""
    print("\n" + "=" * 60)
    print("EXEMPLE 2: Cisaillement partiel")
    print("=" * 60)

    # Définir le problème
    width, height = 10.0, 5.0
    nx, ny = 31, 16
    E = 200e9
    nu = 0.3

    print(f"Géométrie: {width}×{height} m")
    print(f"Discrétisation: {nx}×{ny} nœuds")

    # Créer le solveur
    problem = ElasticityFEM2D(width, height, nx, ny, E, nu)

    # Portion de la face haut/bas (zone centrale 60%)
    i_start = int(nx * 0.2)
    i_end = int(nx * 0.8)
    indices_center = list(range(i_start, i_end))

    # Conditions aux limites
    boundary_conditions = {
        'bottom': [(1, -0.01, indices_center)],     # v=-0.01 sur portion
        'top': [(1, 0.01, indices_center)],         # v=0.01 sur portion
        'left': [(0, 0.0)],                         # u=0 sur face gauche
        'right': None
    }

    print("\nConditions aux limites:")
    print("  - Bas: v = -0.01 m (zone centrale)")
    print("  - Haut: v = +0.01 m (zone centrale)")
    print("  - Gauche: u = 0.0 m")
    print("  - Droite: libre")

    # Résoudre
    print("\nRésolution...")
    problem.solve(boundary_conditions)
    problem.compute_stress()

    # Afficher les résultats
    print(f"\n✓ Problème résolu avec succès")
    print(f"  Max déplacement v: {np.max(np.abs(problem.v)):.6e} m")
    print(f"  Max contrainte σxy: {np.max(np.abs(problem.stress_xy)):.3e} Pa")
    print(f"  Max contrainte von Mises: {np.max(problem.von_mises()):.3e} Pa")

    # Visualiser
    ElasticityVisualizer.plot_displacement_and_stress(problem, "Cisaillement partiel")
    ElasticityVisualizer.plot_deformed_geometry(problem, scale=150, title="Cisaillement partiel")
    ElasticityVisualizer.plot_stress_component(problem, 'xy', "Cisaillement partiel")

    return problem


def example_3_comparaison_poisson():
    """Exemple 3: Influence du coefficient de Poisson."""
    print("\n" + "=" * 60)
    print("EXEMPLE 3: Influence du coefficient de Poisson")
    print("=" * 60)

    width, height = 8.0, 4.0
    nx, ny = 31, 16
    E = 200e9
    disp_top = 0.015

    nu_values = [0.1, 0.3, 0.4]
    problems = {}

    fig, axes = plt.subplots(1, len(nu_values), figsize=(16, 5))
    fig.suptitle("Influence du coefficient de Poisson sur la contrainte von Mises",
                 fontsize=12, fontweight='bold')

    for nu, ax in zip(nu_values, axes):
        print(f"\nCas: ν = {nu}")
        problem = ElasticityFEM2D(width, height, nx, ny, E, nu)

        boundary_conditions = {
            'bottom': [(1, 0.0)],
            'top': [(1, disp_top)],
            'left': [(0, 0.0)],
            'right': None
        }

        problem.solve(boundary_conditions)
        problem.compute_stress()

        vm = problem.von_mises()
        problems[f'ν={nu}'] = problem

        print(f"  Max von Mises: {np.max(vm):.3e} Pa")

        X, Y = np.meshgrid(problem.x, problem.y)
        im = ax.contourf(X, Y, vm/1e9, levels=20, cmap='jet')
        ax.set_title(f'ν = {nu}')
        ax.set_xlabel('x (m)')
        ax.set_ylabel('y (m)')
        ax.set_aspect('equal')
        cbar = plt.colorbar(im, ax=ax)
        cbar.set_label('von Mises (GPa)')

    plt.tight_layout()

    return problems


if __name__ == "__main__":
    print("\n" + "=" * 60)
    print("EXEMPLES SIMPLES - SOLVEUR D'ÉLASTICITÉ 2D")
    print("=" * 60)

    # Exécuter les exemples
    prob1 = example_1_traction()
    prob2 = example_2_cisaillement()
    probs3 = example_3_comparaison_poisson()

    plt.show()

    print("\n" + "=" * 60)
    print("Tous les exemples ont été exécutés avec succès!")
    print("=" * 60)
