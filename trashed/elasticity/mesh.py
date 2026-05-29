#!/usr/bin/env python3
"""
Module de gestion du maillage pour élasticité 2D.
Explique comment le maillage est généré et fournit des outils de sélection.
"""

import numpy as np
import matplotlib.pyplot as plt


class Mesh2D:
    """Gestionnaire de maillage cartésien 2D structuré."""

    def __init__(self, width, height, nx, ny):
        """
        Crée un maillage cartésien structuré.

        Args:
            width: largeur du domaine (m)
            height: hauteur du domaine (m)
            nx: nombre de nœuds en x (nombre d'éléments = nx-1)
            ny: nombre de nœuds en y (nombre d'éléments = ny-1)
        """
        self.width = width
        self.height = height
        self.nx = nx
        self.ny = ny

        # Coordonnées des nœuds
        self.x = np.linspace(0, width, nx)
        self.y = np.linspace(0, height, ny)
        self.dx = width / (nx - 1)
        self.dy = height / (ny - 1)

        # Maillage complet
        self.X, self.Y = np.meshgrid(self.x, self.y)
        self.n_nodes = nx * ny
        self.n_elements = (nx - 1) * (ny - 1)

    def node_index(self, i, j):
        """Index global du nœud (i, j)."""
        return j * self.nx + i

    def get_node_coords(self, node_idx):
        """Retourne (i, j) pour un index de nœud."""
        j = node_idx // self.nx
        i = node_idx % self.nx
        return i, j

    def get_node_position(self, node_idx):
        """Retourne (x, y) pour un index de nœud."""
        i, j = self.get_node_coords(node_idx)
        return self.x[i], self.y[j]

    # ========================================================================
    # SÉLECTION DE NŒUDS
    # ========================================================================

    def select_nodes_at_boundary(self, boundary, tolerance=1e-6):
        """
        Sélectionne tous les nœuds sur une limite.

        Args:
            boundary: 'bottom', 'top', 'left', 'right'
            tolerance: tolérance numérique

        Returns:
            list des indices de nœuds
        """
        nodes = []

        if boundary == 'bottom':
            for i in range(self.nx):
                nodes.append(self.node_index(i, 0))

        elif boundary == 'top':
            for i in range(self.nx):
                nodes.append(self.node_index(i, self.ny - 1))

        elif boundary == 'left':
            for j in range(self.ny):
                nodes.append(self.node_index(0, j))

        elif boundary == 'right':
            for j in range(self.ny):
                nodes.append(self.node_index(self.nx - 1, j))

        else:
            raise ValueError(f"Limite inconnue: {boundary}")

        return nodes

    def select_nodes_in_region(self, x_min, x_max, y_min, y_max):
        """
        Sélectionne les nœuds dans une région rectangulaire.

        Args:
            x_min, x_max: limites en x
            y_min, y_max: limites en y

        Returns:
            list des indices de nœuds dans la région
        """
        nodes = []

        for j in range(self.ny):
            for i in range(self.nx):
                x, y = self.x[i], self.y[j]
                if x_min <= x <= x_max and y_min <= y <= y_max:
                    nodes.append(self.node_index(i, j))

        return nodes

    def select_nodes_on_line(self, boundary, x_min=None, x_max=None):
        """
        Sélectionne les nœuds sur une limite entre deux positions.

        Args:
            boundary: 'bottom', 'top', 'left', 'right'
            x_min, x_max: limites (en x pour top/bottom, en y pour left/right)

        Returns:
            list des indices de nœuds
        """
        nodes = []

        if boundary == 'bottom':
            y = 0
            x_min = x_min if x_min is not None else 0
            x_max = x_max if x_max is not None else self.width
            for i in range(self.nx):
                if x_min <= self.x[i] <= x_max:
                    nodes.append(self.node_index(i, 0))

        elif boundary == 'top':
            x_min = x_min if x_min is not None else 0
            x_max = x_max if x_max is not None else self.width
            for i in range(self.nx):
                if x_min <= self.x[i] <= x_max:
                    nodes.append(self.node_index(i, self.ny - 1))

        elif boundary == 'left':
            y_min = x_min if x_min is not None else 0  # x_min utilisé comme y_min
            y_max = x_max if x_max is not None else self.height
            for j in range(self.ny):
                if y_min <= self.y[j] <= y_max:
                    nodes.append(self.node_index(0, j))

        elif boundary == 'right':
            y_min = x_min if x_min is not None else 0
            y_max = x_max if x_max is not None else self.height
            for j in range(self.ny):
                if y_min <= self.y[j] <= y_max:
                    nodes.append(self.node_index(self.nx - 1, j))

        return nodes

    def select_nodes_in_circle(self, center_x, center_y, radius):
        """
        Sélectionne les nœuds dans un cercle.

        Args:
            center_x, center_y: centre du cercle
            radius: rayon du cercle

        Returns:
            list des indices de nœuds
        """
        nodes = []

        for j in range(self.ny):
            for i in range(self.nx):
                x, y = self.x[i], self.y[j]
                dist = np.sqrt((x - center_x)**2 + (y - center_y)**2)
                if dist <= radius:
                    nodes.append(self.node_index(i, j))

        return nodes

    def select_nodes_custom(self, predicate):
        """
        Sélectionne les nœuds selon un prédicat personnalisé.

        Args:
            predicate: fonction(i, j, x, y) -> bool

        Returns:
            list des indices de nœuds
        """
        nodes = []

        for j in range(self.ny):
            for i in range(self.nx):
                x, y = self.x[i], self.y[j]
                if predicate(i, j, x, y):
                    nodes.append(self.node_index(i, j))

        return nodes

    # ========================================================================
    # VISUALISATION
    # ========================================================================

    def plot_mesh(self, title=""):
        """Affiche le maillage."""
        fig, ax = plt.subplots(figsize=(10, 8))

        # Lignes verticales
        for i in range(self.nx):
            ax.plot(self.X[:, i], self.Y[:, i], 'k-', linewidth=0.5, alpha=0.3)

        # Lignes horizontales
        for j in range(self.ny):
            ax.plot(self.X[j, :], self.Y[j, :], 'k-', linewidth=0.5, alpha=0.3)

        # Nœuds
        ax.plot(self.X, self.Y, 'b.', markersize=3)

        ax.set_xlabel('x (m)')
        ax.set_ylabel('y (m)')
        ax.set_title(f'Maillage {self.nx}×{self.ny} ({self.n_elements} éléments) - {title}')
        ax.set_aspect('equal')
        ax.grid(True, alpha=0.2)

        return fig

    def plot_selected_nodes(self, node_indices, title=""):
        """Affiche le maillage avec les nœuds sélectionnés en surbrillance."""
        fig, ax = plt.subplots(figsize=(10, 8))

        # Tous les nœuds
        ax.plot(self.X, self.Y, 'k.', markersize=3, alpha=0.2)

        # Lignes du maillage
        for i in range(self.nx):
            ax.plot(self.X[:, i], self.Y[:, i], 'k-', linewidth=0.5, alpha=0.2)
        for j in range(self.ny):
            ax.plot(self.X[j, :], self.Y[j, :], 'k-', linewidth=0.5, alpha=0.2)

        # Nœuds sélectionnés
        if node_indices:
            selected_coords = [self.get_node_position(idx) for idx in node_indices]
            selected_x = [c[0] for c in selected_coords]
            selected_y = [c[1] for c in selected_coords]
            ax.plot(selected_x, selected_y, 'r.', markersize=8, label=f'{len(node_indices)} nœuds sélectionnés')

        ax.set_xlabel('x (m)')
        ax.set_ylabel('y (m)')
        ax.set_title(f'Sélection de nœuds - {title}')
        ax.set_aspect('equal')
        ax.legend()
        ax.grid(True, alpha=0.2)

        return fig

    # ========================================================================
    # INFORMATIONS
    # ========================================================================

    def print_info(self):
        """Affiche les informations du maillage."""
        print("=" * 60)
        print("INFORMATIONS MAILLAGE")
        print("=" * 60)
        print(f"Domaine: [{0}, {self.width}] × [{0}, {self.height}] m")
        print(f"Discrétisation: {self.nx} nœuds en x, {self.ny} nœuds en y")
        print(f"Espacement: dx={self.dx:.4f} m, dy={self.dy:.4f} m")
        print(f"Nombre de nœuds: {self.n_nodes}")
        print(f"Nombre d'éléments: {self.n_elements}")
        print(f"Nombre de DDLs (2D): {2 * self.n_nodes}")
        print("=" * 60)

    def get_statistics(self):
        """Retourne les statistiques du maillage."""
        return {
            'domain': (0, self.width, 0, self.height),
            'nodes': (self.nx, self.ny),
            'spacing': (self.dx, self.dy),
            'n_nodes': self.n_nodes,
            'n_elements': self.n_elements,
            'n_dof': 2 * self.n_nodes
        }


