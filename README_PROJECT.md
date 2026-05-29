# Solveur d'Élasticité 2D - Structure du Projet

## 📁 Organisation des dossiers

```
shearBar/
│
├─ elasticity/              Module principal (solveur FEM)
│  ├─ __init__.py           Package Python
│  ├─ fem_solver.py         Cœur du solveur
│  ├─ visualization.py      Affichage des résultats
│  ├─ mesh.py               Gestion du maillage
│  └─ README.md             Documentation du module
│
├─ examples/                Exemples d'utilisation
│  ├─ exemple_simple.py     3 cas basiques
│  ├─ exemple_avance.py     6 cas avancés
│  ├─ exemple_selection_noeuds.py   5 cas de sélection
│  └─ README.md             Guide des exemples
│
├─ tests/                   Tests du projet
│  ├─ test_integration.py   Tests d'intégration
│  └─ README.md             Guide des tests
│
├─ docs/                    Documentation complète
│  ├─ START.md              Point de départ
│  ├─ QUICKSTART.md         Démarrage rapide
│  ├─ MAILLAGE.md           Guide du maillage
│  ├─ README.md             Documentation générale
│  ├─ STRUCTURE.md          Architecture
│  ├─ NOUVEAUTES.md         Quoi de neuf
│  ├─ INDEX.md              Index
│  ├─ FICHIERS_PRINCIPAUX.txt   Guide de navigation
│  └─ README.md             Documentation
│
└─ README_PROJECT.md        Ce fichier (vue d'ensemble)
```

## 🚀 Démarrage rapide

### 1. Installation
```bash
pip install numpy scipy matplotlib
```

### 2. Première utilisation
```bash
# Lire le guide rapide
cat docs/QUICKSTART.md

# Exécuter un exemple
python3 examples/exemple_simple.py
```

### 3. Votre cas
Adapter `examples/exemple_simple.py` pour votre application.

## 📚 Documentation

| Document | Temps | Contenu |
|----------|-------|---------|
| [START.md](docs/START.md) | 2 min | Point de départ |
| [QUICKSTART.md](docs/QUICKSTART.md) | 5 min | Démarrage rapide |
| [MAILLAGE.md](docs/MAILLAGE.md) | 15 min | Guide du maillage |
| [README.md](docs/README.md) | 20 min | Documentation complète |
| [STRUCTURE.md](docs/STRUCTURE.md) | 10 min | Architecture |

## 🐍 Modules Python

### elasticity/
Package principal contenant:
- **fem_solver.py** - Solveur FEM (numpy + scipy)
- **visualization.py** - Affichage (matplotlib)
- **mesh.py** - Gestion du maillage

```python
from elasticity import ElasticityFEM2D, ElasticityVisualizer, Mesh2D
```

## 📝 Exemples

### examples/
Trois fichiers d'exemples:
- **exemple_simple.py** - 3 cas basiques (commencer ici)
- **exemple_avance.py** - 6 cas avancés
- **exemple_selection_noeuds.py** - 5 cas de sélection

```bash
python3 examples/exemple_simple.py
```

## ✅ Tests

### tests/
Tests d'intégration:
- **test_integration.py** - Vérification complète

```bash
python3 tests/test_integration.py
```

## 💻 Exemple complet (10 lignes)

```python
from elasticity import ElasticityFEM2D, ElasticityVisualizer, Mesh2D

# Créer le maillage
mesh = Mesh2D(width=10, height=5, nx=31, ny=16)

# Sélectionner une zone au centre du haut
selected = mesh.select_nodes_on_line('top', x_min=4, x_max=6)

# Créer et résoudre le problème
problem = ElasticityFEM2D(10, 5, 31, 16, E=200e9, nu=0.3)
bc = {
    'bottom': [(1, 0.0)],
    'left': [(0, 0.0)],
    'nodes': [(n, 1, 0.02) for n in selected]
}
problem.solve(bc)
problem.compute_stress()

# Afficher les résultats
ElasticityVisualizer.plot_von_mises(problem)
```

## 🎯 Cas d'usage

### Traction simple
```bash
python3 examples/exemple_simple.py
```

### Sélections avancées
```bash
python3 examples/exemple_selection_noeuds.py
```

### Convergence du maillage
```bash
python3 examples/exemple_avance.py
```

## 📊 Capacités

✅ Solveur FEM 2D (quadrilatères Q1)  
✅ 6 méthodes de sélection de nœuds  
✅ Conditions aux limites flexibles  
✅ Calcul des contraintes (σxx, σyy, σxy, von Mises)  
✅ Visualisation complète  
✅ Exemples et tests inclus  

## 📖 Pour en savoir plus

1. Consulter [docs/QUICKSTART.md](docs/QUICKSTART.md) pour débuter
2. Consulter [docs/MAILLAGE.md](docs/MAILLAGE.md) pour comprendre le maillage
3. Consulter [docs/README.md](docs/README.md) pour la documentation complète
4. Examiner [examples/](examples/) pour des cas concrets

## 🔧 Commandes utiles

```bash
# Installer les dépendances
pip install numpy scipy matplotlib

# Afficher l'explication du maillage
python3 -c "from elasticity import explain_mesh; print(explain_mesh())"

# Lancer un exemple
python3 examples/exemple_simple.py
python3 examples/exemple_avance.py
python3 examples/exemple_selection_noeuds.py

# Vérifier que tout fonctionne
python3 tests/test_integration.py

# Consulter la documentation
cat docs/QUICKSTART.md
cat docs/MAILLAGE.md
cat docs/README.md
```

## 📋 Structure logique

```
elasticity/              ← Cœur du solveur (calcul)
    ↓
    ├─ fem_solver.py     ← Résolution FEM
    ├─ visualization.py  ← Affichage
    └─ mesh.py           ← Maillage + sélections

examples/               ← Utilisation (applications)
    └─ exemple_*.py     ← Cas d'usage

docs/                   ← Guides (documentation)
    └─ *.md             ← Explications

tests/                  ← Vérification (tests)
    └─ test_*.py        ← Tests d'intégration
```

## ✨ Dernière mise à jour

**Date:** 2026-05-29  
**Status:** ✅ Production-ready  
**Fichiers:** 14 fichiers Python/Markdown, ~4000 lignes  
**Taille:** ~120 KB  

---

**Commencez par:** [`docs/QUICKSTART.md`](docs/QUICKSTART.md) ou [`docs/START.md`](docs/START.md)
