<p align="center">
<img src="../babyFEM-logo.png" width="40%"/>
</p>

# babyFEM

Premier outil de la série (voir le [README racine](../README.md) :
babyFEM → kidFEM → teensFEM → adultFEM → seniorFEM).


Solveur éléments finis 2D pour l'élasticité linéaire, en C++17 pur, sans dépendances externes.

## Structure

```
babyfem.cpp             Driver générique : ./babyfem cas.txt
Makefile                Compile babyfem

src/
├── fem_2d.hpp          Solveur FEM (éléments Q1 bilinéaires, intégration 2×2)
├── matrix.hpp          Matrice dense
├── sparse_matrix.hpp   Matrice creuse (COO/CSR)
├── solver.hpp          Solveur direct (élimination de Gauss)
├── sparse_solver.hpp   Solveur itératif (gradient conjugué)
├── problem_io.hpp      Lecture d'un problème depuis un fichier texte
└── svg_generator.hpp   Export SVG

cases/                  Problèmes décrits en fichiers texte
├── point_load_beam.txt force ponctuelle (Neumann) + validation poutre
├── test_glace.txt
└── three_span.txt

examples/               Versions C++ équivalentes (historique)
├── test_glace.cpp
└── three_span_beam.cpp
```

## Compilation

```bash
make
```

Le binaire `babyfem` est généré à la racine.

## Définir un problème par fichier texte

Plutôt que d'écrire du C++, on décrit un problème dans un simple fichier texte
et on le résout avec le driver générique :

```bash
make
./babyfem cases/three_span.txt
```

Le fichier sépare les *données* du *moteur*. Format (un mot-clé par ligne,
`#` = commentaire) :

```
# géométrie et discrétisation
mesh width=1.0 height=0.1 nx=100 ny=10

# matériau (élasticité linéaire isotrope)
material E=210e9 nu=0.3

# solveur : dense (Gauss) ou sparse (gradient conjugué). Défaut : sparse
solver dense

# Conditions aux limites de Dirichlet = déplacements imposés
#   fix  u|v       on <edge> [where x|y in [min,max]]   → impose 0
#   fix  u|v       at node <int>
#   set  u|v=<val> on <edge> [where x|y in [min,max]]
#   set  u|v=<val> at node <int>
# edge ∈ {bottom, top, left, right} ; u = horizontal, v = vertical
fix u at node 0
fix v on bottom where x in [-0.015, 0.015]

# Charges de Neumann = efforts imposés (le déplacement est l'inconnue)
#   force    fx=<f> fy=<f> at node <int>                 → force ponctuelle (N)
#   force    fx=<f> fy=<f> on <edge> [where ...]         → force totale répartie sur la sélection
#   traction tx=<t> ty=<t> on <edge> [where ...]         → effort réparti (N/m)
force fy=-100000 on top where x in [0.49, 0.51]

# Sorties SVG
#   output <field> [file=...] [scale=...] [width=...] [margin=...] [bc] [mesh]
# field ∈ {von_mises, stress_xx, stress_yy, stress_xy, mesh, deformed}
output von_mises file=res_vm.svg bc
output deformed  file=res_def.svg scale=100 mesh
```

On peut donc imposer soit des **déplacements** (Dirichlet, `fix`/`set`), soit des
**efforts** (Neumann, `force`/`traction`). Le cas `cases/point_load_beam.txt`
applique une force ponctuelle et permet de comparer la flèche à la théorie des
poutres `δ = PL³/48EI`.

## Utilisation (API C++)

```cpp
#include "fem_2d.hpp"
#include "svg_generator.hpp"

ElasticityFEM2D problem(width, height, nx, ny, E, nu);

std::map<std::string, std::vector<std::pair<int,double>>> bcs;
bcs["bottom"] = {{1, 0.0}};
bcs["top"]    = {{1, 0.02}};
bcs["left"]   = {{0, 0.0}};

problem.solve(bcs);
problem.compute_stress();
SVGGenerator::write_von_mises(problem, "result.svg");
```

## Licence

CC0 (domaine public)
