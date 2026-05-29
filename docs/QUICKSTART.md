# Démarrage rapide

## Installation

```bash
pip install numpy scipy matplotlib
```

## Fichiers à utiliser

### Option 1: Exemples clé en main
```bash
python3 exemple_simple.py      # 3 cas basiques
python3 exemple_avance.py      # 6 cas avancés
```

### Option 2: Créer votre propre cas

```python
from fem_solver import ElasticityFEM2D
from visualization import ElasticityVisualizer

# Créer le problème
problem = ElasticityFEM2D(
    width=10.0,    # Largeur (m)
    height=5.0,    # Hauteur (m)
    nx=31,         # Nœuds en x
    ny=16,         # Nœuds en y
    E=200e9,       # Module Young (Pa) - Acier
    nu=0.3         # Coefficient Poisson
)

# Conditions aux limites
bc = {
    'bottom': [(1, 0.0)],          # v=0
    'top': [(1, 0.02)],            # v=0.02
    'left': [(0, 0.0)],            # u=0
    'right': None                  # libre
}

# Résoudre
problem.solve(bc)
problem.compute_stress()

# Afficher
ElasticityVisualizer.plot_displacement_and_stress(problem, "Mon cas")
ElasticityVisualizer.plot_von_mises(problem)
```

## Cas courants

### Traction
```python
bc = {
    'bottom': [(1, 0.0)],      # Bloqué
    'top': [(1, 0.01)],        # Tiré vers le haut
    'left': [(0, 0.0)],        # Bloqué horizontalement
    'right': None
}
```

### Compression
```python
bc = {
    'bottom': [(1, 0.0)],      
    'top': [(1, -0.01)],       # Tiré vers le bas
    'left': [(0, 0.0)],        
    'right': None
}
```

### Cisaillement
```python
indices = list(range(5, 26))   # Zone centrale
bc = {
    'bottom': [(1, -0.01, indices)],   # Vers le bas
    'top': [(1, 0.01, indices)],       # Vers le haut
    'left': [(0, 0.0)],                
    'right': None
}
```

### Flexion (poutre cantilever)
```python
bc = {
    'bottom': [(1, 0.0)],
    'top': [(1, 0.01)],
    'left': [(0, 0.0), (1, 0.0)],    # Encastrement
    'right': None
}
```

## Matériaux courants

```python
# Acier
E = 200e9; nu = 0.3

# Aluminium
E = 70e9; nu = 0.33

# Titane
E = 100e9; nu = 0.34

# Béton
E = 30e9; nu = 0.2

# Caoutchouc
E = 0.01e9; nu = 0.49
```

## Accéder aux résultats

```python
# Déplacements
problem.u                    # Horizontal (array ny×nx)
problem.v                    # Vertical (array ny×nx)

# Contraintes
problem.stress_xx            # σxx
problem.stress_yy            # σyy  
problem.stress_xy            # σxy

# Contrainte équivalente
vm = problem.von_mises()

# Résultats structurés
results = problem.get_results()
# contient: x, y, u, v, stress_xx, stress_yy, stress_xy, E, nu, etc.

# Statistiques
print(f"Max déplacement: {np.max(problem.v):.4e} m")
print(f"Max contrainte: {np.max(vm):.4e} Pa")
print(f"Min contrainte: {np.min(vm):.4e} Pa")
```

## Visualisations

```python
from visualization import ElasticityVisualizer

# Vue complète
ElasticityVisualizer.plot_displacement_and_stress(problem, "Titre")

# Géométrie déformée
ElasticityVisualizer.plot_deformed_geometry(problem, scale=100, title="Titre")

# Contrainte von Mises
ElasticityVisualizer.plot_von_mises(problem, "Titre")

# Composante spécifique
ElasticityVisualizer.plot_stress_component(problem, 'xx')   # σxx
ElasticityVisualizer.plot_stress_component(problem, 'yy')   # σyy
ElasticityVisualizer.plot_stress_component(problem, 'xy')   # σxy

# Magnitude déplacement
ElasticityVisualizer.plot_displacement_magnitude(problem)

# Comparer plusieurs cas
cases = {'Cas A': prob1, 'Cas B': prob2}
ElasticityVisualizer.compare_results(cases, metric='von_mises')
```

## Améliorations courantes

### Augmenter la précision
```python
# Plus de nœuds = plus précis
problem = ElasticityFEM2D(10, 5, 61, 31, E, nu)  # Grille 60×30 au lieu de 30×15
```

### Afficher la déformation amplifiée
```python
ElasticityVisualizer.plot_deformed_geometry(problem, scale=500)  # Amplification 500×
```

### Extraire une ligne de contrainte
```python
# Contrainte au centre
j = problem.ny // 2
sigma_line = problem.stress_yy[j, :]

import matplotlib.pyplot as plt
plt.plot(problem.x, sigma_line)
plt.xlabel('x (m)')
plt.ylabel('σyy (Pa)')
plt.show()
```

## Dépannage

**Erreur: "Matrix is singular"**
→ Vérifier que toutes les translations/rotations rigides sont bloquées

**Résultats NaN**
→ Vérifier les conditions aux limites (au moins 3 DDLs fixés)

**Calculs lents**
→ Réduire nx, ny pour tester, puis augmenter pour la solution finale

## Structure des fichiers

```
.
├── fem_solver.py           ← CALCUL SEUL (réutilisable)
├── visualization.py        ← AFFICHAGE SEUL (indépendant)
├── exemple_simple.py       ← Exemples simples (à exécuter)
├── exemple_avance.py       ← Exemples avancés (à exécuter)
├── test_integration.py     ← Vérification
├── README.md               ← Documentation complète
├── STRUCTURE.md            ← Architecture détaillée
└── QUICKSTART.md           ← Ce fichier
```

## Exemple complet: Poutre cantilever

```python
from fem_solver import ElasticityFEM2D
from visualization import ElasticityVisualizer
import numpy as np

# Définir le problème
L = 4.0           # Longueur
h = 0.4           # Hauteur
nx, ny = 51, 11   # Discrétisation

problem = ElasticityFEM2D(
    width=L,
    height=h,
    nx=nx,
    ny=ny,
    E=210e9,      # Acier
    nu=0.3
)

# Encastrement à gauche, charge en flexion à droite
bc = {
    'bottom': [(1, 0.0)],
    'top': [(1, 0.015)],
    'left': [(0, 0.0), (1, 0.0)],    # Encastrement complet
    'right': None
}

# Résoudre
print("Résolution...")
problem.solve(bc)
problem.compute_stress()

# Résultats
vm_max = np.max(problem.von_mises())
deflection = np.max(problem.v)
ratio = deflection / L

print(f"Flèche: {deflection:.4f} m ({ratio*100:.2f}% de L)")
print(f"Contrainte max: {vm_max/1e9:.2f} GPa")

# Visualisation
ElasticityVisualizer.plot_displacement_and_stress(problem, "Poutre cantilever")
ElasticityVisualizer.plot_deformed_geometry(problem, scale=50, title="Poutre cantilever")
```

## Prochaines étapes

1. **Exécuter les exemples** → `python3 exemple_simple.py`
2. **Adapter un exemple** → Copier et modifier
3. **Lire la documentation** → `README.md`, `STRUCTURE.md`
4. **Consulter le code** → Les trois modules sont auto-documentés

Bonne utilisation! 🚀
