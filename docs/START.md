# ▶️ Point de départ - Solveur d'Élasticité 2D

## 🎯 En 3 étapes (10 minutes)

### Étape 1: Lire (2 min)
```bash
cat QUICKSTART.md
```

### Étape 2: Exécuter (5 min)
```bash
python3 exemple_simple.py
```

### Étape 3: Adapter (3 min)
Modifiez `exemple_simple.py` pour votre cas ou créez un nouveau fichier.

## 📂 Fichiers à utiliser

### Pour commencer
- **QUICKSTART.md** ← Lire en premier
- **exemple_simple.py** ← Exécuter
- **INDEX.md** ← Navigation

### Pour votre cas
```python
from fem_solver import ElasticityFEM2D
from visualization import ElasticityVisualizer

problem = ElasticityFEM2D(width=10, height=5, nx=31, ny=16, E=200e9, nu=0.3)
problem.solve({'bottom': [(1,0)], 'top': [(1,0.02)], 'left': [(0,0)], 'right': None})
problem.compute_stress()

ElasticityVisualizer.plot_von_mises(problem)
```

### Pour comprendre
- **README.md** - Documentation complète
- **STRUCTURE.md** - Architecture modulaire

## 🔧 Vérifier que tout fonctionne
```bash
python3 test_integration.py
```

## 📖 Documentation

| Fichier | Lecture | Contenu |
|---------|---------|---------|
| **QUICKSTART.md** | 5 min | Démarrage rapide ⭐ |
| **INDEX.md** | 3 min | Navigation |
| **README.md** | 15 min | Documentation complète |
| **STRUCTURE.md** | 10 min | Architecture |

## 🐍 Modules Python

| Fichier | Utilisation |
|---------|------------|
| `fem_solver.py` | `from fem_solver import ElasticityFEM2D` |
| `visualization.py` | `from visualization import ElasticityVisualizer` |

## 🎬 Exemples

```bash
python3 exemple_simple.py      # 3 cas basiques
python3 exemple_avance.py      # 6 cas avancés
```

---

**C'est tout! Commencez par QUICKSTART.md → exemple_simple.py**
