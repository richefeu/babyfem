# Cas Test_Glace: Poutre Appuyée avec Charge Répartie

## 📐 Description du problème

C'est un cas classique de **flexion de poutre** - essentiellement une plaque mince soumise à:
- **Appuis simples** en deux zones aux extrémités inférieures
- **Charge répartie** au centre du bord supérieur
- **Blocage latéral** pour éviter les translations parasites

## 🔧 Paramètres géométriques

| Paramètre | Valeur | Unité |
|-----------|--------|-------|
| Longueur (L) | 0.1 | m |
| Hauteur (h) | 0.02 | m |
| Rapport L/h | 5 | - |
| Nœuds en x (nx) | 120 | - |
| Nœuds en y (ny) | 12 | - |
| Total nœuds | 1440 | - |
| Total DDLs | 2880 | - |

**Domaine:** 0.1 m × 0.02 m (poutre mince et longue)

## 🧪 Paramètres matériau

| Paramètre | Valeur | Unité |
|-----------|--------|-------|
| Module de Young (E) | 210 | GPa |
| Coefficient de Poisson (ν) | 0.3 | - |
| Matériau | Acier | - |

## 🔨 Conditions aux limites

### Appuis
- **Appui gauche:** nœuds en bas entre x=0 et x=L/3 → **v = 0** (33 nœuds)
- **Appui droit:** nœuds en bas entre x=2L/3 et x=L → **v = 0** (33 nœuds)

### Charge
- **Zone de charge:** nœuds en haut entre x=L/3+δ et x=2L/3-δ → **v = -0.0001 m** (44 nœuds)
  - δ = -0.002 m (offset pour éviter les singularités aux appuis)

### Blocage latéral
- **Gauche:** u = 0 (empêcher translation horizontale)

**Résumé visuel:**
```
        Charge vers le bas
        ↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓
    ┌─────────────────────┐
    │  L/3+δ    2L/3-δ    │
    │  <------->          │
  ┌─┴──────────────────────┴─┐
  │ Appui gauche  |  Appui   │
  └─────────────────────────┘
       L/3          2L/3
    <-----> L/3 <----->
```

## 📊 Résultats attendus

### Déplacements
- Flèche maximale au centre: v_min < 0 (vers le bas)
- Déplacement horizontal maximal: u_max ≈ 0 (blocage à gauche)
- Déplacement imposé en charge: v = -0.0001 m

### Contraintes
- **σxx (contrainte axiale):** Variation linéaire dans l'épaisseur (traction/compression)
- **σyy (contrainte transversale):** Minimal (charge pas directement appliquée)
- **σxy (cisaillement):** Fort au centre (discontinuité de charge)
- **von Mises:** Maximum aux zones de concentration (près des appuis et de la charge)

### Phénomènes physiques
1. **Flexion:** Poutre se courbe sous le chargement
2. **Cisaillement:** Transfert de charge des appuis vers le centre
3. **Concentration de contrainte:** Pics près des discontinuités (appuis, charge)
4. **Effet Poisson:** Contraction latérale mineure (ν=0.3)

## 🚀 Exécution

### Compiler
```bash
cd build
cmake ..
make example_test_glace
```

### Exécuter
```bash
./example_test_glace
```

### Sortie
```
Géométrie: 0.1m × 0.02m, 120×12 nœuds, 2880 DDLs
Matériau: Acier (E=210 GPa, ν=0.3)

Appui gauche: 40 nœuds
Appui droit: 40 nœuds  
Zone de charge: 44 nœuds

Résultats:
  Flèche: -0.0001 m
  σxx max: -6.66 GPa (compression)
  σyy max: -15.5 GPa
  σxy max: 4.81 GPa (cisaillement)
  von Mises max: 15.9 GPa
```

## 📁 Fichiers générés

```
build/
├── test_glace_von_mises.svg    (124 KB) - Visualisation contrainte
└── test_glace_deformed.svg     (209 KB) - Géométrie déformée
```

**Interprétation des visualisations:**
- **test_glace_von_mises.svg:** Carte de couleurs (Jet: bleu=faible → rouge=fort)
  - Zones bleues: contrainte faible (appuis)
  - Zones rouges: contrainte forte (centre, flexion)
  
