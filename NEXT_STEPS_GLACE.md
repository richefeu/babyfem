# Prochaines étapes avec test_glace

## 🎯 Objectif

Le cas **test_glace** est maintenant le **cas de référence** pour guider le développement du solveur C++17. C'est un problème réaliste et extensible.

## 📋 Roadmap de développement

### Phase 1: Validation et analyse (Immédiat)

**Objectifs:**
- Valider que les résultats C++17 correspondent au Python
- Comprendre en détail le comportement mécanique
- Identifier les points de concentration de contrainte

**Tâches:**
- [ ] Comparer résultats C++ vs Python (flèche, contrainte max)
- [ ] Analyser les profils de contrainte (coupes transversales)
- [ ] Vérifier la convergence (v_max vs maillage)
- [ ] Estimer l'erreur vs théorie RdM

**Sorties attendues:**
```
Flèche théorique (poutre simplement appuyée, charge répartie):
  f = 5*P*L^4 / (384*E*I)

Pour notre cas:
  P = E * A * depl / L ≈ 2e12 * 0.002 / 0.1 = 4e13 N
  I = h^3 / 12 ≈ (0.02)^3 / 12 = 6.67e-8 m^4
  f_théo = 5*P*L^4 / (384*E*I) ≈ 0.0001 m

FEM vs Théorie: < 5% de différence attendue
```

---

### Phase 2: Paramètres variables (Court terme, 1 semaine)

**Objectifs:**
- Implémenter des variantes du cas de base
- Étudier l'influence des paramètres
- Créer des cas tests paramétrés

**Tâches:**
- [ ] Variant 1: Variation du déplacement imposé (0.00005 → 0.0005 m)
- [ ] Variant 2: Variation de L/h (5 → 10, 20, 50)
- [ ] Variant 3: Variation du matériau (acier → alu → résine)
- [ ] Variant 4: Appuis ponctuels vs appuis répartis

**Code type:**
```cpp
// examples/test_glace_variant_1.cpp
// Variation du déplacement imposé

struct Case {
    double L, h;
    int nx, ny;
    double E, nu;
    double depl;  // Variable
};

Case cases[] = {
    {0.1, 0.02, 120, 12, 210e9, 0.3, 0.00005},  // Petit
    {0.1, 0.02, 120, 12, 210e9, 0.3, 0.0001},   // Nominal
    {0.1, 0.02, 120, 12, 210e9, 0.3, 0.0005},   // Grand
};

for (auto& cas : cases) {
    // Résoudre et afficher résultats
}
```

**Sorties:**
- Tableau résumé (dépl vs flèche, contrainte)
- Graphiques (mais: implémentation CSV output en premier)

---

### Phase 3: Raffinement du maillage (Moyen terme, 2 semaines)

**Objectifs:**
- Étudier la convergence numérique
- Identifier l'ordre de convergence
- Recommander des maillages

**Tâches:**
- [ ] Boucle sur nx, ny: (60,6), (120,12), (240,24), (480,48)
- [ ] Pour chaque maillage: calculer v_max et σ_von_mises_max
- [ ] Afficher le taux de convergence (pente log-log)
- [ ] Comparer vs théorie RdM

**Algorithme:**
```
pour chaque maillage:
    résoudre(maillage)
    extraire(v_max, σ_max)
    error = abs(v_max - v_théo) / v_théo
    afficher(nx, ny, v_max, error)

tracer(log(nx) vs log(error))  → pente ≈ ordre de convergence
```

**Résultats attendus:**
- Convergence ~O(h²) pour éléments Q1
- Meilleur maillage recommandé: 120×12 déjà bon

---

### Phase 4: Analyse de convergence (Moyen terme, 3 semaines)

**Objectifs:**
- Implémentation d'erreurs estimées
- Raffinement adaptatif (optionnel)
- Validation numérique complète

**Tâches:**
- [ ] Implémentation d'estimateur d'erreur
- [ ] Comparaison avec solution analytique (RdM)
- [ ] Indicateurs de qualité de maillage
- [ ] Recommandations pour production

---

### Phase 5: Matrices creuses (Long terme, 1 mois)

**Objectifs:**
- Supporter des maillages > 100k DDLs
- Optimiser la mémoire et le temps
- Comparer Gauss vs Gauss-Seidel

**Tâches:**
- [ ] Implémenter structure COO sparse
- [ ] Implémenter structure CSR
- [ ] Implémenter Gauss-Seidel itératif
- [ ] Benchmarker: dense vs sparse vs itératif

**Performance attendue:**
```
Dense (Gauss):           O(n³) ≈ O((2h)³)    [actuel]
Sparse (Gauss):          O(n²) ≈ O((2h)²)    [optimisé]
Itératif (GS):           O(k·n²)             [très optimisé]
```

---

### Phase 6: Élasticité non-linéaire (Très long terme, 2 mois)

**Objectifs:**
- Introduire non-linéarité matériau
- Plasticité simple
- Grands déplacements

**Tâches:**
- [ ] Critère de plasticité (von Mises)
- [ ] Écrouissage isotrope
- [ ] Algorithme Newton-Raphson
- [ ] Test_glace non-linéaire

---

## 🔬 Infrastructure de test

### 1. Résultats de référence Python

Avant tout développement, exécutez le Python:

