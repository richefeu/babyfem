<p align="center">
<img src="./babyFEM-logo.png" width="40%"/>
</p>

# babyFEM

Solveur éléments finis 2D pour l'élasticité linéaire, en C++17 pur, sans dépendances externes.

## Structure

```
src/
├── fem_2d.hpp          Solveur FEM (éléments Q1 bilinéaires, intégration 2×2)
├── matrix.hpp          Matrice dense
├── sparse_matrix.hpp   Matrice creuse (COO/CSR)
├── solver.hpp          Solveur direct (élimination de Gauss)
├── sparse_solver.hpp   Solveur itératif (gradient conjugué)
└── svg_generator.hpp   Export SVG

examples/
├── test_glace.cpp      Poutre appuyée avec charge répartie
└── three_span_beam.cpp Poutre sur trois appuis
```

## Compilation

```bash
cd examples
make
```

Les binaires sont générés dans `examples/`.

## Utilisation

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
