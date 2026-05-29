# ✨ Nouvelles fonctionnalités - Maillage et sélection de nœuds

## 📝 Résumé des améliorations

Deux demandes ont été intégrées:

### 1. 📊 Explication du maillage
**Module:** `mesh.py`  
**Documentation:** `MAILLAGE.md`

Le maillage généré par le solveur FEM est maintenant **complètement expliqué**:
- ✅ Structure du maillage cartésien
- ✅ Paramètres (nx, ny, dx, dy)
- ✅ Numérotation des nœuds
- ✅ Calcul des coordonnées
- ✅ Nombre d'éléments et DDLs
- ✅ Rapport d'aspect des éléments

### 2. 🎯 Sélection arbitraire de nœuds
**Module:** `mesh.py` + `fem_solver.py` amélioré  
**Exemples:** `exemple_selection_noeuds.py`

Les conditions aux limites peuvent maintenant être appliquées **sur n'importe quelle sélection de nœuds**, pas seulement les bords complets:
- ✅ Charges concentrées
- ✅ Appuis multiples
- ✅ Régions circulaires
- ✅ Régions rectangulaires
- ✅ Sélections personnalisées (prédicats)

---

## 🆕 Nouveaux fichiers

### `mesh.py` (10 KB)
**Classe:** `Mesh2D`  
**Responsabilité:** Gestion complète du maillage

#### Sélections disponibles:
```python
mesh.select_nodes_at_boundary('bottom')           # Toute une limite
mesh.select_nodes_on_line('top', x_min, x_max)   # Portion d'une limite
mesh.select_nodes_in_region(x_min, x_max, y_min, y_max)    # Rectangle
mesh.select_nodes_in_circle(cx, cy, radius)      # Cercle
mesh.select_nodes_custom(predicate)               # Sélection personnalisée
```

#### Visualisation:
```python
mesh.plot_mesh()                       # Affiche le maillage
mesh.plot_selected_nodes(nodes)        # Affiche les nœuds sélectionnés
mesh.print_info()                      # Infos du maillage
```

#### Utilitaires:
```python
mesh.get_node_position(node_idx)       # Retourne (x, y)
explain_mesh()                         # Explication textuelle
```

---

### `exemple_selection_noeuds.py` (7 KB)
**5 exemples complets** montrant différentes utilisations:

1. **Charge concentrée** - Déplacement imposé sur un petit groupe de nœuds
2. **Appuis multiples** - Plusieurs points d'appui distincts
3. **Région circulaire** - Chargement sur une zone circulaire
4. **Région rectangulaire** - Chargement sur une zone rectangulaire
5. **Sélection personnalisée** - Utilisation d'un prédicat custom

Chaque exemple:
- Crée un maillage
- Sélectionne des nœuds
- Applique des conditions aux limites
- Résout et affiche les résultats

```bash
python3 exemple_selection_noeuds.py
```

---

### `MAILLAGE.md` (10 KB)
**Documentation complète** sur le maillage et les sélections

Contient:
- Structure du maillage cartésien
- Calcul des paramètres (dx, dy)
- Numérotation des nœuds
- 6 méthodes de sélection
- 4 formats de conditions aux limites
- Cas d'usage courants
- Bonnes pratiques

---

## 🔄 Modifications à `fem_solver.py`

### Nouvelles méthodes:
```python
def _get_node_coords(self, node_idx):
    """Convertit index global en (i, j)"""

def get_node_position(self, node_idx):
    """Retourne (x, y) pour un nœud"""
```

### Formats de conditions aux limites étendus:

#### Ancien format (toujours supporté):
```python
bc = {
    'bottom': [(1, 0.0)],
    'top': [(1, 0.02)],
    'left': [(0, 0.0)],
    'right': None
}
```

#### Nouveau format (sélection arbitraire):
```python
bc = {
    'nodes': [
        (node_idx, component, value),
        (node_idx, component, value),
        ...
    ]
}
```

#### Format mixte:
```python
bc = {
    'bottom': [(1, 0.0)],                    # Ancien format
    'nodes': [(5, 0, 0.001), (6, 0, 0.002)] # Nouveau format
}
```

---

