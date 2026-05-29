# Guide complet: Maillage et Sélection de nœuds

## 📊 Structure du maillage

Le projet utilise un **maillage cartésien structuré 2D**, c'est-à-dire:
- Un domaine rectangulaire régulier
- Nœuds arrangés en grille
- Espacement uniforme en x et en y

### Création du maillage

```python
from fem_solver import ElasticityFEM2D

# Le maillage est créé automatiquement
problem = ElasticityFEM2D(
    width=10.0,    # Largeur du domaine (m)
    height=5.0,    # Hauteur du domaine (m)
    nx=31,         # Nombre de nœuds en x
    ny=16          # Nombre de nœuds en y
)
```

### Paramètres du maillage

```
Domaine: [0, width] × [0, height]
Nœuds en x: 0, dx, 2*dx, ..., width    (nx nœuds)
Nœuds en y: 0, dy, 2*dy, ..., height   (ny nœuds)

Espacement:
  dx = width / (nx - 1)     [distance entre nœuds en x]
  dy = height / (ny - 1)    [distance entre nœuds en y]

Éléments:
  Nombre d'éléments en x: nx - 1
  Nombre d'éléments en y: ny - 1
  Total d'éléments: (nx - 1) × (ny - 1)
  
Degrés de liberté (DDLs):
  Chaque nœud a 2 DDLs (u et v)
  Total DDLs: 2 × nx × ny
```

### Exemple numérique

```
Paramètres: width=10m, height=5m, nx=11, ny=6

Espacement:
  dx = 10 / (11-1) = 1.0 m
  dy = 5 / (6-1) = 1.0 m

Nœuds en x: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
Nœuds en y: 0, 1, 2, 3, 4, 5

Éléments: 10 × 5 = 50 éléments (carrés 1×1)
Nœuds: 11 × 6 = 66 nœuds
DDLs: 2 × 66 = 132
```

## 🔢 Numérotation des nœuds

Les nœuds sont numérotés de **bas en haut, de gauche à droite**:

```
Index global = j * nx + i

Où:
  i = position en x (0 à nx-1)
  j = position en y (0 à ny-1)
```

### Exemple visuel (nx=5, ny=3)

```
        j=2  10  11  12  13  14    (y = height)
        j=1   5   6   7   8   9
        j=0   0   1   2   3   4    (y = 0)
             i=0 i=1 i=2 i=3 i=4
                (x = 0)  (x = width)
```

### Accès aux coordonnées

```python
# Obtenir l'index du nœud à la position (i, j)
node_index = j * nx + i

# Obtenir (x, y) à partir de l'index
x = i * dx
y = j * dy

# Dans le code FEM
x[i]  # Coordonnée x du i-ème nœud
y[j]  # Coordonnée y du j-ème nœud
```

## 📍 Sélection de nœuds

Le module `mesh.py` fournit plusieurs méthodes pour sélectionner des nœuds:

### 1. Sélectionner toute une limite

```python
from mesh import Mesh2D

mesh = Mesh2D(width=10, height=5, nx=31, ny=16)

# Toute la limite
nodes_bottom = mesh.select_nodes_at_boundary('bottom')  # 31 nœuds
nodes_top = mesh.select_nodes_at_boundary('top')        # 31 nœuds
nodes_left = mesh.select_nodes_at_boundary('left')      # 16 nœuds
nodes_right = mesh.select_nodes_at_boundary('right')    # 16 nœuds
```

### 2. Sélectionner une portion d'une limite

```python
# Portion du bas entre x=2 et x=8
nodes_bottom_center = mesh.select_nodes_on_line('bottom', x_min=2, x_max=8)

# Portion du haut entre x=3 et x=7
nodes_top_center = mesh.select_nodes_on_line('top', x_min=3, x_max=7)

# Portion de la gauche entre y=1 et y=3
nodes_left_middle = mesh.select_nodes_on_line('left', x_min=1, x_max=3)  # x_min→y_min
```

