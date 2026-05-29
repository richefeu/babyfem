# Exemples d'utilisation

## Fichiers d'exemples

### `exemple_simple.py`
3 cas basiques pour débuter:
1. Traction uniaxiale
2. Cisaillement partiel
3. Influence du coefficient de Poisson

```bash
python3 exemple_simple.py
```

### `exemple_avance.py`
6 cas avancés pour aller plus loin:
1. Poutre encastrée en flexion
2. Étude de convergence du maillage
3. Comparaison de matériaux
4. Cisaillement pur
5. Variation du coefficient de Poisson
6. Analyse de sensibilité

```bash
python3 exemple_avance.py
```

### `exemple_selection_noeuds.py`
5 exemples de sélection de nœuds:
1. Charge concentrée
2. Appuis multiples
3. Région circulaire
4. Région rectangulaire
5. Sélection personnalisée

```bash
python3 exemple_selection_noeuds.py
```

## Comment utiliser les exemples

1. **Exécuter** pour voir le résultat
2. **Modifier** les paramètres (dimensions, matériaux, discrétisation)
3. **Adapter** pour votre cas d'application

## Structure d'un exemple

```python
from elasticity import ElasticityFEM2D, ElasticityVisualizer, Mesh2D

# 1. Créer le maillage
mesh = Mesh2D(width=10, height=5, nx=31, ny=16)

# 2. Sélectionner les nœuds (optionnel)
selected = mesh.select_nodes_on_line('top', 4, 6)

# 3. Créer le problème
problem = ElasticityFEM2D(10, 5, 31, 16, E=200e9, nu=0.3)

# 4. Définir les conditions aux limites
bc = {
    'bottom': [(1, 0.0)],
    'left': [(0, 0.0)],
    'nodes': [(n, 1, 0.02) for n in selected]
}

# 5. Résoudre
problem.solve(bc)
problem.compute_stress()

# 6. Visualiser
ElasticityVisualizer.plot_von_mises(problem)
```

## Paramètres courants

### Matériaux
- Acier: E=200e9, nu=0.3
- Aluminium: E=70e9, nu=0.33
- Titane: E=100e9, nu=0.34

### Discrétisation
- Basse: 21×11 (200 éléments)
- Moyenne: 31×16 (450 éléments)
- Fine: 61×31 (1800 éléments)

### Conditions aux limites
- (0, value): Bloquer u (déplacement horizontal)
- (1, value): Bloquer v (déplacement vertical)

## 4. `exemple_poutre_appuyee.py` ⭐ NOUVEAU

**Cas classique de RDM: Poutre appuyée sur deux tiers extérieurs avec charge répartie**

### Configuration mécanique
- Poutre horizontale de **12 m** sur **1 m** de haut
- **Appuis** (v=0) sur les portions externes: [0, 4m] et [8m, 12m]
- **Charge répartie** (vers le bas) entre les appuis: [4m, 8m]
- Bloquage horizontal à gauche (u=0)

### Résultats
```
Flèche maximale: -1.037 × 10⁻² m (au centre)
Contrainte σyy max: 4.003 GPa (traction)
Contrainte σyy min: -4.003 GPa (compression)
Von Mises max: 3.484 GPa
```

### Visualisations générées
1. Configuration mécanique (appuis et zones de charge)
2. Déplacements u, v et contraintes σxx, σyy
3. Contrainte von Mises
4. Géométrie déformée
5. Profil de flèche le long de la poutre

### Usage
```bash
python3 exemple_poutre_appuyee.py
```

### Points d'intérêt
- Montre comment créer une **sélection partielle sur les bords**
- Démontre l'utilisation de **sélection multiple** (appuis gauche + droit)
- Analyse classique **de poutre** (RDM)
- Visualisation du **profil de déformation** le long de la poutre

---
