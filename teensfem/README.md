<p align="center">
<img src="../teensFEM-logo.png" width="40%"/>
</p>

# teensFEM

Troisième outil de la série (voir le [README racine](../README.md)). Analyse de
**structures à barres et poutres 2D** — portiques et treillis — en C++17 pur,
sans dépendances externes.

Contrairement à babyFEM et kidFEM (milieux continus maillés), teensFEM travaille
sur des **éléments linéiques** : la « géométrie » est une liste de nœuds reliés
par des poutres, dont chaque extrémité est soit **encastrée** (rigide, le moment
passe), soit **articulée** (rotule, libre en rotation).

| | élément | DDL/nœud | sortie typique |
|---|---|---|---|
| babyFEM / kidFEM | surface (Q1 / triangle) | 2 (u, v) | champ de contrainte |
| **teensFEM** | poutre (Euler-Bernoulli + axial) | 3 (u, v, θ) | diagrammes M, N, V |

## Structure

```
teensfem.cpp            Driver générique : ./teensfem cas.txt
Makefile                Compile teensfem

src/
├── frame.hpp           Élément poutre 2D, relâchements, assemblage, solveur
├── svg_frame.hpp       Export SVG (structure, déformée, diagrammes M/N/V)
└── problem_io.hpp      Lecture du problème (fichier texte) + driver

cases/
├── beam_udl.txt        Poutre sur 2 appuis, charge répartie (validation w L²/8)
├── portal.txt          Portique encastré (encastrements + effort latéral)
└── truss.txt           Treillis king-post (tout articulé)

../core/                Briques numériques partagées (matrice dense, Gauss)
```

## Compilation

```bash
make
./teensfem cases/portal.txt
```

## Modèle mécanique

- **Élément** : poutre-colonne 2D Euler-Bernoulli (flexion) + effort normal,
  3 DDL par nœud : `u` (horizontal), `v` (vertical), `r` (rotation).
- **Connexions** : chaque extrémité de poutre est articulée par défaut (moment
  nul, rotation condensée statiquement) ou encastrée (`rigid=…`). Une poutre
  **articulée aux deux bouts** se réduit à une **barre de treillis** (effort
  normal seul) — c'est pourquoi le même outil traite portiques et treillis.
- **Mécanismes** : un nœud dont toutes les barres incidentes sont articulées a
  une rotation sans raideur ; elle est automatiquement bloquée (sans effet
  physique), ce qui garde le système non singulier.
- **Hypothèses** : petits déplacements, matériau élastique linéaire.

## Grammaire du fichier de problème

Un mot-clé par ligne, `#` = commentaire.

### Nœuds et sections

```
node <id> x=<f> y=<f>
section E=<f> A=<f> I=<f>          # propriétés par défaut (module, aire, inertie)
```

### Poutres

```
beam <id1> <id2> [rigid=start|end|both|none]
                 [E=<f>] [A=<f>] [I=<f>]      # surcharge la section par défaut
                 [wx=<f>] [wy=<f>]            # charge répartie globale (N/m)
                 [wn=<f>] [wt=<f>]            # charge répartie locale (N/m)
```

