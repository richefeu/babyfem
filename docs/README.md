# Documentation

Guide complet du projet solveur d'élasticité 2D.

## Point de départ

1. **START.md** - Vue d'ensemble rapide
2. **QUICKSTART.md** - Démarrage en 5 minutes

## Guides détaillés

- **MAILLAGE.md** - Comprendre le maillage et sélectionner les nœuds
- **README.md** - Documentation générale complète
- **STRUCTURE.md** - Architecture du projet
- **NOUVEAUTES.md** - Résumé des améliorations

## Navigation

- **INDEX.md** - Index complet des fichiers
- **FICHIERS_PRINCIPAUX.txt** - Guide de navigation

## Organisation

```
docs/
├─ START.md                  Commencer ici
├─ QUICKSTART.md             5 minutes
├─ MAILLAGE.md               Guide du maillage
├─ README.md                 Documentation générale
├─ STRUCTURE.md              Architecture
├─ NOUVEAUTES.md             Quoi de neuf
├─ INDEX.md                  Index
├─ FICHIERS_PRINCIPAUX.txt   Navigation
└─ README.md                 Ce fichier
```

## Parcours recommandé

### Débutant (15 min)
1. Lire: START.md
2. Lire: QUICKSTART.md
3. Exécuter: exemple_simple.py

### Intermédiaire (45 min)
1. Lire: MAILLAGE.md
2. Exécuter: exemple_selection_noeuds.py
3. Créer votre cas

### Avancé (2h+)
1. Lire: README.md + STRUCTURE.md
2. Étudier: elasticity/fem_solver.py
3. Étudier: elasticity/mesh.py
4. Implémenter des extensions

## Documentation en ligne

Pour afficher l'explication du maillage:
```bash
python3 -c "from elasticity import explain_mesh; print(explain_mesh())"
```

## Vos questions?

Consultez:
- MAILLAGE.md pour le maillage et les sélections
- README.md pour les paramètres et l'utilisation
- STRUCTURE.md pour l'architecture
