#!/usr/bin/env python3
"""
Solveur FEM pour élasticité 2D - Cœur du calcul uniquement.
Résout les équations d'équilibre en élasticité linéaire.
"""

import numpy as np
from scipy.sparse import lil_matrix
from scipy.sparse.linalg import spsolve
import warnings
warnings.filterwarnings('ignore')


class ElasticityFEM2D:
    """Solveur FEM pour élasticité 2D avec quadrilatères bilinéaires (Q1)."""

    def __init__(self, width, height, nx, ny, E, nu):
        """
        Initialise le problème.

        Args:
            width: largeur du rectangle (m)
            height: hauteur du rectangle (m)
            nx: nombre de nœuds en x
            ny: nombre de nœuds en y
            E: module de Young (Pa)
            nu: coefficient de Poisson
        """
        self.width = width
        self.height = height
        self.nx = nx
        self.ny = ny
        self.E = E
        self.nu = nu

        # Maillage
        self.x = np.linspace(0, width, nx)
        self.y = np.linspace(0, height, ny)
        self.dx = width / (nx - 1)
        self.dy = height / (ny - 1)

        # Paramètres matériau
        self.lam = E * nu / ((1 + nu) * (1 - 2*nu))
        self.mu = E / (2 * (1 + nu))

        self.n_nodes = nx * ny
        self.n_dof = 2 * self.n_nodes

        # Résultats
        self.u = None
        self.v = None
        self.stress_xx = None
        self.stress_yy = None
        self.stress_xy = None

    def node_idx(self, i, j):
        """Index global du nœud (i, j)."""
        return j * self.nx + i

    def _get_node_coords(self, node_idx):
        """Convertit index global en (i, j)."""
        j = node_idx // self.nx
        i = node_idx % self.nx
        return i, j

    def dof_u(self, i, j):
        """DOF pour déplacement u au nœud (i, j)."""
        return 2 * self.node_idx(i, j)

    def dof_v(self, i, j):
        """DOF pour déplacement v au nœud (i, j)."""
        return 2 * self.node_idx(i, j) + 1

    def get_node_position(self, node_idx):
        """Retourne (x, y) pour un index de nœud global."""
        i, j = self._get_node_coords(node_idx)
        return self.x[i], self.y[j]

    def local_to_global_dofs(self, elem_nodes):
        """Conversion des DDLs locaux aux DDLs globaux."""
        global_dofs = []
        for i in range(0, len(elem_nodes), 2):
            node_i, node_j = elem_nodes[i], elem_nodes[i+1]
            global_dofs.extend([self.dof_u(node_i, node_j), self.dof_v(node_i, node_j)])
        return np.array(global_dofs)

    def local_stiffness_q1(self):
        """Matrice de rigidité locale pour élément Q1 bilinéaire."""
        a, b = self.dx / 2, self.dy / 2
        lam, mu = self.lam, self.mu

        k_local = np.zeros((8, 8))
        gp = np.array([-1/np.sqrt(3), 1/np.sqrt(3)])

        for xi in gp:
            for eta in gp:
                dN_dxi = np.array([
                    [-(1-eta)/4, (1-eta)/4, (1+eta)/4, -(1+eta)/4],
                    [-(1-xi)/4, -(1+xi)/4, (1+xi)/4, (1-xi)/4]
                ])

                dx_dxi = dN_dxi[0, :] @ np.array([0, self.dx, self.dx, 0])
                dx_deta = dN_dxi[1, :] @ np.array([0, self.dx, self.dx, 0])
                dy_dxi = dN_dxi[0, :] @ np.array([0, 0, self.dy, self.dy])
                dy_deta = dN_dxi[1, :] @ np.array([0, 0, self.dy, self.dy])

                det_J = dx_dxi * dy_deta - dx_deta * dy_dxi

                dN_dx = (dy_deta * dN_dxi[0, :] - dy_dxi * dN_dxi[1, :]) / det_J
                dN_dy = (dx_dxi * dN_dxi[1, :] - dx_deta * dN_dxi[0, :]) / det_J

                w = 1.0

                D = np.array([
                    [lam + 2*mu, lam, 0],
                    [lam, lam + 2*mu, 0],
                    [0, 0, mu]
                ])

                B = np.zeros((3, 8))
                for i in range(4):
                    B[0, 2*i] = dN_dx[i]
                    B[1, 2*i+1] = dN_dy[i]
                    B[2, 2*i] = dN_dy[i]
                    B[2, 2*i+1] = dN_dx[i]

                k_local += B.T @ D @ B * det_J * w

        return k_local

    def assemble_global_matrix(self):
        """Assemble la matrice de rigidité globale."""
        K = lil_matrix((self.n_dof, self.n_dof))
        k_ref = self.local_stiffness_q1()

        for j in range(self.ny - 1):
            for i in range(self.nx - 1):
                nodes = [i, j, i+1, j, i+1, j+1, i, j+1]
                global_dofs = self.local_to_global_dofs(nodes)

                for i_loc in range(8):
                    for j_loc in range(8):
                        K[global_dofs[i_loc], global_dofs[j_loc]] += k_ref[i_loc, j_loc]

        return K.tocsr()

    def solve(self, boundary_conditions):
        """
        Résout le problème avec conditions aux limites spécifiées.

        Args:
            boundary_conditions: dict avec plusieurs formats possibles:

            Format 1 - Bords complets:
            {
                'bottom': [(component, value), ...],
                'top': [(component, value), ...],
                'left': [(component, value), ...],
                'right': [(component, value), ...]
            }

            Format 2 - Bords avec sélection:
            {
                'bottom': [(component, value, indices), ...],
                'top': [(component, value, indices), ...],
                ...
            }

            Format 3 - Sélection arbitraire (liste de nœuds):
            {
                'nodes': [
                    (node_index, component, value),
                    (node_index, component, value),
                    ...
                ]
            }

            Format 4 - Mélange de formats:
            {
                'bottom': [(1, 0.0)],
                'top': [(1, 0.02)],
                'nodes': [(5, 0, 0.001), (6, 0, 0.002)]
            }

            component: 0 (u) ou 1 (v)
            value: valeur du déplacement imposé
            indices: liste des nœuds (optionnel, tous par défaut)
            node_index: index global du nœud (0 à n_nodes-1)
        """
        K = self.assemble_global_matrix()
        F = np.zeros(self.n_dof, dtype=float)

        K = K.tolil()

        # Appliquer les conditions aux limites
        for boundary, bcs_list in boundary_conditions.items():
            if bcs_list is None or boundary == 'nodes':
                continue

            if boundary == 'bottom':
                j = 0
                idx_range = range(self.nx)
            elif boundary == 'top':
                j = self.ny - 1
                idx_range = range(self.nx)
            elif boundary == 'left':
                i = 0
                idx_range = range(self.ny)
            elif boundary == 'right':
                i = self.nx - 1
                idx_range = range(self.ny)
            else:
                continue

            for bc in bcs_list:
                component, value = bc[:2]
                indices = bc[2] if len(bc) > 2 else list(idx_range)

                if boundary in ['bottom', 'top']:
                    for idx in indices:
                        dof = self.dof_u(idx, j) if component == 0 else self.dof_v(idx, j)
                        K[dof, :] = 0
                        K[dof, dof] = 1.0
                        F[dof] = value
                else:
                    for idx in indices:
                        dof = self.dof_u(i, idx) if component == 0 else self.dof_v(i, idx)
                        K[dof, :] = 0
                        K[dof, dof] = 1.0
                        F[dof] = value

        # Appliquer les conditions aux limites sur sélection de nœuds
        if 'nodes' in boundary_conditions:
            for node_bc in boundary_conditions['nodes']:
                node_idx, component, value = node_bc
                # Convertir index global de nœud en DDL
                i, j = self._get_node_coords(node_idx)
                dof = self.dof_u(i, j) if component == 0 else self.dof_v(i, j)
                K[dof, :] = 0
                K[dof, dof] = 1.0
                F[dof] = value

        K = K.tocsr()
        disp = spsolve(K, F)

        if isinstance(disp, np.matrix):
            disp = np.asarray(disp).flatten()

        # Extraire u et v
        self.u = np.zeros((self.ny, self.nx))
        self.v = np.zeros((self.ny, self.nx))

        for j in range(self.ny):
            for i in range(self.nx):
                self.u[j, i] = disp[self.dof_u(i, j)]
                self.v[j, i] = disp[self.dof_v(i, j)]

    def compute_stress(self):
        """Calcule les contraintes à partir des déplacements."""
        self.stress_xx = np.zeros((self.ny, self.nx))
        self.stress_yy = np.zeros((self.ny, self.nx))
        self.stress_xy = np.zeros((self.ny, self.nx))

        if self.u is None:
            return

        for j in range(self.ny):
            for i in range(self.nx):
                i_left = max(0, i-1)
                i_right = min(self.nx-1, i+1)
                j_down = max(0, j-1)
                j_up = min(self.ny-1, j+1)

                if i_left == i:
                    du_dx = (self.u[j, i_right] - self.u[j, i]) / self.dx
                elif i_right == i:
                    du_dx = (self.u[j, i] - self.u[j, i_left]) / self.dx
                else:
                    du_dx = (self.u[j, i_right] - self.u[j, i_left]) / (2*self.dx)

                if j_down == j:
                    dv_dy = (self.v[j_up, i] - self.v[j, i]) / self.dy
                elif j_up == j:
                    dv_dy = (self.v[j, i] - self.v[j_down, i]) / self.dy
                else:
                    dv_dy = (self.v[j_up, i] - self.v[j_down, i]) / (2*self.dy)

                if j_down == j:
                    du_dy = (self.u[j_up, i] - self.u[j, i]) / self.dy
                elif j_up == j:
                    du_dy = (self.u[j, i] - self.u[j_down, i]) / self.dy
                else:
                    du_dy = (self.u[j_up, i] - self.u[j_down, i]) / (2*self.dy)

                if i_left == i:
                    dv_dx = (self.v[j, i_right] - self.v[j, i]) / self.dx
                elif i_right == i:
                    dv_dx = (self.v[j, i] - self.v[j, i_left]) / self.dx
                else:
                    dv_dx = (self.v[j, i_right] - self.v[j, i_left]) / (2*self.dx)

                eps_xx = du_dx
                eps_yy = dv_dy
                eps_xy = 0.5 * (du_dy + dv_dx)

                self.stress_xx[j, i] = self.lam*(eps_xx+eps_yy) + 2*self.mu*eps_xx
                self.stress_yy[j, i] = self.lam*(eps_xx+eps_yy) + 2*self.mu*eps_yy
                self.stress_xy[j, i] = 2*self.mu*eps_xy

    def get_results(self):
        """Retourne tous les résultats du calcul."""
        return {
            'x': self.x,
            'y': self.y,
            'u': self.u,
            'v': self.v,
            'stress_xx': self.stress_xx,
            'stress_yy': self.stress_yy,
            'stress_xy': self.stress_xy,
            'E': self.E,
            'nu': self.nu,
            'width': self.width,
            'height': self.height,
            'nx': self.nx,
            'ny': self.ny
        }

    def von_mises(self):
        """Calcule la contrainte von Mises."""
        if self.stress_xx is None:
            return None
        return np.sqrt(self.stress_xx**2 + self.stress_yy**2 -
                      self.stress_xx*self.stress_yy + 3*self.stress_xy**2)
