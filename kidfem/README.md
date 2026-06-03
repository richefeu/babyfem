<p align="center">
<img src="../kidFEM-logo.png" width="40%"/>
</p>

# kidFEM

Deuxième outil de la série **babyFEM → kidFEM → …** : solveur éléments finis 2D
pour l'élasticité linéaire, en C++17 pur, sans dépendances externes.

Là où [babyFEM](../babyfem/README.md) travaille sur une grille rectangulaire régulière,
kidFEM lit un **maillage triangulaire non structuré** (au format gmsh) et permet
donc des géométries quelconques : frontières courbes, trous, formes complexes.

| | babyFEM | kidFEM |
|---|---|---|
| Géométrie | rectangle implicite | maillage explicite (gmsh) |
| Élément | quadrangle Q1 bilinéaire | triangle linéaire (CST) |
| Bords | `bottom/top/left/right` | groupes physiques nommés |
| Hypothèse | déformations planes | déformations planes |

## Structure

```
kidfem.cpp              Driver générique : ./kidfem cas.txt
Makefile                Compile kidfem

src/
├── mesh.hpp            Maillage : nœuds, triangles, groupes de bord
├── gmsh_reader.hpp     Lecture d'un maillage gmsh ASCII (format 2.2)
├── fem_tri.hpp         Moteur FEM (triangle CST, contrainte constante/élément)
├── svg_tri.hpp         Export SVG (rendu par triangles)
└── problem_io.hpp      Lecture du problème (fichier texte) + driver

cases/
├── plate.geo / .msh    Plaque rectangulaire (tension)
├── plate_tension.txt   Traction uniaxiale — patch test (CST exact)
├── ring.geo / .msh     Anneau (disque troué)
└── ring_compression.txt Compression radiale — validé contre Lamé

../core/                Briques numériques partagées avec babyFEM
├── matrix.hpp          Matrice dense
├── solver.hpp          Élimination de Gauss
├── sparse_matrix.hpp   Matrice creuse CSR
├── sparse_solver.hpp   Gradient conjugué
└── colormap.hpp        Palettes de couleurs (viridis / coolwarm)
```

## Compilation

```bash
make
```

Le binaire `kidfem` est généré dans le dossier `kidfem/`.

## Le maillage : gmsh, format 2.2

kidFEM ne lit que le **format gmsh ASCII 2.2** (le plus simple). À partir d'un
fichier de géométrie `.geo`, on génère le maillage avec :

```bash
gmsh -2 ma_geometrie.geo -o ma_geometrie.msh -format msh22
```

Le maillage doit définir :
- des **triangles** (le domaine) regroupés dans une *Physical Surface* ;
- des **segments de bord** regroupés en *Physical Line* nommées — ce sont ces
  noms qui servent ensuite à appliquer les conditions aux limites.

Exemple minimal (`cases/plate.geo`) :

```c
L = 1.0; H = 0.2; cl = 0.04;
Point(1) = {0, 0, 0, cl}; Point(2) = {L, 0, 0, cl};
Point(3) = {L, H, 0, cl}; Point(4) = {0, H, 0, cl};
Line(1) = {1,2}; Line(2) = {2,3}; Line(3) = {3,4}; Line(4) = {4,1};
Line Loop(1) = {1,2,3,4}; Plane Surface(1) = {1};
Physical Line("bottom") = {1};  Physical Line("right") = {2};
Physical Line("top")    = {3};  Physical Line("left")  = {4};
Physical Surface("plate") = {1};
Mesh.MshFileVersion = 2.2;
```

## Définir un problème par fichier texte

Comme babyFEM, on décrit le problème dans un fichier texte (un mot-clé par
ligne, `#` = commentaire) et on le résout avec le driver générique :

```bash
make
./kidfem cases/ring_compression.txt
```

Les chemins (`mesh file=…`, `output file=…`) sont résolus relativement au
dossier du fichier de cas.

### Grammaire complète

#### Maillage, matériau, solveur

```
mesh     file=<chemin.msh>
material E=<f> nu=<f>
solver   dense|sparse          # défaut : sparse (gradient conjugué)
```

- `dense`  : élimination de Gauss (direct, O(n³) — petits maillages).
- `sparse` : gradient conjugué (itératif, recommandé).

#### Conditions de Dirichlet — déplacements imposés

