# Index du projet - Solveur d'Élasticité 2D

## 📁 Organisation du répertoire

```
shearBar/
├── 🐍 MODULES PYTHON (Cœur du projet)
│   ├── fem_solver.py              [9,5 KB]  Calcul FEM - À importer
│   ├── visualization.py           [8,1 KB]  Affichage - À importer
│   ├── mesh.py                    [10  KB]  Gestion du maillage ⭐ NOUVEAU
│   └── test_integration.py        [4,8 KB]  Tests - À exécuter une fois
│
├── 📚 EXEMPLES (À exécuter ou adapter)
│   ├── exemple_simple.py          [5,7 KB]  3 cas basiques
│   ├── exemple_avance.py          [8,7 KB]  6 cas avancés
│   └── exemple_selection_noeuds.py [7  KB]  5 cas de sélection ⭐ NOUVEAU
│
└── 📖 DOCUMENTATION (À lire selon vos besoins)
    ├── START.md                   [0,5 KB]  Point de départ
    ├── QUICKSTART.md              [6,1 KB]  Démarrage rapide ⭐
    ├── MAILLAGE.md                [10  KB]  Guide du maillage ⭐ NOUVEAU
    ├── README.md                  [8,6 KB]  Documentation complète
    ├── STRUCTURE.md               [8,4 KB]  Architecture détaillée
    ├── FICHIERS_PRINCIPAUX.txt    [10  KB]  Guide de navigation
    └── INDEX.md                           Ce fichier
```

## 🚀 Par où commencer?

### 1️⃣ Lire (5 minutes)
```bash
cat QUICKSTART.md
```

### 2️⃣ Exécuter les exemples
```bash
python3 exemple_simple.py
```

### 3️⃣ Créer votre cas
```python
from fem_solver import ElasticityFEM2D
from visualization import ElasticityVisualizer

# Votre code ici...
```

## 📋 Description des fichiers

### MODULES PYTHON - À IMPORTER

#### `fem_solver.py` (9,5 KB)
**Cœur du calcul FEM**
- Classe `ElasticityFEM2D`
- Résolution du système linéaire
- Calcul des contraintes
- Dépendances: `numpy`, `scipy`
- **Réutilisable** dans d'autres projets

```python
from fem_solver import ElasticityFEM2D
problem = ElasticityFEM2D(width, height, nx, ny, E, nu)
```

#### `visualization.py` (8,1 KB)
**Module d'affichage indépendant**
- Classe `ElasticityVisualizer`
- 6 méthodes de visualisation
- Dépendances: `numpy`, `matplotlib`
- **Indépendant** du solveur

```python
from visualization import ElasticityVisualizer
ElasticityVisualizer.plot_von_mises(problem)
```

#### `test_integration.py` (4,8 KB)
**Tests d'intégration**
- Vérifie que tous les modules fonctionnent
- 5 tests différents
- À exécuter une fois pour confirmer

```bash
python3 test_integration.py
```

### EXEMPLES - À EXÉCUTER OU ADAPTER

#### `exemple_simple.py` (5,7 KB)
**3 cas basiques - Point de départ idéal**
1. Traction uniaxiale
2. Cisaillement partiel
3. Influence du coefficient de Poisson

```bash
python3 exemple_simple.py
```

#### `exemple_avance.py` (8,7 KB)
**6 cas avancés - Pour aller plus loin**
1. Poutre encastrée (flexion)
2. Étude de convergence du maillage
3. Comparaison de matériaux (Acier/Alu/Titane)
4. Cisaillement pur
5. Variation du coefficient de Poisson (5 valeurs)
6. Analyse de sensibilité au module de Young

```bash
python3 exemple_avance.py
```

### DOCUMENTATION - À LIRE

#### `QUICKSTART.md` (6,1 KB)
**⭐ À LIRE EN PREMIER**
- Démarrage en 5 minutes
- Cas courants (traction, flexion, cisaillement)
- Matériaux typiques
- Exemples de code

#### `README.md` (8,6 KB)
**Documentation complète**
- Architecture modulaire
- Toutes les fonctionnalités
- Paramètres et résultats
- Hypothèses et limitations

#### `STRUCTURE.md` (8,4 KB)
**Architecture détaillée**
- Organigramme des modules
- Flux de données
- Séparation des responsabilités
- Comment étendre le code

#### `FICHIERS_PRINCIPAUX.txt` (10 KB)
**Guide de navigation**
- Vue d'ensemble
- Fichiers à utiliser vs ignorer
- Utilisation typique
- Statut de chaque composant

#### `INDEX.md`
**Ce fichier**
- Organisation du répertoire
- Description rapide de chaque fichier

## 🎯 Parcours utilisateur recommandé

### Pour un utilisateur **impatient** (10 min)
```
1. Lire QUICKSTART.md (5 min)
2. python3 exemple_simple.py (5 min)
```

### Pour un utilisateur **complet** (30 min)
```
1. Lire QUICKSTART.md
2. python3 exemple_simple.py
3. Lire STRUCTURE.md
4. Adapter exemple_simple.py pour votre cas
```

### Pour un développeur (1h)
```
1. Lire README.md
2. Lire STRUCTURE.md
3. Lancer test_integration.py
4. Étudier fem_solver.py
5. Étudier visualization.py
6. Créer votre propre cas
7. Ajouter des tests
```

## 💾 Dépendances

```
numpy              Calculs numériques
scipy              Algèbre linéaire creuse
matplotlib         Visualisation (optionnel pour fem_solver)
```

Installation:
```bash
pip install numpy scipy matplotlib
```

## ✅ Vérification

Pour vérifier que tout fonctionne:
```bash
python3 test_integration.py
```

Résultat attendu:
```
✓ Solveur OK
✓ von Mises OK
✓ plot_displacement_and_stress OK
✓ plot_deformed_geometry OK
✓ plot_von_mises OK
✓ Cisaillement partiel OK
✓ Export résultats OK

✓ TOUS LES TESTS RÉUSSIS
```

## 🔧 Utilisation typique

```python
from fem_solver import ElasticityFEM2D
from visualization import ElasticityVisualizer

# Créer
problem = ElasticityFEM2D(10, 5, 31, 16, 200e9, 0.3)

# Résoudre
bc = {
    'bottom': [(1, 0.0)],
    'top': [(1, 0.02)],
    'left': [(0, 0.0)],
    'right': None
}
problem.solve(bc)

# Calculer
problem.compute_stress()

# Afficher
ElasticityVisualizer.plot_von_mises(problem, "Mon cas")
```

## 📊 Taille des fichiers

| Fichier | Taille | Type |
|---------|--------|------|
| fem_solver.py | 9,5 KB | Module |
| visualization.py | 8,1 KB | Module |
| exemple_simple.py | 5,7 KB | Exemple |
| exemple_avance.py | 8,7 KB | Exemple |
| test_integration.py | 4,8 KB | Test |
| **Documentation** | **~40 KB** | Guide |
| **Total** | **~80 KB** | Compact! |

## 🚀 Prochaines étapes

1. **QUICKSTART.md** - 5 min
2. **exemple_simple.py** - 5 min
3. **Adapter pour votre cas** - ?

---

**Dernière mise à jour:** 2026-05-29  
**Status:** ✅ Production-ready