### 3. Sélectionner une région rectangulaire

```python
# Tous les nœuds dans [2,8] × [1,4]
nodes_rect = mesh.select_nodes_in_region(
    x_min=2.0, x_max=8.0,
    y_min=1.0, y_max=4.0
)
```

### 4. Sélectionner une région circulaire

```python
# Tous les nœuds dans un cercle
nodes_circle = mesh.select_nodes_in_circle(
    center_x=5.0,    # Centre du cercle
    center_y=2.5,
    radius=1.5       # Rayon
)
```

### 5. Sélection personnalisée

```python
# Prédicat personnalisé: fonction(i, j, x, y) -> bool
def my_selection(i, j, x, y):
    # Exemple: triangle supérieur droit
    return x + y > width * 0.7

nodes_custom = mesh.select_nodes_custom(my_selection)

# Autre exemple: rectangle biseauté
def bevel_selection(i, j, x, y):
    dx, dy = 10/30, 5/15
    # Points proches du centre
    dist_to_center = np.sqrt((x - 5)**2 + (y - 2.5)**2)
    return dist_to_center < 2.0

nodes_bevel = mesh.select_nodes_custom(bevel_selection)
```

### 6. Sélections combinées

```python
# Combiner plusieurs sélections
top_center = mesh.select_nodes_on_line('top', 3, 7)
left_middle = mesh.select_nodes_on_line('left', 1, 3)
circle = mesh.select_nodes_in_circle(5, 2.5, 1)

combined = top_center + left_middle + circle  # Union
combined = list(set(combined))  # Supprimer les doublons
```

## 🎯 Imposer des conditions aux limites

### Format 1: Limites complètes (ancien style)

```python
bc = {
    'bottom': [(1, 0.0)],           # v=0 sur tout le bas
    'top': [(1, 0.02)],             # v=0.02 sur tout le haut
    'left': [(0, 0.0)],             # u=0 sur toute la gauche
    'right': None
}
problem.solve(bc)
```

### Format 2: Portions de limites

```python
bc = {
    'bottom': [(1, 0.0, list(range(5, 26)))],   # v=0 sur portion du bas
    'top': [(1, 0.02)],                         # v=0.02 sur tout le haut
    'left': [(0, 0.0)],
    'right': None
}
problem.solve(bc)
```

### Format 3: Sélection arbitraire de nœuds (NOUVEAU)

```python
from mesh import Mesh2D

mesh = Mesh2D(10, 5, 31, 16)

# Sélectionner des nœuds
center_nodes = mesh.select_nodes_on_line('top', 4, 6)
left_nodes = mesh.select_nodes_at_boundary('left')

# Appliquer les conditions
bc = {
    'nodes': [
        (node, 0, 0.0) for node in left_nodes      # u=0 à gauche
        + [(node, 1, 0.01) for node in center_nodes]  # v=0.01 au centre top
    ]
}
problem.solve(bc)
```

### Format 4: Mélange de formats

```python
mesh = Mesh2D(10, 5, 31, 16)
support_nodes = mesh.select_nodes_on_line('bottom', 3, 7)
load_nodes = mesh.select_nodes_in_circle(5, 4.5, 0.5)

bc = {
    'bottom': [(1, 0.0)],              # Appui complet en bas
    'left': [(0, 0.0)],                # Bloqué en u à gauche
    'nodes': [
        *[(n, 1, 0.0) for n in support_nodes],     # Appuis supplémentaires
        *[(n, 1, 0.02) for n in load_nodes]        # Charge concentrée
    ]
}
problem.solve(bc)
```

## 📈 Rapport d'aspect des éléments

Le rapport d'aspect est important pour la qualité du calcul:

```python
aspect_ratio = dy / dx

# Idéal: ratio ≈ 1 (éléments carrés)
# Acceptable: 0.5 < ratio < 2
# Problématique: ratio > 10 ou < 0.1
```

