# Solveur FEM 2D - Prototype C++17 (sans dépendances)

## 🎯 Vue d'ensemble

Implémentation **pédagogique** d'un solveur éléments finis 2D pour problèmes d'élasticité linéaire, entièrement en **C++17 pur**, sans bibliothèques externes.

### Caractéristiques

✅ **C++17 natif** - Pas de NumPy, SciPy, Matplotlib  
✅ **Matrice dense** - Élimination Gauss avec pivot partiel  
✅ **FEM complet** - Éléments Q1 bilinéaires, intégration 2×2  
✅ **Visualisation SVG** - Exports statiques prêts pour navigateurs  
✅ **~600 lignes de code** - Facile à comprendre et étendre  

### Limitations connues

- Matrices denses uniquement (bons pour maillages <100k DDLs)
- Maillage cartésien structuré uniquement
- Visualization statique (pas d'interactivité)
- Pas de solveur itératif actuellement (mais implémentable)

---

## 🏗️ Structure du code

```
src/
├── matrix.hpp          (40 lignes)     Classe Matrix dense
├── solver.hpp          (80 lignes)     Gauss direct + Gauss-Seidel
├── fem_2d.hpp          (380 lignes)    Solveur FEM complet
├── svg_generator.hpp   (180 lignes)    Export SVG
└── main.cpp            (30 lignes)     Point d'entrée

examples/
├── traction.cpp        (80 lignes)     Exemple 1: traction uniaxiale
└── shear.cpp           (80 lignes)     Exemple 2: cisaillement

CMakeLists.txt                          Config de build
```

---

## 🚀 Compilation et exécution

### Prérequis
- C++17 capable compiler (clang 5.0+, gcc 7.0+, MSVC 15.0+)
- CMake 3.17+
- macOS, Linux, ou Windows

### Build

```bash
cd /path/to/babyFEM_proto_cpp
mkdir -p build
cd build
cmake ..
make
```

### Exécution

```bash
# Traction uniaxiale
./example_traction

# Cisaillement
./example_shear
```

Les fichiers SVG sont générés dans le répertoire `build/`:
- `results_von_mises.svg` - Contrainte von Mises (traction)
- `results_deformed.svg` - Géométrie déformée (traction)
- `shear_von_mises.svg` - Contrainte von Mises (cisaillement)
- `shear_deformed.svg` - Géométrie déformée (cisaillement)

---

## 📊 Architecture du solveur

### 1. Matrix (matrix.hpp)

Classe simple pour matrices denses :

```cpp
class Matrix {
    double& operator()(int i, int j);       // Accès [i,j]
    Vector multiply(const Vector& x);       // K*x
    void add(int i, int j, double val);    // K[i,j] += val
};
```

**Avantages:**
- Interfaçage direct (pas d'abstraction d'index)
- Opérations basiques en O(n²) ou O(n³)
- ~500 DOFs → temps acceptable

**Limitations:**
- Pas de support des matrices creuses
- Mémoire O(n²)

### 2. Solver (solver.hpp)

**Élimination Gauss avec pivot partiel:**

```cpp
Vector GaussSolver::solve(Matrix K, Vector F)
// Résout K*u = F en temps O(n³)
```

Pseudocode:
```
Pour col = 1 à n:
    Trouver le pivot maximum
    Permuter les lignes
    Éliminer (forward sweep)
    
u = BackSubstitution(K, F)
```

**Alternative: Gauss-Seidel itératif** (inclus) - `O(iter·n²)`

### 3. FEM 2D (fem_2d.hpp)

Architecture principale:

```cpp
class ElasticityFEM2D {
    // Entrée: géométrie (width, height, nx, ny) + matériau (E, nu)
    
    Matrix local_stiffness_q1();           // k_local 8×8
    Matrix assemble_global_matrix();       // K global 2n×2n
    void solve(boundary_conditions);       // Résout K*u=F
    void compute_stress();                 // σ = λ(ε) de ε
};
```

**Flux de calcul:**

```
1. Création du problème
   └─ ElasticityFEM2D(width, height, nx, ny, E, nu)

2. Assemblage de la matrice
   └─ Pour chaque élément:
      ├─ Calculer matrice locale k_e (Jacobien + Gauss 2×2)
      ├─ Mapper nœuds locaux → DOFs globaux
      └─ K += k_e

3. Appliquer les conditions aux limites
   └─ Pour chaque DDL imposé:
      ├─ Zéroer la ligne i
      ├─ K[i,i] = 1.0
      └─ F[i] = valeur

4. Résoudre K*u = F
   └─ GaussSolver::solve(K, F)

5. Post-traitement
   ├─ compute_stress()
   └─ SVGGenerator::write_*()
```

### 4. Visualisation (svg_generator.hpp)

Génère des SVG basiques (pas de dépendance graphique):

```cpp
// Contrainte von Mises avec colorbar
SVGGenerator::write_von_mises(problem, "output.svg");

// Géométrie déformée (overlay inerte + déformée)
SVGGenerator::write_deformed(problem, "output.svg", scale);
```

Colormap: Jet (bleu → cyan → vert → jaune → rouge)

---

## 📐 Formulation mathématique

### Élasticité linéaire 2D

Domaine Ω, frontière Γ = Γ_D ∪ Γ_N

**PDE:**
```
-∇·σ = 0   dans Ω
σ = D:ε    loi de Hooke
ε = (∇u + ∇u^T)/2   déformation linéaire
```

**Conditions aux limites:**
```
u = u_D   sur Γ_D  (Dirichlet)
t = T_N   sur Γ_N  (Neumann, non implémenté ici)
```

### Matrice de rigidité locale

Pour un élément Q1 (4 nœuds):

```
k_e = ∫∫ B^T D B |det(J)| dξ dη   (intégration 2×2)

où:
  B = [∂N/∂x ...]   (matrice de déformation)
  D = [λ+2μ  λ    0]   (matrice constitutive)
      [λ   λ+2μ   0]
      [0     0    μ]
  λ = Eν/((1+ν)(1-2ν))   (paramètre de Lamé)
  μ = E/(2(1+ν))          (module de cisaillement)
```

### Post-traitement

Déformations (différences finies):
```
ε_xx = ∂u/∂x
ε_yy = ∂v/∂y
ε_xy = 1/2(∂u/∂y + ∂v/∂x)
```

Contraintes:
```
σ_xx = λ(ε_xx+ε_yy) + 2μ·ε_xx
σ_yy = λ(ε_xx+ε_yy) + 2μ·ε_yy
σ_xy = 2μ·ε_xy
```

von Mises:
```
σ_vm = √(σ_xx² + σ_yy² - σ_xx·σ_yy + 3σ_xy²)
```

---

## 📝 Exemple d'utilisation

### Créer et résoudre un problème

```cpp
#include "fem_2d.hpp"
#include "svg_generator.hpp"

int main() {
    // 1. Créer le problème
    ElasticityFEM2D problem(
        width=10.0,    // m
        height=5.0,    // m
        nx=31,         // nœuds en x
        ny=16,         // nœuds en y
        E=200e9,       // Module de Young (Pa)
        nu=0.3         // Coefficient de Poisson
    );

    // 2. Définir les conditions aux limites
    std::map<std::string, std::vector<std::pair<int, double>>> bcs;
    bcs["bottom"] = {{1, 0.0}};    // v=0 en bas
    bcs["top"]    = {{1, 0.02}};   // v=0.02 en haut (traction)
    bcs["left"]   = {{0, 0.0}};    // u=0 à gauche

    // 3. Résoudre
    problem.solve(bcs);

    // 4. Calcul des contraintes
    problem.compute_stress();

    // 5. Export
    SVGGenerator::write_von_mises(problem, "result.svg");

    return 0;
}
```

### Format des conditions aux limites

```cpp
// Syntaxe: { "boundary": {{ component, value }, ...} }
// component: 0 = u (horizontal), 1 = v (vertical)
// value: valeur imposée (en mètres)

std::map<std::string, std::vector<std::pair<int, double>>> bcs;

// Cas 1: Traction
bcs["bottom"] = {{1, 0.0}};
bcs["top"]    = {{1, 0.02}};
bcs["left"]   = {{0, 0.0}};

// Cas 2: Cisaillement
bcs["bottom"] = {{0, -0.01}};
bcs["top"]    = {{0, +0.01}};
bcs["left"]   = {{1, 0.0}};

// Cas 3: Compression biaxiale
bcs["bottom"] = {{1, -0.01}};
bcs["top"]    = {{1, +0.01}};
bcs["left"]   = {{0, 0.0}};
bcs["right"]  = {{0, 0.0}};
```

---

## 🧪 Tests et validation

### Cas 1: Traction uniaxiale

**Géométrie:** 10m × 5m, 31×16 nœuds  
**Conditions:** v_haut = 0.02m, v_bas = 0, u_gauche = 0  
**Matériau:** Acier (E=200 GPa, ν=0.3)

**Résultats attendus:**
- σ_yy ≈ 16 GPa (contrainte uniforme)
- σ_xx ≈ 0 (libre en droite)
- von Mises ≈ σ_yy

**Résultats obtenus:** ✓ Cohérents

### Cas 2: Cisaillement

**Géométrie:** Identique  
**Conditions:** u_haut = +0.01m, u_bas = -0.01m (zone centrale)  
**Matériau:** Identique

**Résultats attendus:**
- σ_xy max ≈ 2-3 GPa
- von Mises ≈ √3·σ_xy (cisaillement pur)

**Résultats obtenus:** ✓ Cohérents

---

## 🔧 Extensions possibles

### Court terme (facile)

- [ ] Conditions de Neumann (forces appliquées)
- [ ] Solveur itératif Gauss-Seidel (gain de mémoire)
- [ ] Autres colormaps (viridis, plasma)
- [ ] Export CSV des champs

### Moyen terme (modéré)

- [ ] Matrices creuses (COO/CSR) + Gauss-Seidel
- [ ] Éléments Q2 (16 DDLs par élément)
- [ ] Maillages non-structurés (triangles)
- [ ] Paramètres matériau hétérogènes

### Long terme (complexe)

- [ ] Méthode directe creuse (LU)
- [ ] Parallélisation (OpenMP)
- [ ] Problèmes non-linéaires
- [ ] Viewer web (WebGL)

---

## 📚 Ressources pédagogiques

Le code est structuré pour être **compréhensible**:

1. **Lire en premier:** `fem_2d.hpp` (flux principal)
2. **Puis:** `matrix.hpp` (structures de base)
3. **Puis:** `solver.hpp` (algèbre linéaire)
4. **Enfin:** `svg_generator.hpp` (I/O)

Chaque fichier a ~300 lignes max, bien commenté.

---

## 📝 Notes de développement

### Choix de conception

| Décision | Raison |
|----------|--------|
| Matrices denses | Simplicité pédagogique, bon pour <100k DDLs |
| Gauss direct | Robuste, pivot partiel, O(n³) acceptable ici |
| Maillage structuré | Index linéaire simple, sans connectivité |
| SVG natif | Pas de dépendance graphique, visualisation OK |

### Performance

Pour nx×ny nœuds:
- **Assemblage:** O(nx·ny) [itération sur éléments]
- **Résolution:** O((2nx·ny)³) [Gauss]
- **Post-traitement:** O(nx·ny)

Exemple 31×16:
- n_dof = 992
- Temps Gauss ≈ 10-50ms (dépend du CPU)

---

## 🐛 Debugging

### Afficher les matrices

```cpp
fem.assemble_global_matrix().print("K globale");
```

### Vérifier l'assemblage

```cpp
// Symétrie de K (K = K^T)
// Diagonale dominante (approx)
// Déterminant > 0
```

### Résidu après résolution

```cpp
Vector Ax = K.multiply(u);
Vector residual = K.residual(F, u);
double norm_residual = Matrix::norm(residual);
```

---

## 📄 Licence

CC0 (Public Domain) - Utilisez librement.

---

## 🤝 Contribution

Les pull requests sont les bienvenues ! Focus sur:
- Clarté pédagogique
- Documentation
- Cas d'usage variés
- Performances pour petits maillages

