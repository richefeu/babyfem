#!/usr/bin/env python3
"""
Exemples d'utilisation des sélections de nœuds arbitraires.
Montre les différentes façons d'imposer des conditions aux limites.
"""

import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import numpy as np
import matplotlib.pyplot as plt
from elasticity import ElasticityFEM2D, ElasticityVisualizer, Mesh2D, explain_mesh


def example_1_charge_concentree():
    """Charge concentrée sur un petit groupe de nœuds."""
    print("\n" + "=" * 60)
    print("EXEMPLE 1: Charge concentrée sur un groupe de nœuds")
    print("=" * 60)

    # Créer le problème
    width, height = 10.0, 5.0
    nx, ny = 31, 16
    problem = ElasticityFEM2D(width, height, nx, ny, E=200e9, nu=0.3)

    # Créer le maillage pour visualiser
    mesh = Mesh2D(width, height, nx, ny)

    # Sélectionner une petite zone au centre du haut
    # Zone de x=4 à x=6, au sommet (y=height)
    top_nodes = mesh.select_nodes_on_line('top', x_min=4.0, x_max=6.0)

    print(f"Nombre de nœuds chargés: {len(top_nodes)}")
    print(f"Indices des nœuds: {top_nodes[:5]}... (affichage limité)")

    # Conditions aux limites: déplacement imposé sur la sélection
    bc = {
        'bottom': [(1, 0.0)],              # Bas: v=0
        'left': [(0, 0.0)],                # Gauche: u=0
        'nodes': [                          # Charge concentrée au centre haut
            (node, 1, 0.015) for node in top_nodes
        ]
    }

    problem.solve(bc)
    problem.compute_stress()

    print(f"Max déplacement v: {np.max(problem.v):.6e} m")
    print(f"Max contrainte von Mises: {np.max(problem.von_mises()):.3e} Pa")

    # Visualiser
    ElasticityVisualizer.plot_von_mises(problem, "Charge concentrée")
    mesh.plot_selected_nodes(top_nodes, "Zone chargée")

    return problem, mesh, top_nodes


def example_2_supports_multiples():
    """Plusieurs appuis ponctuels."""
    print("\n" + "=" * 60)
    print("EXEMPLE 2: Appuis ponctuels multiples")
    print("=" * 60)

    width, height = 10.0, 5.0
    nx, ny = 31, 16
    problem = ElasticityFEM2D(width, height, nx, ny, E=200e9, nu=0.3)
    mesh = Mesh2D(width, height, nx, ny)

    # Sélectionner 3 points d'appui au bas
    # Point 1: x=2
    # Point 2: x=5 (centre)
    # Point 3: x=8
    support_nodes = (
        mesh.select_nodes_on_line('bottom', x_min=1.8, x_max=2.2) +
        mesh.select_nodes_on_line('bottom', x_min=4.8, x_max=5.2) +
        mesh.select_nodes_on_line('bottom', x_min=7.8, x_max=8.2)
    )

    print(f"Nombre de nœuds d'appui: {len(support_nodes)}")

    # Charge au centre haut
    top_center = mesh.select_nodes_on_line('top', x_min=4.8, x_max=5.2)

    # Conditions aux limites
    bc = {
        'left': [(0, 0.0)],           # Bloqué horizontalement
        'nodes': [
            *[(node, 1, 0.0) for node in support_nodes],  # Appuis: v=0
            *[(node, 1, 0.01) for node in top_center]      # Charge: v=0.01
        ]
    }

    problem.solve(bc)
    problem.compute_stress()

    print(f"Max déplacement v: {np.max(problem.v):.6e} m")

    ElasticityVisualizer.plot_von_mises(problem, "Appuis multiples")
    mesh.plot_selected_nodes(support_nodes + top_center, "Appuis et charge")

    return problem, mesh


def example_3_region_circulaire():
    """Chargement sur une région circulaire."""
    print("\n" + "=" * 60)
    print("EXEMPLE 3: Chargement sur une région circulaire")
    print("=" * 60)

    width, height = 10.0, 5.0
    nx, ny = 31, 16
    problem = ElasticityFEM2D(width, height, nx, ny, E=200e9, nu=0.3)
    mesh = Mesh2D(width, height, nx, ny)

    # Sélectionner une région circulaire au centre
    center_x, center_y = width / 2, height / 2
    radius = 1.0
    circular_nodes = mesh.select_nodes_in_circle(center_x, center_y, radius)

    print(f"Nombre de nœuds dans la région circulaire: {len(circular_nodes)}")

    # Conditions aux limites
    bc = {
        'bottom': [(1, 0.0)],           # Bas: v=0
        'left': [(0, 0.0)],             # Gauche: u=0
        'nodes': [                       # Déplacement imposé dans la région
            (node, 1, 0.02) for node in circular_nodes
        ]
    }

    problem.solve(bc)
    problem.compute_stress()

    print(f"Max déplacement v: {np.max(problem.v):.6e} m")

    ElasticityVisualizer.plot_von_mises(problem, "Région circulaire")
    mesh.plot_selected_nodes(circular_nodes, "Région circulaire chargée")

    return problem, mesh