```
fix u|v [u|v] on "groupe" [where x|y in [<min>,<max>]]
fix u|v [u|v] at node <id>
fix u|v [u|v] at node nearest x=<f> y=<f>

set u=<f>|v=<f>  on "groupe" [where ...]
set u=<f>|v=<f>  at node <id>
set u=<f>|v=<f>  at node nearest x=<f> y=<f>
```

- `u` = composante horizontale (0), `v` = composante verticale (1).
- `fix` impose 0 ; `set` impose une valeur (en mètres). On peut lister
  plusieurs composantes : `fix u v on "left"`.
- `"groupe"` = nom d'une *Physical Line* du maillage (entre guillemets).
- `at node nearest x=… y=…` cible le **nœud le plus proche** d'un point —
  pratique pour bloquer les modes de corps rigide sans connaître les numéros
  de nœuds du maillage.
- `where x|y in [min,max]` restreint la sélection aux nœuds dont la coordonnée
  (`x` ou `y`) tombe dans l'intervalle.

#### Conditions de Neumann — efforts imposés

```
force    fx=<f> fy=<f> on "groupe" [where ...]   # force totale répartie (N)
force    fx=<f> fy=<f> at node <id>              # force ponctuelle (N)
traction tx=<f> ty=<f> on "groupe" [where ...]   # effort linéique (N/m)
pressure p=<f>         on "groupe" [where ...]   # pression normale (Pa)
```

- `force … on "groupe"` répartit également la force **totale** sur les nœuds
  sélectionnés.
- `traction` applique un effort réparti **dans des directions globales fixes**
  `(tx, ty)` (N/m), distribué de façon cohérente sur les arêtes.
- `pressure` applique une **pression normale** au bord : `p > 0` = compression
  (effort dirigé vers l'intérieur du domaine). La direction normale est
  calculée arête par arête — adapté aux frontières courbes et aux trous.

#### Sorties SVG

```
output <field> [file=...] [scale=...] [width=...] [margin=...] [bc] [mesh]
```

- `<field>` ∈ `{von_mises, stress_xx, stress_yy, stress_xy, deformed, mesh}`.
- `file`   : nom du fichier SVG (défaut `out.svg`).
- `scale`  : facteur d'amplification pour la déformée (défaut 100).
- `width`  : largeur de l'image en pixels (défaut 700).
- `margin` : marge relative autour du maillage (défaut 0.08).
- `bc`     : marque les nœuds de conditions aux limites (appuis en bleu,
  charges en orange).
- `mesh`   : superpose le maillage.

> Les contraintes étant **constantes par élément** (CST), les champs sont
> rendus en aplat (une couleur par triangle).

## Exemples fournis

### Traction d'une plaque — patch test

`cases/plate_tension.txt` : plaque encastrée à gauche, traction uniforme à
droite. L'état de contrainte est quasi uniaxial ; loin de l'encastrement
`σxx ≈ traction`. C'est le régime où le triangle linéaire est **exact** : sous
un champ de contrainte uniforme, le CST le reproduit à la précision machine.

```bash
./kidfem cases/plate_tension.txt
```

### Compression radiale d'un anneau — validation Lamé

`cases/ring_compression.txt` : anneau (rayons a=0.4, b=1.0) sous pression
externe uniforme. Géométrie **impossible avec babyFEM** (frontière courbe +
trou). Solution analytique de Lamé :

```
σ_r(r)     = -p·b²/(b²-a²)·(1 - a²/r²)
σ_θ(r)     = -p·b²/(b²-a²)·(1 + a²/r²)
σ_θ(a)     = -2·p·b²/(b²-a²)     (contrainte max, au bord du trou)
```

La pression seule étant auto-équilibrée, on bloque 3 DDL (modes de corps
rigide) via `at node nearest` sur les nœuds d'axe. von Mises calculé au bord du
trou : ≈ 24.1 MPa contre 23.8 MPa (Lamé), soit ~1 % d'écart.

```bash
./kidfem cases/ring_compression.txt
```

## Limites assumées (prototype)

- **Élément CST** : raide en flexion ; les problèmes dominés par la flexion
  exigent un maillage fin (motivation des outils suivants de la série, à
  éléments d'ordre supérieur).
- **Déformations planes** uniquement (pas de plane stress).
- **Triangles linéaires uniquement** ; pas de quadrangles, pas d'ordre 2.
- **Lecture gmsh 2.2 ASCII** seulement (exporter avec `-format msh22`).
- Contraintes rendues en aplat (constantes par élément).

## Licence

CC0 (domaine public)
