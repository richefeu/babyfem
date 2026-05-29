#!/usr/bin/env python3
"""
Test d'intégration: vérifie que tous les modules fonctionnent ensemble.
"""

import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import numpy as np
from elasticity import ElasticityFEM2D, ElasticityVisualizer, Mesh2D, explain_mesh

def test_solver():
    """Test du solveur."""
    print("Test 1: Solveur FEM...")
    
    problem = ElasticityFEM2D(10.0, 5.0, 21, 11, 200e9, 0.3)
    bc = {
        'bottom': [(1, 0.0)],
        'top': [(1, 0.01)],
        'left': [(0, 0.0)],
        'right': None
    }
    
    problem.solve(bc)
    problem.compute_stress()
    
    assert problem.u is not None, "Déplacement u non calculé"
    assert problem.v is not None, "Déplacement v non calculé"
    assert problem.stress_xx is not None, "Contrainte σxx non calculée"
    assert problem.stress_yy is not None, "Contrainte σyy non calculée"
    assert problem.stress_xy is not None, "Contrainte σxy non calculée"
    
    assert not np.isnan(problem.u).any(), "NaN dans u"
    assert not np.isnan(problem.v).any(), "NaN dans v"
    assert not np.isnan(problem.stress_xx).any(), "NaN dans σxx"
    
    print("  ✓ Solveur OK")
    return problem


def test_von_mises(problem):
    """Test contrainte von Mises."""
    print("Test 2: Contrainte von Mises...")
    
    vm = problem.von_mises()
    assert vm is not None, "von Mises retourne None"
    assert vm.shape == problem.u.shape, "Forme von Mises incorrecte"
    assert not np.isnan(vm).any(), "NaN dans von Mises"
    assert np.max(vm) > 0, "von Mises vide"
    
    print(f"  ✓ von Mises OK (max={np.max(vm)/1e9:.2f} GPa)")
    return vm


def test_visualizer(problem):
    """Test visualiseur (sans afficher)."""
    print("Test 3: Visualiseur...")
    
    import matplotlib
    matplotlib.use('Agg')  # Backend non-graphique
    import matplotlib.pyplot as plt
    
    # Test chaque fonction
    try:
        fig1 = ElasticityVisualizer.plot_displacement_and_stress(problem, "Test")
        assert fig1 is not None
        plt.close(fig1)
        print("  ✓ plot_displacement_and_stress OK")
    except Exception as e:
        print(f"  ✗ plot_displacement_and_stress FAIL: {e}")
        return False
    
    try:
        fig2 = ElasticityVisualizer.plot_deformed_geometry(problem, scale=100)
        assert fig2 is not None
        plt.close(fig2)
        print("  ✓ plot_deformed_geometry OK")
    except Exception as e:
        print(f"  ✗ plot_deformed_geometry FAIL: {e}")
        return False
    
    try:
        fig3 = ElasticityVisualizer.plot_von_mises(problem)
        assert fig3 is not None
        plt.close(fig3)
        print("  ✓ plot_von_mises OK")
    except Exception as e:
        print(f"  ✗ plot_von_mises FAIL: {e}")
        return False
    
    return True


def test_boundary_conditions():
    """Test différentes conditions aux limites."""
    print("Test 4: Conditions aux limites...")
    
    problem = ElasticityFEM2D(10.0, 5.0, 21, 11, 200e9, 0.3)
    
    # Cisaillement partiel
    indices = list(range(5, 16))
    bc = {
        'bottom': [(1, -0.005, indices)],
        'top': [(1, 0.005, indices)],
        'left': [(0, 0.0)],
        'right': None
    }
    
    problem.solve(bc)
    problem.compute_stress()
    
    assert not np.isnan(problem.u).any(), "NaN dans cisaillement partiel"
    print("  ✓ Cisaillement partiel OK")
    
    return problem


def test_get_results(problem):
    """Test export des résultats."""
    print("Test 5: Export résultats...")
    
    results = problem.get_results()
    
    assert 'x' in results, "x manquant"
    assert 'y' in results, "y manquant"
    assert 'u' in results, "u manquant"
    assert 'v' in results, "v manquant"
    assert 'stress_xx' in results, "stress_xx manquant"
    assert 'E' in results, "E manquant"
    assert 'nu' in results, "nu manquant"
    
    assert results['E'] == problem.E
    assert results['nu'] == problem.nu
    
    print("  ✓ Export résultats OK")
    return results


def main():
    """Exécute tous les tests."""
    print("=" * 60)
    print("TEST D'INTÉGRATION - SOLVEUR D'ÉLASTICITÉ 2D")
    print("=" * 60)
    print()
    
    try:
        problem1 = test_solver()
        vm = test_von_mises(problem1)
        vis_ok = test_visualizer(problem1)
        problem2 = test_boundary_conditions()
        results = test_get_results(problem2)
        
        print()
        print("=" * 60)
        if vis_ok:
            print("✓ TOUS LES TESTS RÉUSSIS")
            print("=" * 60)
            return 0
        else:
            print("⚠ Tests partiellement réussis (visualisation)")
            print("=" * 60)
            return 1
            
    except Exception as e:
        print()
        print("=" * 60)
        print(f"✗ TEST ÉCHOUÉ: {e}")
        print("=" * 60)
        import traceback
        traceback.print_exc()
        return 1


if __name__ == "__main__":
    exit_code = main()
    sys.exit(exit_code)