## 💡 Exemples d'utilisation

### Exemple 1: Charge concentrée au centre haut

```python
from fem_solver import ElasticityFEM2D
from mesh import Mesh2D

mesh = Mesh2D(10, 5, 31, 16)
problem = ElasticityFEM2D(10, 5, 31, 16, 200e9, 0.3)

# Sélectionner nœuds au centre du haut
top_center = mesh.select_nodes_on_line('top', x_min=4, x_max=6)

# Appliquer les conditions
bc = {
    'bottom': [(1, 0.0)],
    'left': [(0, 0.0)],
    'nodes': [(n, 1, 0.02) for n in top_center]
}

problem.solve(bc)
problem.compute_stress()
```

### Exemple 2: Appuis élastiques multiples

```python
# Trois appuis au bas
support1 = mesh.select_nodes_on_line('bottom', 1, 3)
support2 = mesh.select_nodes_on_line('bottom', 4, 6)
support3 = mesh.select_nodes_on_line('bottom', 7, 9)

bc = {
    'left': [(0, 0.0)],
    'nodes': [
        *[(n, 1, 0.0) for n in support1 + support2 + support3],
        *[(n, 1, 0.01) for n in top_center]
    ]
}
```

### Exemple 3: Sélection personnalisée

```python
# Tous les nœuds avec x > 5 et y < 3
def my_selection(i, j, x, y):
    return x > 5 and y < 3

selected = mesh.select_nodes_custom(my_selection)

bc = {
    'nodes': [(n, 1, 0.01) for n in selected]
}
```

---

## 📚 Flux d'utilisation recommandé

### Pour comprendre le maillage:

```bash
1. Lire MAILLAGE.md
2. Exécuter:
   python3 -c "from mesh import Mesh2D; m = Mesh2D(10,5,21,11); m.print_info()"
3. Visualiser:
   python3 -c "from mesh import Mesh2D; m = Mesh2D(10,5,21,11); m.plot_mesh()"
```

### Pour utiliser les sélections:

```bash
1. Lire la section "Sélection de nœuds" dans MAILLAGE.md
2. Exécuter les exemples:
   python3 exemple_selection_noeuds.py
3. Adapter pour votre cas
```

### Structure typique d'un script:

```python
# 1. Importer
from fem_solver import ElasticityFEM2D
from mesh import Mesh2D
from visualization import ElasticityVisualizer

# 2. Créer le maillage et l'afficher
mesh = Mesh2D(10, 5, 31, 16)
mesh.print_info()
mesh.plot_mesh()

# 3. Sélectionner les nœuds
selected = mesh.select_nodes_on_line('top', 4, 6)
mesh.plot_selected_nodes(selected)

# 4. Créer et résoudre le problème
problem = ElasticityFEM2D(10, 5, 31, 16, 200e9, 0.3)
bc = {
    'bottom': [(1, 0.0)],
    'left': [(0, 0.0)],
    'nodes': [(n, 1, 0.02) for n in selected]
}
problem.solve(bc)
problem.compute_stress()

# 5. Visualiser les résultats
ElasticityVisualizer.plot_von_mises(problem)
```

---

## 🎯 Cas d'usage maintenant possibles

### Avant:
- ❌ Conditions aux limites **seulement sur les bords complets**
- ❌ Pas d'explication du maillage

### Maintenant:
- ✅ Charges concentrées
- ✅ Appuis multiples
- ✅ Régions circulaires ou rectangulaires
- ✅ Sélections personnalisées
- ✅ Documentation complète du maillage
- ✅ Visualisation du maillage et des sélections

---

## 📋 Checklist d'utilisation

- [ ] Lire QUICKSTART.md
- [ ] Exécuter exemple_simple.py
- [ ] Lire MAILLAGE.md
- [ ] Exécuter exemple_selection_noeuds.py
- [ ] Créer votre propre cas

---

## 🚀 Prochaines étapes

1. Consulter `MAILLAGE.md` pour les détails
2. Exécuter `exemple_selection_noeuds.py` pour voir les exemples
3. Adapter le code pour votre application

---

**Dernière mise à jour:** 2026-05-29  
**Statut:** ✅ Production-ready
