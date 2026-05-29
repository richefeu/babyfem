#!/usr/bin/env python3
"""
Exemples avancés d'utilisation du solveur d'élasticité 2D.
"""

import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import numpy as np
import matplotlib.pyplot as plt
from elasticity import ElasticityFEM2D, ElasticityVisualizer


def exemple_poutre_encastree():
    """Poutre encastrée avec charge en flexion."""
    print("Exemple: Poutre encastrée en flexion")
    print("-" * 50)

    L, h = 4.0, 0.4
    nx, ny = 51, 11
    E = 210e9
    nu = 0.3

    problem = ElasticityFEM2D(L, h, nx, ny, E, nu)

    boundary_conditions = {
        'bottom': [(1, 0.0)],
        'top': [(1, 0.015)],
        'left': [(0, 0.0)],
        'right': None
    }

    problem.solve(boundary_conditions)
    problem.compute_stress()

    vm = problem.von_mises()

    print(f"  Dimensions: {L}m × {h}m (ratio L/h = {L/h:.1f})")
    print(f"  Discrétisation: {nx}×{ny}")
    print(f"  Déplacement imposé: 0.015m")
    print(f"  Contrainte von Mises max: {np.max(vm):.2e} Pa")
    print(f"  Flèche relative (v_max/L): {np.max(problem.v)/L:.4f}")

    ElasticityVisualizer.plot_displacement_and_stress(problem, "Poutre en flexion")
    ElasticityVisualizer.plot_deformed_geometry(problem, scale=50, title="Poutre en flexion")

    return problem


