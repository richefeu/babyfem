# Structure Modulaire du Projet

## Organisation en trois niveaux

Le projet est organisé en **trois niveaux distincts** pour une meilleure séparation des responsabilités:

```
┌─────────────────────────────────────────────────────┐
│  PROBLÈMES (Exemples & Applications)               │
│  ├─ exemple_simple.py (3 cas simples)              │
│  └─ exemple_avance.py (6 cas avancés)              │
└────────────────┬────────────────────────────────────┘
                 │ utilise
                 ↓
┌─────────────────────────────────────────────────────┐
│  VISUALISATION (Affichage des résultats)           │
│  └─ visualization.py (classe ElasticityVisualizer)  │
└────────────────┬────────────────────────────────────┘
                 │ utilise
                 ↓
┌─────────────────────────────────────────────────────┐
│  CALCUL (Cœur du solveur FEM)                       │
│  └─ fem_solver.py (classe ElasticityFEM2D)          │
└─────────────────────────────────────────────────────┘
```

## Fichiers principaux

### 1. `fem_solver.py` - Module de calcul
**Dépendances:** numpy, scipy  
**Exporté:** `ElasticityFEM2D`

```python
from fem_solver import ElasticityFEM2D

problem = ElasticityFEM2D(width, height, nx, ny, E, nu)
problem.solve(boundary_conditions)
problem.compute_stress()
vm = problem.von_mises()
```

**Responsabilités:**
- ✓ Assemblage de la matrice de rigidité
- ✓ Résolution du système linéaire
- ✓ Calcul des déformations et contraintes
- ✓ Utilitaires numériques
- ✗ Pas de visualisation
- ✗ Pas de matplotlib

**Points d'entrée:**
- `ElasticityFEM2D()`: Initialisation
- `.solve()`: Résoudre avec conditions aux limites
- `.compute_stress()`: Calculer contraintes
- `.von_mises()`: Contrainte équivalente
- `.get_results()`: Retourner tous les résultats

### 2. `visualization.py` - Module de visualisation
**Dépendances:** numpy, matplotlib  
**Exporté:** `ElasticityVisualizer` (classe statique)

```python
from visualization import ElasticityVisualizer

ElasticityVisualizer.plot_displacement_and_stress(problem, "Titre")
ElasticityVisualizer.plot_deformed_geometry(problem, scale=100)
```

**Responsabilités:**
- ✓ Visualiser déplacements
- ✓ Visualiser contraintes (xx, yy, xy, von Mises)
- ✓ Afficher géométrie déformée
- ✓ Comparer plusieurs problèmes
- ✗ Pas de calcul
- ✗ Pas de solveur

**Méthodes disponibles:**
- `plot_displacement_and_stress()`: Vue 2×2
- `plot_deformed_geometry()`: Géométrie + von Mises
- `plot_von_mises()`: Contrainte équivalente
- `plot_stress_component()`: σxx, σyy, ou σxy
- `plot_displacement_magnitude()`: |u|
- `compare_results()`: Comparaison multi-cas

### 3. `exemple_simple.py` - Exemples simples
**Dépendances:** fem_solver, visualization  
**Type:** Exécutable

```bash
python3 exemple_simple.py
```

**Contenu:**
1. **Traction uniaxiale** - Cas basique
   - Déplacement v imposé sur face supérieure
   - Démonstration: contrainte σyy uniforme

2. **Cisaillement partiel** - Déplacement sur portion
   - Déplacements opposés sur zone centrale
   - Démonstration: cisaillement non-uniforme

3. **Influence de ν** - Paramétrique
   - Variation du coefficient de Poisson
   - Comparaison de 3 valeurs

### 4. `exemple_avance.py` - Exemples avancés
**Dépendances:** fem_solver, visualization  
**Type:** Exécutable

```bash
python3 exemple_avance.py
```

**Contenu:**
1. **Poutre encastrée** - Flexion
2. **Convergence** - Étude du maillage
3. **Matériaux** - Acier vs Alu vs Titane
4. **Cisaillement pur** - Déformation angulaire
5. **Coefficient Poisson** - 5 valeurs
6. **Sensibilité E** - Variation du module

## Flux d'utilisation

### Cas 1: Utilisation simple
```python
from fem_solver import ElasticityFEM2D
from visualization import ElasticityVisualizer

# Créer et résoudre
p = ElasticityFEM2D(10, 5, 31, 16, 200e9, 0.3)
p.solve({'bottom': [(1,0)], 'top': [(1,0.02)], 'left': [(0,0)], 'right': None})
p.compute_stress()

# Visualiser
ElasticityVisualizer.plot_von_mises(p)
```

