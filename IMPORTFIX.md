# Fix des imports après réorganisation

## Problème
Après la réorganisation du projet en dossiers (elasticity/, examples/, docs/, tests/), les imports dans les fichiers d'exemples et de tests ne fonctionnaient plus.

## Solution
Ajout de `sys.path.insert()` au début de chaque fichier exécutable pour ajouter le répertoire parent au chemin Python.

## Fichiers corrigés
- ✅ examples/exemple_simple.py
- ✅ examples/exemple_avance.py
- ✅ examples/exemple_selection_noeuds.py
- ✅ tests/test_integration.py

## Code ajouté
```python
import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
```

Cela permet aux fichiers dans les sous-dossiers d'importer depuis le répertoire racine:
```python
from elasticity import ElasticityFEM2D, ElasticityVisualizer, Mesh2D
```

## Vérification
✅ `python3 examples/exemple_simple.py` → Fonctionne
✅ `python3 examples/exemple_avance.py` → Fonctionne
✅ `python3 examples/exemple_selection_noeuds.py` → Fonctionne
✅ `python3 tests/test_integration.py` → Tous les tests passent

## Alternative (pour une utilisation depuis n'importe quel dossier)
```bash
# Depuis la racine du projet
python3 -m examples.exemple_simple
```

Cela fonctionne aussi grâce à la structure du package.