def exemple_etude_convergence():
    """Étude de convergence en fonction du maillage."""
    print("\nExemple: Étude de convergence du maillage")
    print("-" * 50)

    width, height = 10.0, 5.0
    E = 200e9
    nu = 0.3
    disp = 0.02

    resolutions = [(11, 6), (21, 11), (41, 21), (81, 41)]
    resultats = []

    for nx, ny in resolutions:
        problem = ElasticityFEM2D(width, height, nx, ny, E, nu)

        boundary_conditions = {
            'bottom': [(1, 0.0)],
            'top': [(1, disp)],
            'left': [(0, 0.0)],
            'right': None
        }

        problem.solve(boundary_conditions)
        problem.compute_stress()

        stress_yy_center = problem.stress_yy[ny//2, nx//2]
        resultats.append({
            'nelem': (nx-1) * (ny-1),
            'nx': nx,
            'ny': ny,
            'stress': stress_yy_center,
            'max_stress': np.max(problem.stress_yy)
        })

        print(f"  Grille {nx:2d}×{ny:2d} ({(nx-1)*(ny-1):4d} éléments): "
              f"σyy_center={stress_yy_center/1e9:.4f} GPa")

    # Graphique de convergence
    fig, ax = plt.subplots(figsize=(10, 6))

    nelems = [r['nelem'] for r in resultats]
    stress_vals = [r['stress'] for r in resultats]

    ax.loglog(nelems, stress_vals, 'o-', linewidth=2, markersize=8, label='σyy au centre')
    ax.set_xlabel('Nombre d\'éléments')
    ax.set_ylabel('Contrainte σyy (Pa)')
    ax.set_title('Étude de convergence du maillage')
    ax.grid(True, which='both', alpha=0.3)
    ax.legend()

    plt.tight_layout()

    return resultats


def exemple_comparaison_matieres():
    """Comparaison de différents matériaux."""
    print("\nExemple: Comparaison de matériaux")
    print("-" * 50)

    width, height = 10.0, 5.0
    nx, ny = 31, 16
    disp = 0.02

    materiaux = {
        'Acier': (210e9, 0.3),
        'Aluminium': (70e9, 0.33),
        'Titane': (100e9, 0.34),
    }

    problems = {}

    fig, axes = plt.subplots(1, 3, figsize=(16, 5))
    fig.suptitle("Comparaison de matériaux", fontsize=12, fontweight='bold')

    for (nom, (E, nu)), ax in zip(materiaux.items(), axes):
        problem = ElasticityFEM2D(width, height, nx, ny, E, nu)

        boundary_conditions = {
            'bottom': [(1, 0.0)],
            'top': [(1, disp)],
            'left': [(0, 0.0)],
            'right': None
        }

        problem.solve(boundary_conditions)
        problem.compute_stress()

        vm = problem.von_mises()
        problems[nom] = problem

        X, Y = np.meshgrid(problem.x, problem.y)
        im = ax.contourf(X, Y, vm/1e9, levels=20, cmap='jet')
        ax.set_title(f'{nom}\nE={E/1e9:.0f}GPa')
        ax.set_xlabel('x (m)')
        ax.set_ylabel('y (m)')
        ax.set_aspect('equal')
        cbar = plt.colorbar(im, ax=ax)
        cbar.set_label('von Mises (GPa)')

        print(f"  {nom:12s}: σmax={np.max(vm)/1e9:.2f} GPa, "
              f"déflection={np.max(problem.v):.3f}m")

    plt.tight_layout()

    return problems


def exemple_cisaillement_pur():
    """Cisaillement pur."""
    print("\nExemple: Cisaillement pur")
    print("-" * 50)

    width, height = 10.0, 10.0
    nx, ny = 31, 31
    E = 200e9
    nu = 0.3

    problem = ElasticityFEM2D(width, height, nx, ny, E, nu)

    # Déplacements opposés sur les faces haut/bas
    i_start = 0
    i_end = nx - 1
    indices = list(range(i_start, i_end+1))

    boundary_conditions = {
        'bottom': [(1, -0.02, indices)],
        'top': [(1, 0.02, indices)],
        'left': [(0, 0.0)],
        'right': None
    }

    problem.solve(boundary_conditions)
    problem.compute_stress()

    print(f"  Dimensions: {width}m × {height}m")
    print(f"  Cisaillement pur")
    print(f"  Contrainte de cisaillement max: {np.max(np.abs(problem.stress_xy)):.2e} Pa")

    gamma = 2*np.max(problem.v) / width
    print(f"  Angle de distorsion γ ≈ {np.degrees(gamma):.4f}°")

    ElasticityVisualizer.plot_displacement_and_stress(problem, "Cisaillement pur")
    ElasticityVisualizer.plot_deformed_geometry(problem, scale=100, title="Cisaillement pur")
    ElasticityVisualizer.plot_stress_component(problem, 'xy', "Cisaillement pur")

    return problem


def exemple_comparaison_nu():
    """Influence du coefficient de Poisson."""
    print("\nExemple: Influence du coefficient de Poisson")
    print("-" * 50)

    width, height = 10.0, 5.0
    nx, ny = 31, 16
    E = 200e9
    disp = 0.02

    nu_values = [0.0, 0.1, 0.2, 0.3, 0.45]
    resultats = []

    fig, axes = plt.subplots(1, len(nu_values), figsize=(20, 4))
    fig.suptitle("Influence du coefficient de Poisson", fontsize=12, fontweight='bold')

    for nu, ax in zip(nu_values, axes):
        problem = ElasticityFEM2D(width, height, nx, ny, E, nu)

        boundary_conditions = {
            'bottom': [(1, 0.0)],
            'top': [(1, disp)],
            'left': [(0, 0.0)],
            'right': None
        }

        problem.solve(boundary_conditions)
        problem.compute_stress()

        vm = problem.von_mises()

        X, Y = np.meshgrid(problem.x, problem.y)
        im = ax.contourf(X, Y, vm/1e9, levels=20, cmap='jet')
        ax.set_title(f'ν = {nu:.2f}')
        ax.set_xlabel('x (m)')
        ax.set_ylabel('y (m)')
        ax.set_aspect('equal')
        plt.colorbar(im, ax=ax, label='von Mises (GPa)')

        resultats.append({
            'nu': nu,
            'stress_xx_max': np.max(np.abs(problem.stress_xx)),
            'stress_yy_max': np.max(np.abs(problem.stress_yy)),
            'vm_max': np.max(vm)
        })

        print(f"  ν={nu:.2f}: σxx_max={np.max(np.abs(problem.stress_xx))/1e9:.3f} GPa, "
              f"σyy_max={np.max(np.abs(problem.stress_yy))/1e9:.3f} GPa")

    plt.tight_layout()

    return resultats


def exemple_analyse_sensibilite():
    """Analyse de sensibilité aux paramètres."""
    print("\nExemple: Analyse de sensibilité")
    print("-" * 50)

    width, height = 10.0, 5.0
    nx, ny = 31, 16
    disp = 0.02

    # Variation du module de Young
    E_values = [100e9, 200e9, 300e9]
    problems = {}

    fig, axes = plt.subplots(1, 3, figsize=(16, 5))
    fig.suptitle("Sensibilité au module de Young (ν=0.3)", fontsize=12, fontweight='bold')

    for E, ax in zip(E_values, axes):
        problem = ElasticityFEM2D(width, height, nx, ny, E, 0.3)

        boundary_conditions = {
            'bottom': [(1, 0.0)],
            'top': [(1, disp)],
            'left': [(0, 0.0)],
            'right': None
        }

        problem.solve(boundary_conditions)
        problem.compute_stress()

        vm = problem.von_mises()
        problems[f'E={E/1e9:.0f}GPa'] = problem

        X, Y = np.meshgrid(problem.x, problem.y)
        im = ax.contourf(X, Y, vm/1e9, levels=20, cmap='jet')
        ax.set_title(f'E = {E/1e9:.0f} GPa')
        ax.set_xlabel('x (m)')
        ax.set_ylabel('y (m)')
        ax.set_aspect('equal')
        cbar = plt.colorbar(im, ax=ax)
        cbar.set_label('von Mises (GPa)')

        print(f"  E={E/1e9:.0f}GPa: σmax={np.max(vm)/1e9:.3f} GPa")

    plt.tight_layout()

    return problems


if __name__ == "__main__":
    print("=" * 60)
    print("EXEMPLES AVANCÉS - ÉLASTICITÉ 2D")
    print("=" * 60)

    # Exécuter les exemples
    prob1 = exemple_poutre_encastree()
    result2 = exemple_etude_convergence()
    probs3 = exemple_comparaison_matieres()
    prob4 = exemple_cisaillement_pur()
    result5 = exemple_comparaison_nu()
    probs6 = exemple_analyse_sensibilite()

    plt.show()

    print("\n" + "=" * 60)
    print("Tous les exemples ont été exécutés avec succès!")
    print("=" * 60)