def explain_mesh():
    """Explication textuelle du maillage."""
    explanation = """
╔════════════════════════════════════════════════════════════════════════════╗
║                      EXPLICATION DU MAILLAGE                               ║
╚════════════════════════════════════════════════════════════════════════════╝

1. STRUCTURE DU MAILLAGE
═══════════════════════════════════════════════════════════════════════════

Un maillage cartésien 2D structuré est généré avec:
  • nx nœuds en direction x → (nx-1) éléments en x
  • ny nœuds en direction y → (ny-1) éléments en y
  • Total d'éléments = (nx-1) × (ny-1)

Exemple: 5×3 nœuds = 4×2 = 8 éléments

  ny=3
   ^
   |  •--•--•--•       Les points (•) sont les NŒUDS
   |  |  |  |  |       Les lignes forment les ÉLÉMENTS (carrés)
   |  •--•--•--•
   |  |  |  |  |
   |  •--•--•--•
   +--→ nx=5


2. ESPACEMENT DU MAILLAGE
═══════════════════════════════════════════════════════════════════════════

L'espacement est automatiquement calculé:
  • dx = width / (nx - 1)      [espacement en x]
  • dy = height / (ny - 1)     [espacement en y]

Exemple: width=10m, nx=31
  → dx = 10 / 30 = 0.333 m


3. NUMÉROTATION DES NŒUDS
═══════════════════════════════════════════════════════════════════════════

Les nœuds sont numérotés de bas en haut, de gauche à droite:
  node_index(i, j) = j * nx + i

Exemple pour nx=5, ny=3:

        j=2  10  11  12  13  14    (y = height)
        j=1   5   6   7   8   9
        j=0   0   1   2   3   4    (y = 0)
             i=0 i=1 i=2 i=3 i=4
                 x = 0      x = width


4. COORDONNÉES DES NŒUDS
═══════════════════════════════════════════════════════════════════════════

Les coordonnées sont:
  • x[i] = i * dx
  • y[j] = j * dy

Exemple: nœud (i=2, j=1) avec dx=0.333, dy=1.67
  → x = 0.667 m
  → y = 1.67 m


5. SÉLECTION DE NŒUDS
═══════════════════════════════════════════════════════════════════════════

Plusieurs méthodes pour sélectionner des nœuds:

  a) Toute une limite:
     mesh.select_nodes_at_boundary('bottom')
     mesh.select_nodes_at_boundary('top')
     mesh.select_nodes_at_boundary('left')
     mesh.select_nodes_at_boundary('right')

  b) Portion d'une limite (entre x_min et x_max):
     mesh.select_nodes_on_line('bottom', x_min=2, x_max=8)
     mesh.select_nodes_on_line('top', x_min=3, x_max=7)
     mesh.select_nodes_on_line('left', x_min=0, x_max=5)   # y_min, y_max

  c) Région rectangulaire:
     mesh.select_nodes_in_region(x_min, x_max, y_min, y_max)

  d) Région circulaire:
     mesh.select_nodes_in_circle(center_x, center_y, radius)

  e) Sélection personnalisée:
     mesh.select_nodes_custom(lambda i, j, x, y: x > 5 and y < 3)


6. EXEMPLE COMPLET
═══════════════════════════════════════════════════════════════════════════

from mesh import Mesh2D

# Créer le maillage
mesh = Mesh2D(width=10, height=5, nx=31, ny=16)

# Afficher les infos
mesh.print_info()

# Sélectionner des nœuds
nodes_bottom = mesh.select_nodes_at_boundary('bottom')           # Tous en bas
nodes_center = mesh.select_nodes_on_line('bottom', 3, 7)        # Centre bas
nodes_circle = mesh.select_nodes_in_circle(5, 2.5, 1)           # Cercle
nodes_custom = mesh.select_nodes_custom(
    lambda i, j, x, y: 2 < x < 8 and y < 2.5
)

# Visualiser
mesh.plot_mesh()
mesh.plot_selected_nodes(nodes_center, "Zone centrale")


7. ÉLÉMENTS ET DEGRÉS DE LIBERTÉ
═══════════════════════════════════════════════════════════════════════════

• Chaque nœud a 2 DDLs (déplacements u et v)
• Total DDLs = 2 × nombre de nœuds = 2 × nx × ny
• Les éléments sont des quadrilatères bilinéaires (Q1)
• Intégration par points de Gauss 2×2


8. RAPPORTS D'ASPECT
═══════════════════════════════════════════════════════════════════════════

Le rapport d'aspect des éléments est:
  aspect_ratio = dy / dx

Pour un maillage régulier:
  • Idéal: aspect_ratio ≈ 1 (carrés)
  • Acceptable: 0.5 < aspect_ratio < 2
  • Peut causer des problèmes: aspect_ratio > 10

Exemple:
  • width=10, height=5, nx=31, ny=16
  • dx = 10/30 = 0.333, dy = 5/15 = 0.333
  • aspect_ratio = 1 (parfait!)


9. CONVERGENCE
═══════════════════════════════════════════════════════════════════════════

Pour une meilleure précision:
  • Augmenter nx et ny de façon proportionnelle
  • Maintenir un aspect ratio proche de 1
  • Exemple: 31×16 → 61×31 → 121×61
  • Doubler la résolution quadruple le nombre d'éléments!

"""
    return explanation
