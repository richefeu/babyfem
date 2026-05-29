
import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import numpy as np
import matplotlib.pyplot as plt
from elasticity import ElasticityFEM2D, ElasticityVisualizer, Mesh2D

# maillage
L = 0.1          # Longueur  (m)
h = 0.02         # Hauteur  (m)
nx, ny = 60*2, 6*2   # Discrétisation

mesh = Mesh2D(width=L, height=h, nx=nx, ny=ny)
mesh.print_info()

# conditions aux limites
Lappui = L / 3
delta = -0.002


appui_gauche = mesh.select_nodes_on_line('bottom', x_min=0, x_max=Lappui)
appui_droit = mesh.select_nodes_on_line('bottom', x_min=L-Lappui, x_max=L)
appuis = appui_gauche + appui_droit

zone_charge = mesh.select_nodes_on_line('top', x_min=Lappui+delta, x_max=L-Lappui-delta)

# Visualiser la géométrie avec les appuis et charge
mesh.plot_selected_nodes(appuis + zone_charge, "Appuis et charge")


E = 210e9
nu = 0.3
problem = ElasticityFEM2D(L, h, nx, ny, E, nu)

# Conditions aux limites
# - Appuis: v=0
# - Charge: v=-depl (vers le bas) sur la zone centrale
# - Gauche: u=0 (empêcher les translations horizontales)
depl = 0.0001
bc = {
    'left': [(0, 0.0)],           # u=0 à gauche
    'nodes': [
        *[(n, 1, 0.0) for n in appuis],        # Appuis: v=0
        *[(n, 1, -depl) for n in zone_charge]  # Charge: v=-0.01 (vers le bas)
    ]
}
problem.solve(bc)
problem.compute_stress()

# Diagnostic: Distribution des contraintes von Mises par ligne (y)
print("\nContrainte von Mises par ligne (y):")
vm = np.sqrt(problem.stress_xx**2 + problem.stress_yy**2 - problem.stress_xx*problem.stress_yy + 3.0*problem.stress_xy**2)
for j in range(problem.ny-1, -1, -(problem.ny//6 + 1)):  # Afficher ~6 lignes
    y_pos = j * (h / (problem.ny - 1))
    max_on_line = np.max(vm[j, :])
    print(f"  y={y_pos:.4f} m: max σ = {max_on_line:.4e} GPa")

#ElasticityVisualizer.plot_displacement_and_stress(problem, "Poutre appuyée - Charge répartie")
#ElasticityVisualizer.plot_von_mises(problem, "Poutre appuyée - Charge répartie")
#ElasticityVisualizer.plot_deformed_geometry(problem, scale=100, title="Poutre appuyée - Géométrie déformée")

#plt.show()