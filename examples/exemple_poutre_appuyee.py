#!/usr/bin/env python3
"""
Exemple: Poutre appuyée sur deux tiers extérieurs avec charge répartie.

Géométrie:
- Poutre horizontale de longueur L et hauteur h
- Appuis (v=0) sur les tiers extérieurs: [0, L/3] et [2L/3, L]
- Charge répartie (v négatif) entre les appuis: [L/3, 2L/3]

C'est un cas classique de RDM (Résistance des Matériaux).
"""

import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import numpy as np
import matplotlib.pyplot as plt
from elasticity import ElasticityFEM2D, ElasticityVisualizer, Mesh2D


def exemple_poutre_appuyee():
    """Poutre appuyée sur deux tiers extérieurs avec charge répartie."""
    print("\n" + "=" * 70)
    print("EXEMPLE: POUTRE APPUYÉE SUR DEUX TIERS EXTÉRIEURS")
    print("=" * 70)

    # Paramètres géométriques
    L = 12.0          # Longueur de la poutre (m)
    h = 1.0           # Hauteur de la poutre (m)
    nx, ny = 61, 11   # Discrétisation

    # Matériau: acier
    E = 210e9         # Module de Young (Pa)
    nu = 0.3          # Coefficient de Poisson

    print(f"\nGéométrie:")
    print(f"  Longueur: {L} m")
    print(f"  Hauteur: {h} m")
    print(f"  Discrétisation: {nx}×{ny} nœuds")
    print(f"  E = {E:.2e} Pa, ν = {nu}")

    # Créer le maillage
    mesh = Mesh2D(width=L, height=h, nx=nx, ny=ny)
    mesh.print_info()

    # Positions des appuis et de la charge
    L_third = L / 3
    two_L_third = 2 * L / 3

    print(f"\nConfiguration mécanique:")
    print(f"  Appui gauche: x ∈ [0, {L_third:.1f}] m")
    print(f"  Zone de charge: x ∈ [{L_third:.1f}, {two_L_third:.1f}] m")
    print(f"  Appui droit: x ∈ [{two_L_third:.1f}, {L:.1f}] m")

    # Sélectionner les appuis (portions externes du bas)
    appui_gauche = mesh.select_nodes_on_line('bottom', x_min=0, x_max=L_third)
    appui_droit = mesh.select_nodes_on_line('bottom', x_min=two_L_third, x_max=L)
    appuis = appui_gauche + appui_droit

    # Sélectionner la zone de charge (portion centrale du haut)
    zone_charge = mesh.select_nodes_on_line('top', x_min=L_third, x_max=two_L_third)

    print(f"\nSélection des nœuds:")
    print(f"  Appui gauche: {len(appui_gauche)} nœuds")
    print(f"  Appui droit: {len(appui_droit)} nœuds")
    print(f"  Zone de charge: {len(zone_charge)} nœuds")

    # Visualiser la géométrie avec les appuis et charge
    fig = plt.figure(figsize=(14, 6))

    ax1 = fig.add_subplot(121)
    mesh.plot_selected_nodes(appuis + zone_charge, "Appuis et charge")

    # Colorer les appuis en bleu et la charge en rouge
    ax2 = fig.add_subplot(122)
    X, Y = np.meshgrid(mesh.x, mesh.y)
    ax2.plot(X, Y, 'k.', markersize=2, alpha=0.2)

    # Appuis en bleu
    appuis_coords = [mesh.get_node_position(idx) for idx in appuis]
    appuis_x = [c[0] for c in appuis_coords]
    appuis_y = [c[1] for c in appuis_coords]
    ax2.plot(appuis_x, appuis_y, 'bo', markersize=6, label='Appuis (v=0)')

    # Charge en rouge
    charge_coords = [mesh.get_node_position(idx) for idx in zone_charge]
    charge_x = [c[0] for c in charge_coords]
    charge_y = [c[1] for c in charge_coords]
    ax2.plot(charge_x, charge_y, 'r^', markersize=6, label='Charge (v<0)')

    ax2.set_xlim(-0.5, L+0.5)
    ax2.set_ylim(-0.2, h+0.2)
    ax2.set_xlabel('x (m)')
    ax2.set_ylabel('y (m)')
    ax2.set_title('Configuration mécanique')
    ax2.legend()
    ax2.set_aspect('equal')
    ax2.grid(True, alpha=0.3)

    plt.tight_layout()

    # Créer le problème FEM
    problem = ElasticityFEM2D(L, h, nx, ny, E, nu)

    # Conditions aux limites
    # - Appuis: v=0
    # - Charge: v=-0.01 (vers le bas) sur la zone centrale
    # - Gauche: u=0 (empêcher les translations horizontales)
    bc = {
        'left': [(0, 0.0)],           # u=0 à gauche
        'nodes': [
            *[(n, 1, 0.0) for n in appuis],        # Appuis: v=0
            *[(n, 1, -0.01) for n in zone_charge]  # Charge: v=-0.01 (vers le bas)
        ]
    }

    print(f"\nConditions aux limites:")
    print(f"  Appuis: v = 0 m")
    print(f"  Charge répartie: v = -0.01 m (vers le bas)")
    print(f"  Gauche: u = 0 m")

    # Résoudre
    print(f"\nRésolution FEM...")
    problem.solve(bc)
    problem.compute_stress()

    # Résultats
    print(f"\n✓ Problème résolu avec succès")
    print(f"\nRésultats:")
    print(f"  Flèche max (au centre): {np.min(problem.v):.6e} m")
    print(f"  Flèche relative (δ/L): {np.min(problem.v)/L:.6f}")
    print(f"  Contrainte σyy max: {np.max(np.abs(problem.stress_yy)):.3e} Pa")
    print(f"  Contrainte σyy min: {np.min(problem.stress_yy):.3e} Pa")
    print(f"  von Mises max: {np.max(problem.von_mises()):.3e} Pa")

    # Analyse des sections critiques
    # Chercher la position du maximum de flèche (au centre normalement)
    center_idx = nx // 2
    v_center_line = problem.v[:, center_idx]
    max_deflection_idx = np.argmin(v_center_line)
    max_deflection = v_center_line[max_deflection_idx]

    print(f"\nAnalyse aux sections critiques:")
    print(f"  Position max flèche: x = {mesh.x[center_idx]:.2f} m")
    print(f"  Flèche au centre: {max_deflection:.6e} m")

    # Visualiser les résultats
    ElasticityVisualizer.plot_displacement_and_stress(problem, "Poutre appuyée - Charge répartie")
    ElasticityVisualizer.plot_von_mises(problem, "Poutre appuyée - Charge répartie")
    ElasticityVisualizer.plot_deformed_geometry(problem, scale=100, title="Poutre appuyée - Géométrie déformée")

    # Profil de flèche le long de la poutre
    fig, ax = plt.subplots(figsize=(12, 5))

    # Flèche au centre (y = h/2)
    j_center = ny // 2
    v_profile = problem.v[j_center, :]

    ax.plot(mesh.x, v_profile*1000, 'b-', linewidth=2, label='Flèche verticale (×1000)')
    ax.axhline(y=0, color='k', linestyle='--', alpha=0.3)
    ax.axvline(x=L_third, color='g', linestyle='--', alpha=0.5, label='Appuis')
    ax.axvline(x=two_L_third, color='g', linestyle='--', alpha=0.5)
    ax.fill_between([L_third, two_L_third], ax.get_ylim()[0], ax.get_ylim()[1],
                     alpha=0.1, color='red', label='Zone de charge')

    ax.set_xlabel('Position x (m)')
    ax.set_ylabel('Flèche v (mm)')
    ax.set_title('Profil de flèche le long de la poutre')
    ax.grid(True, alpha=0.3)
    ax.legend()

    plt.tight_layout()

    # Comparaison théorique vs FEM (optionnel)
    print(f"\nNotes théoriques (RDM):")
    print(f"  Pour une poutre simplement appuyée (charge concentrée au centre):")
    print(f"  f_max = P·L³/(48·E·I)")
    print(f"  ")
    print(f"  Pour une charge répartie uniforme:")
    print(f"  f_max = 5·q·L⁴/(384·E·I)")
    print(f"  ")
    print(f"  Ici: charge répartie sur portion centrale avec appuis aux tiers")
    print(f"  Configuration non-standard → simulation FEM nécessaire")

    return problem, mesh, appuis, zone_charge


if __name__ == "__main__":
    print("\n" + "=" * 70)
    print("EXEMPLE: POUTRE APPUYÉE SUR DEUX TIERS EXTÉRIEURS")
    print("=" * 70)

    problem, mesh, appuis, zone_charge = exemple_poutre_appuyee()

    plt.show()

    print("\n" + "=" * 70)
    print("Exemple terminé avec succès!")
    print("=" * 70)
