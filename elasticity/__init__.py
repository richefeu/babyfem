"""
Solveur d'élasticité 2D par méthode des éléments finis.

Modules disponibles:
- fem_solver: Solveur FEM pur
- visualization: Affichage des résultats
- mesh: Gestion du maillage et sélection de nœuds
"""

from .fem_solver import ElasticityFEM2D
from .visualization import ElasticityVisualizer
from .mesh import Mesh2D, explain_mesh

__all__ = ['ElasticityFEM2D', 'ElasticityVisualizer', 'Mesh2D', 'explain_mesh']
__version__ = '1.0'