### Exemple

```python
width, height = 10, 5
nx, ny = 31, 16
dx = width / (nx - 1) = 10/30 = 0.333
dy = height / (ny - 1) = 5/15 = 0.333
aspect_ratio = 0.333 / 0.333 = 1.0  ✓ Parfait!
```

Pour maintenir un ratio proche de 1:

```python
# Si vous changez les dimensions
width, height = 10, 2    # Ratio H/W = 0.2
nx = 51                   # 50 éléments en x
ny = int(50 * height / width) + 1  # = 11 → 10 éléments en y
# dy/dx ≈ 1 ✓
```

## 🔍 Visualisation du maillage

```python
from mesh import Mesh2D

mesh = Mesh2D(width=10, height=5, nx=31, ny=16)

# Afficher le maillage
fig = mesh.plot_mesh("Mon maillage")

# Afficher les nœuds sélectionnés
selected = mesh.select_nodes_on_line('top', 3, 7)
fig = mesh.plot_selected_nodes(selected, "Sélection")

# Infos du maillage
mesh.print_info()
```

## 📋 Exemple complet

```python
from fem_solver import ElasticityFEM2D
from mesh import Mesh2D
from visualization import ElasticityVisualizer

# 1. Créer le maillage
mesh = Mesh2D(width=10, height=5, nx=31, ny=16)
mesh.print_info()

# 2. Visualiser
mesh.plot_mesh("Maillage initial")

# 3. Sélectionner des nœuds
support = mesh.select_nodes_on_line('bottom', 2, 8)
load = mesh.select_nodes_in_circle(5, 4.5, 1)

mesh.plot_selected_nodes(support + load, "Appuis et charge")

# 4. Créer le problème
problem = ElasticityFEM2D(10, 5, 31, 16, 200e9, 0.3)

# 5. Appliquer les conditions aux limites
bc = {
    'left': [(0, 0.0)],
    'nodes': [
        (n, 1, 0.0) for n in support +
        [(n, 1, 0.02) for n in load]
    ]
}

# 6. Résoudre
problem.solve(bc)
problem.compute_stress()

# 7. Visualiser les résultats
ElasticityVisualizer.plot_von_mises(problem, "Résultats")
```

## 🎓 Cas d'usage courants

### Charge ponctuelle

```python
# Un seul nœud chargé
single_node = 150  # Index global
bc = {'nodes': [(single_node, 1, 0.01)]}
```

### Charge distribuée sur une zone

```python
# Charge sur une région
zone = mesh.select_nodes_in_region(3, 7, 4.5, 5)
bc = {'nodes': [(n, 1, 0.01) for n in zone]}
```

### Appui élastique (partiellement élastique)

```python
# Appui avec ressort (simulé par déplacement partiel)
support = mesh.select_nodes_on_line('bottom', 2, 8)
bc = {'nodes': [(n, 1, 0.001) for n in support]}  # Déplacement petit = ressort
```

### Encastrement partiel

```python
# Encastrement seulement au centre
center = mesh.select_nodes_on_line('left', 1.5, 3.5)
bc = {
    'nodes': [
        (n, 0, 0.0) for n in center +
        [(n, 1, 0.0) for n in center]
    ]
}
```

## 🚀 Bonnes pratiques

1. **Ratio d'aspect**: Garder dy/dx proche de 1
2. **Résolution**: Augmenter progressivement nx et ny pour tests de convergence
3. **Visualisation**: Toujours visualiser le maillage et les sélections avant résolution
4. **DDLs**: S'assurer que tous les mouvements rigides sont bloqués
5. **Précision**: Affiner le maillage dans les zones d'intérêt

## 📖 Pour en savoir plus

- Exécuter `python3 exemple_selection_noeuds.py` pour voir 5 exemples
- Consulter `mesh.py` pour la documentation des méthodes
- Lancer `python3 -c "from mesh import explain_mesh; print(explain_mesh())"`
