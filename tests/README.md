# Tests du projet

## `test_integration.py`

Tests d'intégration complets vérifiant:
1. Import de tous les modules
2. Création du maillage
3. Sélections de nœuds
4. Résolution avec conditions aux limites
5. Calcul des contraintes
6. Visualisation

```bash
python3 test_integration.py
```

Résultat attendu:
```
✓ Importation des modules... OK
✓ Création du maillage... OK
✓ Tests de sélection... OK
✓ Résolution avec sélection de nœuds... OK
✓ Visualisation... OK
✓ Explication du maillage... OK

✅ TOUS LES TESTS RÉUSSIS
```

## Exécuter les tests

```bash
# Tests d'intégration
cd ../tests
python3 test_integration.py

# Exemples (incluent des vérifications)
cd ../examples
python3 exemple_simple.py
python3 exemple_avance.py
python3 exemple_selection_noeuds.py
```

## Couverture des tests

Les tests vérifient:
- ✓ Modules et imports
- ✓ Création du maillage
- ✓ Sélections de nœuds (6 méthodes)
- ✓ Conditions aux limites
- ✓ Résolution FEM
- ✓ Calcul des contraintes
- ✓ Visualisation
- ✓ Absence de NaN/Inf