- **test_glace_deformed.svg:** Overlay géométrie
  - Gris clair: géométrie initiale
  - Bleu: géométrie déformée (flèche visible au centre)

## 🎯 Points d'intérêt pédagogiques

1. **Sélection de nœuds conditionnelle** (`select_nodes_on_line`)
   - Appuis non continus (deux zones séparées)
   - Charge répartie (zone centrale)

2. **Conditions aux limites complexes**
   - Mélange de conditions sur bords et sur nœuds spécifiques
   - Application manuelle des BC pour flexibilité

3. **Phénomènes mécaniques visibles**
   - Flexion (déformation)
   - Cisaillement (contrainte)
   - Concentration de contrainte (singularité)

4. **Validation numérique possible**
   - Comparaison avec théorie poutre (RdM)
   - Flèche théorique: f = 5·P·L⁴/(384·E·I)
   - Contrainte théorique: σ = M·y/I

## 🔬 Comparaison Python vs C++17

### Python
```bash
python3 examples/test_glace.py
# → Affichage matplotlib interactif
# → Résolution immédiate (SciPy optimisé)
```

### C++17
```bash
./example_test_glace
# → Affichage console détaillé
# → Export SVG navigateur
# → Code source visible et modifiable
```

## 📝 Code source

### Python (`elasticity/fem_solver.py`)
```python
problem = ElasticityFEM2D(L, h, nx, ny, E, nu)
bc = {
    'left': [(0, 0.0)],
    'nodes': [
        *[(n, 1, 0.0) for n in appuis],
        *[(n, 1, -depl) for n in zone_charge]
    ]
}
problem.solve(bc)
```

### C++17 (`examples/test_glace.cpp`)
```cpp
ElasticityFEM2D problem(L, h, nx, ny, E, nu);

auto appui_gauche = problem.select_nodes_on_line("bottom", 0.0, Lappui);
auto appui_droit = problem.select_nodes_on_line("bottom", L-Lappui, L);
auto zone_charge = problem.select_nodes_on_line("top", Lappui+delta, L-Lappui-delta);

Matrix K = problem.assemble_global_matrix();
Vector F(n_dof, 0.0);

// Appliquer BC...
Vector u = GaussSolver::solve(K, F);
```

## 🔮 Extensions possibles

1. **Variation de paramètres**
   - Rapport L/h different (plaque carrée, très longue)
   - Matériau différent (E, ν)
   - Déplacement imposé variable

2. **Chargements alternatifs**
   - Force ponctuelle (Dirac)
   - Charge uniformément répartie (intégrer sur zone)
   - Charge triangulaire

3. **Analyse de convergence**
   - Raffinement du maillage (nq, ny)
   - Étude de la flèche vs maillage
   - Taux de convergence

4. **Comparaison avec théorie**
   - Calcul RdM (flèche théorique)
   - Erreur relative FEM vs RdM
   - Validation numérique

## 📚 Références théoriques

### Théorie des poutres (RdM)
Pour une poutre simplement appuyée avec charge ponctuelle au centre:
```
Flèche: f = P·L³/(48·E·I)
Contrainte max: σ = M·c/I  où M = P·L/4
```

Pour charge répartie:
```
Flèche max: f = 5·q·L⁴/(384·E·I)
Contrainte max: σ = q·L²/8·c/I
```

### FEM vs RdM
- FEM: résout le problème 2D complet (σxx, σyy, σxy)
- RdM: hypothèses simplifiées (sections planes, etc.)
- Différences: négligeables pour L/h >> 1 (poutre mince)

## 🎓 Objectif pédagogique

Ce cas enseigne:

1. **Problème d'ingénierie réel:** Flexion de poutre classique
2. **Sélection conditionnelle de nœuds:** Appuis et charges complexes
3. **Conditions aux limites mixtes:** Bords + nœuds spécifiques
4. **Analyse mécanique:** Déformations et contraintes
5. **Validation:** Comparaison avec la théorie des poutres

C'est un **benchmark pédagogique excellent** pour progresser dans le code!

---

**Status:** ✅ Implémenté en C++17, fonctionnel, prêt pour extensions
