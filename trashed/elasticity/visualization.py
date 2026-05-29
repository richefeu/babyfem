#!/usr/bin/env python3
"""
Module de visualisation pour les résultats d'élasticité 2D.
"""

import numpy as np
import matplotlib.pyplot as plt


class ElasticityVisualizer:
    """Visualisation des résultats d'élasticité 2D."""

    @staticmethod
    def plot_displacement_and_stress(problem, title=""):
        """
        Visualise les déplacements et contraintes.

        Args:
            problem: objet ElasticityFEM2D avec résultats
            title: titre de la figure
        """
        if problem.u is None:
            print("Erreur: résoudre d'abord le problème")
            return None

        problem.compute_stress()

        fig, axes = plt.subplots(2, 2, figsize=(14, 10))
        fig.suptitle(f"Élasticité 2D - {title}", fontsize=14, fontweight='bold')

        X, Y = np.meshgrid(problem.x, problem.y)

        # Déplacement u
        im0 = axes[0, 0].contourf(X, Y, problem.u, levels=20, cmap='RdBu_r')
        axes[0, 0].set_title('Déplacement u (horizontal)')
        axes[0, 0].set_xlabel('x (m)')
        axes[0, 0].set_ylabel('y (m)')
        plt.colorbar(im0, ax=axes[0, 0], label='u (m)')

        # Déplacement v
        im1 = axes[0, 1].contourf(X, Y, problem.v, levels=20, cmap='RdBu_r')
        axes[0, 1].set_title('Déplacement v (vertical)')
        axes[0, 1].set_xlabel('x (m)')
        axes[0, 1].set_ylabel('y (m)')
        plt.colorbar(im1, ax=axes[0, 1], label='v (m)')

        # Contrainte σxx
        im2 = axes[1, 0].contourf(X, Y, problem.stress_xx, levels=20, cmap='RdYlBu_r')
        axes[1, 0].set_title('Contrainte σxx')
        axes[1, 0].set_xlabel('x (m)')
        axes[1, 0].set_ylabel('y (m)')
        plt.colorbar(im2, ax=axes[1, 0], label='σxx (Pa)')

        # Contrainte σyy
        im3 = axes[1, 1].contourf(X, Y, problem.stress_yy, levels=20, cmap='RdYlBu_r')
        axes[1, 1].set_title('Contrainte σyy')
        axes[1, 1].set_xlabel('x (m)')
        axes[1, 1].set_ylabel('y (m)')
        plt.colorbar(im3, ax=axes[1, 1], label='σyy (Pa)')

        plt.tight_layout()
        return fig

    @staticmethod
    def plot_deformed_geometry(problem, scale=1.0, title=""):
        """
        Visualise la géométrie déformée.

        Args:
            problem: objet ElasticityFEM2D avec résultats
            scale: facteur d'échelle pour les déplacements
            title: titre de la figure
        """
        if problem.u is None:
            return None

        fig, ax = plt.subplots(figsize=(10, 8))

        X, Y = np.meshgrid(problem.x, problem.y)

        # Géométrie initiale
        ax.plot(X[0, :], Y[0, :], 'b--', linewidth=1, alpha=0.5, label='Géométrie initiale')
        ax.plot(X[-1, :], Y[-1, :], 'b--', linewidth=1, alpha=0.5)
        ax.plot(X[:, 0], Y[:, 0], 'b--', linewidth=1, alpha=0.5)
        ax.plot(X[:, -1], Y[:, -1], 'b--', linewidth=1, alpha=0.5)

        # Géométrie déformée
        X_def = X + scale * problem.u
        Y_def = Y + scale * problem.v

        ax.plot(X_def[0, :], Y_def[0, :], 'r-', linewidth=2, label='Géométrie déformée')
        ax.plot(X_def[-1, :], Y_def[-1, :], 'r-', linewidth=2)
        ax.plot(X_def[:, 0], Y_def[:, 0], 'r-', linewidth=2)
        ax.plot(X_def[:, -1], Y_def[:, -1], 'r-', linewidth=2)

        # Champ von Mises
        von_mises = problem.von_mises()
        im = ax.contourf(X, Y, von_mises, levels=20, cmap='jet', alpha=0.7)

        ax.set_xlabel('x (m)')
        ax.set_ylabel('y (m)')
        ax.set_title(f'Géométrie déformée - {title}')
        ax.legend(loc='upper right')
        ax.set_aspect('equal')

        cbar = plt.colorbar(im, ax=ax)
        cbar.set_label('Contrainte von Mises (Pa)')

        return fig

    @staticmethod
    def plot_von_mises(problem, title=""):
        """
        Visualise uniquement la contrainte von Mises.

        Args:
            problem: objet ElasticityFEM2D avec résultats
            title: titre de la figure
        """
        if problem.u is None:
            return None

        fig, ax = plt.subplots(figsize=(10, 7))

        X, Y = np.meshgrid(problem.x, problem.y)
        von_mises = problem.von_mises()

        im = ax.contourf(X, Y, von_mises, levels=20, cmap='jet')
        ax.contour(X, Y, von_mises, levels=10, colors='black', alpha=0.3, linewidths=0.5)

        ax.set_xlabel('x (m)')
        ax.set_ylabel('y (m)')
        ax.set_title(f'Contrainte von Mises - {title}')
        ax.set_aspect('equal')

        cbar = plt.colorbar(im, ax=ax)
        cbar.set_label('von Mises (Pa)')

        return fig

    @staticmethod
    def plot_stress_component(problem, component='xx', title=""):
        """
        Visualise une composante de contrainte.

        Args:
            problem: objet ElasticityFEM2D avec résultats
            component: 'xx', 'yy' ou 'xy'
            title: titre de la figure
        """
        if problem.u is None:
            return None

        fig, ax = plt.subplots(figsize=(10, 7))

        X, Y = np.meshgrid(problem.x, problem.y)

        if component == 'xx':
            stress = problem.stress_xx
            label = 'σxx (Pa)'
        elif component == 'yy':
            stress = problem.stress_yy
            label = 'σyy (Pa)'
        elif component == 'xy':
            stress = problem.stress_xy
            label = 'σxy (Pa)'
        else:
            raise ValueError("component doit être 'xx', 'yy' ou 'xy'")

        im = ax.contourf(X, Y, stress, levels=20, cmap='RdYlBu_r')
        ax.contour(X, Y, stress, levels=10, colors='black', alpha=0.2, linewidths=0.5)

        ax.set_xlabel('x (m)')
        ax.set_ylabel('y (m)')
        ax.set_title(f'Contrainte σ{component} - {title}')
        ax.set_aspect('equal')

        cbar = plt.colorbar(im, ax=ax)
        cbar.set_label(label)

        return fig

    @staticmethod
    def plot_displacement_magnitude(problem, title=""):
        """
        Visualise la magnitude du déplacement.

        Args:
            problem: objet ElasticityFEM2D avec résultats
            title: titre de la figure
        """
        if problem.u is None:
            return None

        fig, ax = plt.subplots(figsize=(10, 7))

        X, Y = np.meshgrid(problem.x, problem.y)
        displacement = np.sqrt(problem.u**2 + problem.v**2)

        im = ax.contourf(X, Y, displacement, levels=20, cmap='viridis')
        ax.contour(X, Y, displacement, levels=10, colors='white', alpha=0.3, linewidths=0.5)

        ax.set_xlabel('x (m)')
        ax.set_ylabel('y (m)')
        ax.set_title(f'Magnitude du déplacement - {title}')
        ax.set_aspect('equal')

        cbar = plt.colorbar(im, ax=ax)
        cbar.set_label('|Déplacement| (m)')

        return fig

    @staticmethod
    def compare_results(problems_dict, metric='von_mises', title=""):
        """
        Compare plusieurs problèmes.

        Args:
            problems_dict: dict {nom: problem, ...}
            metric: 'von_mises', 'stress_xx', 'stress_yy', 'displacement'
            title: titre de la figure
        """
        n = len(problems_dict)
        fig, axes = plt.subplots(1, n, figsize=(5*n, 5))

        if n == 1:
            axes = [axes]

        for ax, (name, problem) in zip(axes, problems_dict.items()):
            X, Y = np.meshgrid(problem.x, problem.y)

            if metric == 'von_mises':
                data = problem.von_mises()
                label = 'von Mises (Pa)'
            elif metric == 'stress_xx':
                data = problem.stress_xx
                label = 'σxx (Pa)'
            elif metric == 'stress_yy':
                data = problem.stress_yy
                label = 'σyy (Pa)'
            elif metric == 'displacement':
                data = np.sqrt(problem.u**2 + problem.v**2)
                label = '|Déplacement| (m)'
            else:
                data = problem.von_mises()
                label = 'von Mises (Pa)'

            im = ax.contourf(X, Y, data, levels=20, cmap='jet')
            ax.set_title(name)
            ax.set_xlabel('x (m)')
            ax.set_ylabel('y (m)')
            ax.set_aspect('equal')
            plt.colorbar(im, ax=ax, label=label)

        fig.suptitle(title, fontsize=14, fontweight='bold')
        plt.tight_layout()
        return fig
