# Série FEM

Une famille d'outils éléments finis 2D pédagogiques, en C++17 pur, sans
dépendances externes. Chaque palier ajoute **une** capacité majeure au
précédent, en réutilisant un noyau numérique commun.

```
babyFEM → kidFEM → teensFEM → adultFEM → seniorFEM
```

| Outil | Capacité ajoutée | État |
|---|---|---|
| **[babyFEM](babyfem/)** | grille rectangulaire structurée, quadrangles Q1, élasticité linéaire | ✅ |
| **[kidFEM](kidfem/)** | maillages triangulaires non structurés (import gmsh), géométries quelconques | ✅ |
| teensFEM | éléments d'ordre supérieur, plane stress/strain au choix | à venir |
| adultFEM | non-linéarité / dynamique | à venir |
| seniorFEM | 3D, multi-physique | à venir |

<p align="center">
<img src="./babyFEM-logo.png" width="24%"/>
<img src="./kidFEM-logo.png" width="24%"/>
<img src="./teensFEM-logo.png" width="24%"/>
<img src="./adultFEM-logo.png" width="24%"/>
</p>

## Organisation du dépôt

```
core/        Briques numériques partagées (en-têtes seuls)
├── matrix.hpp          Matrice dense
├── solver.hpp          Élimination de Gauss
├── sparse_matrix.hpp   Matrice creuse CSR
├── sparse_solver.hpp   Gradient conjugué
└── colormap.hpp        Palettes de couleurs SVG

babyfem/     Outil 1 — grille structurée (voir babyfem/README.md)
kidfem/      Outil 2 — maillages triangulaires gmsh (voir kidfem/README.md)

Makefile     Construit tous les outils
```

Le principe de partage est volontairement mesuré : `core/` ne contient que des
briques simples et stables. Dès qu'une fonctionnalité devient spécifique à un
outil (génération de maillage, rendu SVG, grammaire du fichier de problème),
chaque outil garde **sa** version, pour préserver la lisibilité pédagogique.

## Compilation

```bash
make            # construit babyfem et kidfem
make babyfem    # un seul outil
make kidfem
make examples   # exemples C++ historiques de babyfem
make clean
```

Chaque binaire est généré dans le dossier de son outil (`babyfem/babyfem`,
`kidfem/kidfem`).

## Utilisation

Chaque outil lit un problème décrit dans un fichier texte :

```bash
babyfem/babyfem babyfem/cases/three_span.txt
kidfem/kidfem   kidfem/cases/ring_compression.txt
```

Voir le README de chaque outil pour la grammaire détaillée du fichier de
problème :
- [babyfem/README.md](babyfem/README.md)
- [kidfem/README.md](kidfem/README.md)

## Licence

CC0 (domaine public)