### Cas 2: Utiliser un exemple
```bash
python3 exemple_simple.py
```

### Cas 3: Étendre les exemples
```python
# Copier exemple_simple.py
# Modifier example_1_traction() pour votre cas
# Exécuter

# Ou directement:
from fem_solver import ElasticityFEM2D
from visualization import ElasticityVisualizer

def mon_cas():
    p = ElasticityFEM2D(...)
    p.solve(...)
    p.compute_stress()
    ElasticityVisualizer.plot_von_mises(p)
```

## Avantages de cette architecture

### Séparation des responsabilités
| Aspect | Fichier |
|--------|---------|
| Calcul numérique | fem_solver.py |
| Affichage | visualization.py |
| Application | exemple_*.py |

### Réutilisabilité
- ✓ Utiliser `fem_solver` dans d'autres projets (Jupyter, API web, etc.)
- ✓ Swapper la visualisation (matplotlib → plotly, paraview, etc.)
- ✓ Créer vos propres cas d'usage

### Testabilité
- ✓ Tester le solveur indépendamment
- ✓ Tester la visualisation indépendamment
- ✓ Ajouter des tests unitaires

### Maintenabilité
- ✓ Bugfixes isolés
- ✓ Évolutions indépendantes
- ✓ Documentation claire par module

## Flux de données

```python
# 1. Initialiser le problème
problem = ElasticityFEM2D(L, h, nx, ny, E, nu)
#        ↓
#        fem_solver.ElasticityFEM2D

# 2. Définir conditions aux limites (dict)
bc = {'bottom': [...], 'top': [...], ...}

# 3. Résoudre
problem.solve(bc)
#        ↓
#        Assemble K, applique BC, résout Ku=F
#        Stocke problem.u, problem.v

# 4. Calculer contraintes
problem.compute_stress()
#        ↓
#        Calcule dérivées, σ = Dε
#        Stocke problem.stress_xx/yy/xy

# 5. Visualiser
ElasticityVisualizer.plot_von_mises(problem)
#        ↓
#        visualization.ElasticityVisualizer
#        ↓
#        Lecture problem.stress_xx, problem.stress_yy, problem.stress_xy
#        Affichage matplotlib

# 6. Accéder aux résultats
results = problem.get_results()
#        ↓ retourne dict avec x, y, u, v, σxx, σyy, σxy, E, nu, etc.
```

## Extension: Ajouter un nouveau cas

```python
# 1. Créer nouveau fichier: mon_application.py

from fem_solver import ElasticityFEM2D
from visualization import ElasticityVisualizer

# 2. Définir votre problème
problem = ElasticityFEM2D(
    width=20.0,
    height=10.0,
    nx=61, ny=31,
    E=100e9,      # Votre matériau
    nu=0.25
)

# 3. Vos conditions aux limites
bc = {
    'bottom': [(1, 0.0)],
    'top': [(1, 0.05)],
    'left': [(0, 0.0)],
    'right': None
}

# 4. Résoudre et afficher
problem.solve(bc)
problem.compute_stress()

ElasticityVisualizer.plot_displacement_and_stress(problem, "Mon cas")
ElasticityVisualizer.plot_von_mises(problem, "Mon cas")
```

## Hiérarchie des importations

```
exemple_simple.py, exemple_avance.py
    └── from fem_solver import ElasticityFEM2D
    └── from visualization import ElasticityVisualizer
        ├── fem_solver (numpy, scipy)
        └── visualization (numpy, matplotlib)
```

**Important:** 
- fem_solver n'importe PAS matplotlib
- visualization importe matplotlib mais pas de dépendances spéciales
- Les exemples importent les deux

## Fichiers hérités

- `elasticite_2d.py` - Version intégrée (héritage)
- `exemples_avances.py` - Version intégrée (héritage)
- `README_ELASTICITE.md` - Documentation héritage

Ces fichiers sont conservés pour compatibilité mais **la version modulaire (`fem_solver.py` + `visualization.py`) est à préférer.**

## Récapitulatif

| Fichier | Rôle | Indépendant |
|---------|------|-------------|
| `fem_solver.py` | Calcul FEM | ✓ (numpy, scipy) |
| `visualization.py` | Affichage | ✓ (numpy, matplotlib) |
| `exemple_simple.py` | Cas simples | ✗ (utilise les deux) |
| `exemple_avance.py` | Cas avancés | ✗ (utilise les deux) |

La **séparation modulaire** permet une meilleure maintenance et réutilisabilité.