- Défaut : **articulé-articulé** (rotule aux deux extrémités).
- `rigid=both` : encastré aux deux bouts ; `rigid=start` / `rigid=end` : un seul
  bout encastré (l'autre reste articulé) ; `rigid=none` : explicitement articulé.
- `start` = nœud `id1`, `end` = nœud `id2`.
- **Charge répartie uniforme sur toute la longueur**, dans la direction de ton
  choix. Deux repères, combinables :
  - **global** : `wx`, `wy` (p. ex. `wy=-1000` pour 1 kN/m vers le bas — gravité,
    neige…) ;
  - **local** : `wn` perpendiculaire à la barre, `wt` le long de la barre
    (p. ex. une pression sur un versant incliné : `wn=-800`). `wn` positif pointe
    à 90° dans le sens trigonométrique depuis l'axe `id1→id2`.

### Appuis (DDL bloqués)

```
fix u|v|r [u|v|r] at node <id>
```

- `u`, `v`, `r` = translations horizontale/verticale et rotation.
- `fix u v at node 0` → articulation (appui double) ; `fix v at node 4` → appui
  simple ; `fix u v r at node 0` → encastrement.

### Charges nodales

```
load fx=<f> fy=<f> m=<f> at node <id>
```

Forces (`fx`, `fy`, en N) et moment (`m`, en N·m) appliqués à un nœud.

### Sorties SVG

```
output <field> [file=...] [width=...] [scale=...] [dscale=...]
```

- `<field>` ∈ `{structure, deformed, moment, shear, normal}`.
- `scale`  : amplification de la déformée (auto si absent).
- `dscale` : échelle des diagrammes d'efforts (auto si absent).
- `width`  : largeur de l'image en pixels (défaut 800).

Le champ **`structure`** (alias `loads`) produit un **schéma de chargement
complet**, dont voici la légende.

## Légende des symboles

### Liaisons d'appui (en vert)

Le symbole dépend des DDL bloqués au nœud (`fix …`) :

| Appui | DDL bloqués | Symbole |
|---|---|---|
| **Encastrement** | `u v r` | carré |
| **Articulation** (appui double) | `u v` | triangle (pointe sur le nœud) |
| **Appui simple** (rouleau) | un seul, p. ex. `v` | triangle + trait sous la base |

### Liaison interne barre ↔ nœud

| Extrémité de barre | Réglage | Symbole |
|---|---|---|
| **Encastrée** (le moment passe) | `rigid=…` | aucun : la barre est continue avec le nœud |
| **Articulée** (rotule, moment nul) | défaut | petit **cercle ouvert** à l'extrémité de la barre |

> Sur un treillis (tout articulé) chaque barre porte donc un cercle à ses deux
> bouts ; sur un portique rigide les angles n'ont aucun cercle.

### Charges

| Charge | Symbole | Couleur |
|---|---|---|
| **Force nodale** (`load fx/fy`) | flèche droite pointant vers le nœud + valeur (kN) | violet |
| **Moment nodal** (`load m`) | flèche **courbe** (sens trigonométrique si `m > 0`) + valeur (kN·m) | violet |
| **Charge répartie** (`wx/wy/wn/wt`) | série de flèches régulières le long de la barre, **dans la direction réelle de la charge**, reliées par un trait, + valeur (kN/m) | bleu-vert |

Les nœuds sont des points noirs, les barres des traits noirs. Les valeurs sont
affichées avec un préfixe `k` automatique au-delà de 1000.

### Diagrammes d'efforts (`moment`, `shear`, `normal`)

Le diagramme est tracé **perpendiculairement à chaque barre**, rempli en **bleu**
pour les valeurs positives et en **orange** pour les négatives ; la valeur
maximale est indiquée en bas de l'image. Conventions de signe : effort normal
`N` positif en **traction**, moment `M` positif quand la **fibre inférieure est
tendue** (sagging), `V = dM/dx`. La déformée (`deformed`) est tracée en rouge
par-dessus la structure non déformée en gris.

## Exemples fournis

### Poutre sur 2 appuis — validation
`cases/beam_udl.txt` : poutre continue (4 tronçons rigides) sous charge répartie.
Moment à mi-portée calculé = 2000 N·m = `w L²/8`, flèche = `5 w L⁴/(384 EI)`
(exact).

### Portique — encastrements
`cases/portal.txt` : deux poteaux encastrés au sol, traverse chargée + effort
latéral. Le moment passe dans les angles rigides ; le diagramme de moment montre
la reprise de l'effort horizontal par flexion.

### Treillis — articulations
`cases/truss.txt` : king-post entièrement articulé. Chaque barre ne reprend que
de l'effort normal (M ≈ 0), illustrant le comportement de treillis obtenu
simplement par le défaut rotule-rotule.

### Ferme de toiture — charge perpendiculaire (`wn`)
`cases/ferme.txt` : deux arbalétriers inclinés et un entrait (tout articulé)
sous une charge **perpendiculaire au versant** (`wn`, neige/vent). Chaque
arbalétrier, articulé à ses deux bouts, fléchit comme une poutre sur 2 appuis :
moment à mi-arbalétrier = `wn·L²/8 ≈ 3250 N·m`, tandis que la ferme transmet la
charge aux appuis par efforts normaux (arbalétriers comprimés, entrait tendu).

### Schéma de chargement — toutes liaisons et charges
`cases/loading_demo.txt` : un modèle réunissant les trois types d'appuis, une
rotule interne, force et moment nodaux, charge répartie globale et
perpendiculaire — pour illustrer la sortie `output structure`.

## Limites assumées (prototype)

- Petits déplacements (analyse linéaire du premier ordre, pas de flambement).
- Charges réparties uniformes seulement (pas de charge ponctuelle en travée :
  ajouter un nœud à l'emplacement voulu).
- Déformée tracée en segments droits entre nœuds (pas d'interpolation cubique).
- Solveur direct dense (suffisant pour des structures de taille modeste).

## Licence

CC0 (domaine public)