```bash
python3 examples/test_glace.py
# Écrivez les résultats:
# - Déplacement max
# - Contrainte max
# - Profils transversaux
```

### 2. Comparaison C++ vs Python

Créez un script de validation:

```cpp
// utils/compare_with_python.cpp
struct ValidationResult {
    double u_error, v_error, sigma_error;
    bool is_valid;
};

ValidationResult compare(const std::string& python_file,
                        const std::string& cpp_file) {
    // Charger résultats Python
    // Charger résultats C++
    // Calculer erreurs
    // Retourner validation
}
```

### 3. Suite de tests paramétrés

```bash
test_glace_variant_depl.cpp   # Variation déplacement
test_glace_variant_ratio.cpp  # Variation L/h
test_glace_variant_mat.cpp    # Variation matériau
test_glace_refinement.cpp     # Étude convergence
```

---

## 📊 Cas test additionnels (bonus)

### Autres cas pédagogiques à explorer

1. **Cantilever (console):** Une extrémité fixée, charge en bout
   - Utile pour: Compression vs flexion
   - Cas Python: `exemple_poutre_appuyee.py`

2. **Compression pure:** Appuis points, déplacement latéral bloqué
   - Utile pour: Instabilité, flambement
   - Cas avancé

3. **Cisaillement pur:** Appuis de Dirichlet complexes
   - Utile pour: Shear-locking, test Q1
   - Déjà implémenté: `example_shear.cpp`

4. **Trou dans plaque:** Concentration de contrainte
   - Utile pour: Singularités, raffinement adaptatif
   - Cas complexe

---

## 🛠️ Outil d'export CSV

Pour faciliter l'analyse, créez un exporteur CSV:

```cpp
// src/csv_generator.hpp
class CSVGenerator {
    static void write_solution(const ElasticityFEM2D& fem,
                              const std::string& filename) {
        // Exporte: x, y, u, v, σxx, σyy, σxy, σvM
        // Format: une ligne par nœud
        // Utilisable dans Excel / Python / gnuplot
    }
    
    static void write_cross_section(const ElasticityFEM2D& fem,
                                   double x_pos,
                                   const std::string& filename) {
        // Coupe transversale à x_pos
        // Profil: y vs u, v, σxx, σyy, σxy, σvM
    }
};
```

---

## 📈 Métriques de suivi

Créez un **dashboard** pour chaque variant:

```
test_glace [baseline]
├─ Géométrie: 0.1 × 0.02 m, 120×12 nœuds
├─ Déplacement imposé: 0.0001 m
├─ Résultats:
│  ├─ v_max: -0.0001 m ✓
│  ├─ σ_von_mises_max: 15.9 GPa
│  ├─ Erreur vs RdM: -2.3 %
│  └─ Temps CPU: 85 ms
├─ Fichiers:
│  ├─ test_glace_von_mises.svg (124 KB)
│  ├─ test_glace_deformed.svg (209 KB)
│  └─ test_glace_results.csv (12 KB)
└─ Status: ✅ PASS

test_glace_large_depl
├─ Déplacement imposé: 0.0005 m
├─ Résultats:
│  ├─ v_max: -0.0005 m ✓
│  ├─ σ_von_mises_max: 79.5 GPa
│  ├─ Erreur vs RdM: -2.1 %
│  └─ Temps CPU: 85 ms
└─ Status: ✅ PASS

...
```

---

## 🎓 Pédagogie

Chaque phase apprend une nouvelle notion:

1. **Validation** → Confiance dans le code
2. **Paramètres** → Influence physique
3. **Convergence** → Mathématiques numérique
4. **Sparse** → Algorithmes efficaces
5. **Non-linéarité** → Mécanique avancée

---

## 📝 Planning suggéré

| Semaine | Tâche | Effort | Priorité |
|---------|-------|--------|----------|
| 1 | Validation & analyse | 2j | 🔴 Critique |
| 2 | Variantes simples | 3j | 🟠 Haute |
| 3 | Convergence maillage | 3j | 🟠 Haute |
| 4 | CSV output | 1j | 🟡 Moyenne |
| 5-6 | Matrices creuses | 5j | 🟡 Moyenne |
| 7+ | Cas additionnels | Variable | 🟢 Basse |

---

## ✅ Checklist avant de commencer

- [ ] Cas test_glace C++ exécute sans erreur
- [ ] Résultats C++ VS Python documentés
- [ ] GLACE_CASE.md lu et compris
- [ ] Référence théorique (RdM) comprise
- [ ] Plan Git clear (branches pour chaque variant)
- [ ] Outils de visualisation (gnuplot, Excel, etc.) prêts

---

## 🚀 Commencer Phase 1

```bash
# 1. Exécuter le Python pour référence
python3 examples/test_glace.py | tee results_python.txt

# 2. Exécuter le C++ pour validation
./build/example_test_glace | tee results_cpp.txt

# 3. Comparer manuellement
diff results_python.txt results_cpp.txt

# 4. Noter les écarts (doit être < 1%)

# 5. Analyser les fichiers SVG
open build/test_glace_von_mises.svg
open build/test_glace_deformed.svg

# 6. Lancer la Phase 2: variantes
```

---

**Prêt à progresser ? Cette infrastructure test_glace vous permet d'étendre le solveur en toute confiance !**
