# Module elasticity

Solveur d'élasticité 2D par méthode des éléments finis.

## Modules

### `fem_solver.py`
Cœur du solveur FEM:
- Classe `ElasticityFEM2D`
- Assemblage de la matrice de rigidité
- Résolution du système linéaire
- Calcul des contraintes

**Dépendances:** numpy, scipy

```python
from elasticity import ElasticityFEM2D

problem = ElasticityFEM2D(width=10, height=5, nx=31, ny=16, E=200e9, nu=0.3)
```

### `visualization.py`
Affichage des résultats:
- Classe `ElasticityVisualizer`
- 6 méthodes de visualisation
- Comparaison de cas

**Dépendances:** numpy, matplotlib

```python
from elasticity import ElasticityVisualizer

ElasticityVisualizer.plot_von_mises(problem)
```

### `mesh.py`
Gestion du maillage:
- Classe `Mesh2D`
- 6 méthodes de sélection de nœuds
- Visualisation du maillage
- Fonction `explain_mesh()`

**Dépendances:** numpy, matplotlib

```python
from elasticity import Mesh2D

mesh = Mesh2D(width=10, height=5, nx=31, ny=16)
selected = mesh.select_nodes_in_circle(5, 2.5, 1)
```

## Utilisation rapide

```python
from elasticity import ElasticityFEM2D, ElasticityVisualizer, Mesh2D

# Créer le maillage
mesh = Mesh2D(10, 5, 31, 16)

# Sélectionner des nœuds
selected = mesh.select_nodes_on_line('top', 4, 6)

# Créer le problème
problem = ElasticityFEM2D(10, 5, 31, 16, 200e9, 0.3)

# Appliquer les conditions aux limites
bc = {
    'bottom': [(1, 0.0)],
    'left': [(0, 0.0)],
    'nodes': [(n, 1, 0.02) for n in selected]
}

# Résoudre
problem.solve(bc)
problem.compute_stress()

# Afficher
ElasticityVisualizer.plot_von_mises(problem)
```

## Documentation

Pour la documentation complète, voir `/docs/README.md` ou `/docs/QUICKSTART.md`