def example_4_region_rectangulaire():
    """Chargement sur une région rectangulaire."""
    print("\n" + "=" * 60)
    print("EXEMPLE 4: Chargement sur une région rectangulaire")
    print("=" * 60)

    width, height = 10.0, 5.0
    nx, ny = 31, 16
    problem = ElasticityFEM2D(width, height, nx, ny, E=200e9, nu=0.3)
    mesh = Mesh2D(width, height, nx, ny)

    # Région rectangulaire
    x_min, x_max = 3.0, 7.0
    y_min, y_max = height - 0.5, height
    rect_nodes = mesh.select_nodes_in_region(x_min, x_max, y_min, y_max)

    print(f"Nombre de nœuds dans la région rectangulaire: {len(rect_nodes)}")

    # Conditions aux limites
    bc = {
        'bottom': [(1, 0.0), (0, 0.0)],   # Bas: encastrement complet
        'left': [(0, 0.0)],
        'nodes': [
            (node, 1, 0.015) for node in rect_nodes  # Déplacement vertical
        ]
    }

    problem.solve(bc)
    problem.compute_stress()

    print(f"Max déplacement v: {np.max(problem.v):.6e} m")

    ElasticityVisualizer.plot_von_mises(problem, "Région rectangulaire")
    mesh.plot_selected_nodes(rect_nodes, "Région rectangulaire chargée")

    return problem, mesh


def example_5_custom_predicate():
    """Sélection personnalisée avec prédicat."""
    print("\n" + "=" * 60)
    print("EXEMPLE 5: Sélection personnalisée (prédicat)")
    print("=" * 60)

    width, height = 10.0, 5.0
    nx, ny = 31, 16
    problem = ElasticityFEM2D(width, height, nx, ny, E=200e9, nu=0.3)
    mesh = Mesh2D(width, height, nx, ny)

    # Sélection personnalisée: nœuds dans le triangle supérieur droit
    def in_triangle_upper_right(i, j, x, y):
        # Ligne de (0, height) à (width, 0)
        return x + y > width * 0.7

    custom_nodes = mesh.select_nodes_custom(in_triangle_upper_right)

    print(f"Nombre de nœuds sélectionnés: {len(custom_nodes)}")

    # Conditions aux limites
    bc = {
        'bottom': [(1, 0.0)],
        'left': [(0, 0.0)],
        'nodes': [
            (node, 1, 0.01) for node in custom_nodes  # Charge dans la région
        ]
    }

    problem.solve(bc)
    problem.compute_stress()

    print(f"Max déplacement v: {np.max(problem.v):.6e} m")

    ElasticityVisualizer.plot_von_mises(problem, "Sélection personnalisée")
    mesh.plot_selected_nodes(custom_nodes, "Sélection personnalisée")

    return problem, mesh


def example_6_explication_maillage():
    """Affiche l'explication complète du maillage."""
    print(explain_mesh())

    # Créer et afficher un exemple de maillage
    print("\nAffichage d'un maillage d'exemple...")
    mesh = Mesh2D(width=10, height=5, nx=11, ny=6)
    mesh.print_info()

    fig = mesh.plot_mesh("Exemple 11×6 nœuds")

    return mesh


if __name__ == "__main__":
    print("\n" + "=" * 60)
    print("EXEMPLES: SÉLECTION DE NŒUDS ARBITRAIRES")
    print("=" * 60)

    # Exécuter les exemples
    prob1, mesh1, nodes1 = example_1_charge_concentree()
    prob2, mesh2 = example_2_supports_multiples()
    prob3, mesh3 = example_3_region_circulaire()
    prob4, mesh4 = example_4_region_rectangulaire()
    prob5, mesh5 = example_5_custom_predicate()
    mesh6 = example_6_explication_maillage()

    plt.show()

    print("\n" + "=" * 60)
    print("Tous les exemples ont été exécutés avec succès!")
    print("=" * 60)
