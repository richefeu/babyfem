#pragma once
#include "fem_2d.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>

class SVGGenerator {
public:
    static void write_von_mises(const ElasticityFEM2D& fem, const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + filename);
        }

        Matrix vm = fem.von_mises();

        // Trouve min/max des valeurs non-zéro SEULEMENT
        double vm_min = 1e308, vm_max = -1e308;
        for (int j = 0; j < fem.ny; ++j) {
            for (int i = 0; i < fem.nx; ++i) {
                double val = vm(j, i);
                if (val > 1e6) {  // > 1 MPa seulement
                    vm_min = std::min(vm_min, val);
                    vm_max = std::max(vm_max, val);
                }
            }
        }

        // Fallback si tous les zéros
        if (vm_min > vm_max) {
            vm_min = 1e6;  // 1 MPa
            vm_max = 1e10; // 10 GPa
        }

        // Échelle de dessin avec rapport d'aspect préservé
        double margin_ratio = 0.05;
        double target_size = 600.0;
        double max_dim = std::max(fem.width, fem.height);
        double scale = target_size / (max_dim * (1.0 + 2.0 * margin_ratio));
        double margin_x = margin_ratio * fem.width * scale;
        double margin_y = margin_ratio * fem.height * scale;

        int svg_width = (int)(fem.width * scale + 2.0 * margin_x + 100);  // +100 pour colorbar
        int svg_height = (int)(fem.height * scale + 2.0 * margin_y);
        double scale_x = scale;
        double scale_y = scale;

        // En-tête SVG
        file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        file << "<svg width=\"" << svg_width << "\" height=\"" << svg_height << "\" ";
        file << "xmlns=\"http://www.w3.org/2000/svg\">\n";
        file << "<rect width=\"" << svg_width << "\" height=\"" << svg_height
             << "\" fill=\"white\"/>\n";

        // Dessine les éléments colorés
        for (int j = 0; j < fem.ny - 1; ++j) {
            for (int i = 0; i < fem.nx - 1; ++i) {
                // Valeur moyenne sur l'élément
                double val = 0.25 * (vm(j, i) + vm(j+1, i) + vm(j+1, i+1) + vm(j, i+1));
                std::string color = value_to_color(val, vm_min, vm_max);

                // Coordonnées du rectangle (avec marges)
                double x1 = margin_x + i * (fem.width / (fem.nx - 1)) * scale_x;
                double y1 = margin_y + (fem.height - (j + 1) * (fem.height / (fem.ny - 1))) * scale_y;
                double w = (fem.width / (fem.nx - 1)) * scale_x;
                double h = (fem.height / (fem.ny - 1)) * scale_y;

                file << "<rect x=\"" << x1 << "\" y=\"" << y1 << "\" ";
                file << "width=\"" << w << "\" height=\"" << h << "\" ";
                file << "fill=\"" << color << "\" stroke=\"none\"/>\n";
            }
        }

        // Grille (optionnel)
        file << "<!-- Grid -->\n";
        for (int i = 0; i < fem.nx; i += (fem.nx / 10 + 1)) {
            double x = margin_x + i * (fem.width / (fem.nx - 1)) * scale_x;
            file << "<line x1=\"" << x << "\" y1=\"" << margin_y << "\" x2=\"" << x << "\" y2=\""
                 << (margin_y + fem.height * scale_y) << "\" stroke=\"lightgray\" stroke-width=\"0.5\"/>\n";
        }
        for (int j = 0; j < fem.ny; j += (fem.ny / 10 + 1)) {
            double y = margin_y + (fem.height - j * (fem.height / (fem.ny - 1))) * scale_y;
            file << "<line x1=\"" << margin_x << "\" y1=\"" << y << "\" x2=\""
                 << (margin_x + fem.width * scale_x) << "\" y2=\"" << y << "\" stroke=\"lightgray\" stroke-width=\"0.5\"/>\n";
        }

        // Colorbar (à droite du dessin)
        write_colorbar(file, (int)(margin_x + fem.width * scale_x + 20), (int)margin_y, 20, (int)(fem.height * scale_y), vm_min, vm_max);

        file << "</svg>\n";
        file.close();

        std::cout << "Written: " << filename << "\n";
    }

public:
    static void write_deformed(const ElasticityFEM2D& fem, const std::string& filename,
                               double deform_scale = 100.0) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + filename);
        }

        // Échelle de dessin avec rapport d'aspect préservé
        double margin_ratio = 0.05;
        double target_size = 600.0;
        double max_dim = std::max(fem.width, fem.height);
        double scale = target_size / (max_dim * (1.0 + 2.0 * margin_ratio));
        double margin_x = margin_ratio * fem.width * scale;
        double margin_y = margin_ratio * fem.height * scale;

        int svg_width = (int)(fem.width * scale + 2.0 * margin_x);
        int svg_height = (int)(fem.height * scale + 2.0 * margin_y);
        double scale_x = scale;
        double scale_y = scale;

        file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        file << "<svg width=\"" << svg_width << "\" height=\"" << svg_height << "\" ";
        file << "xmlns=\"http://www.w3.org/2000/svg\">\n";
        file << "<rect width=\"" << svg_width << "\" height=\"" << svg_height
             << "\" fill=\"white\"/>\n";

        // Géométrie inerte (gris clair)
        file << "<g stroke=\"lightgray\" fill=\"none\" stroke-width=\"1\">\n";
        for (int j = 0; j < fem.ny - 1; ++j) {
            for (int i = 0; i < fem.nx - 1; ++i) {
                double x1 = margin_x + i * (fem.width / (fem.nx - 1)) * scale_x;
                double y1 = margin_y + (fem.height - (j + 1) * (fem.height / (fem.ny - 1))) * scale_y;
                double w = (fem.width / (fem.nx - 1)) * scale_x;
                double h = (fem.height / (fem.ny - 1)) * scale_y;

                file << "<rect x=\"" << x1 << "\" y=\"" << y1 << "\" ";
                file << "width=\"" << w << "\" height=\"" << h << "\"/>\n";
            }
        }
        file << "</g>\n";

        // Géométrie déformée (bleu)
        file << "<g stroke=\"blue\" fill=\"none\" stroke-width=\"2\">\n";
        for (int j = 0; j < fem.ny - 1; ++j) {
            for (int i = 0; i < fem.nx - 1; ++i) {
                double xi[] = {(double)i, (double)(i+1), (double)(i+1), (double)i, (double)i};
                double yi[] = {(double)j, (double)j, (double)(j+1), (double)(j+1), (double)j};

                file << "<polyline points=\"";
                for (int k = 0; k < 5; ++k) {
                    double x = margin_x + (xi[k] * (fem.width / (fem.nx - 1)) +
                               fem.u[fem.node_idx((int)xi[k], (int)yi[k])] * deform_scale) * scale_x;
                    double y = margin_y + (fem.height - (yi[k] * (fem.height / (fem.ny - 1)) +
                               fem.v[fem.node_idx((int)xi[k], (int)yi[k])] * deform_scale)) * scale_y;
                    file << x << "," << y;
                    if (k < 4) file << " ";
                }
                file << "\"/>\n";
            }
        }
        file << "</g>\n";

        file << "</svg>\n";
        file.close();

        std::cout << "Written: " << filename << "\n";
    }

private:
    static std::string value_to_color(double val, double vmin, double vmax) {
        // Échelle logarithmique pour distributions biaisées
        double norm;

        // Ajouter petit offset pour éviter log(0)
        double offset = vmax * 1e-4;  // 0.01% du max
        double log_val = std::log10(val + offset);
        double log_vmin = std::log10(vmin + offset);
        double log_vmax = std::log10(vmax + offset);

        norm = (log_val - log_vmin) / (log_vmax - log_vmin);
        norm = std::max(0.0, std::min(1.0, norm));

        // Jet colormap: blue -> cyan -> green -> yellow -> red
        int r, g, b;
        if (norm < 0.25) {
            r = 0;
            g = 0;
            b = 255 * (0.25 + norm) / 0.25;
        } else if (norm < 0.5) {
            r = 0;
            g = 255 * (norm - 0.25) / 0.25;
            b = 255;
        } else if (norm < 0.75) {
            r = 255 * (norm - 0.5) / 0.25;
            g = 255;
            b = 255 * (1.0 - (norm - 0.5) / 0.25);
        } else {
            r = 255;
            g = 255 * (1.0 - (norm - 0.75) / 0.25);
            b = 0;
        }

        char hex[8];
        snprintf(hex, sizeof(hex), "#%02x%02x%02x", r, g, b);
        return std::string(hex);
    }

    static void write_colorbar(std::ofstream& file, int x, int y, int w, int h,
                               double vmin, double vmax) {
        file << "<!-- Colorbar -->\n";
        int steps = 20;
        for (int i = 0; i < steps; ++i) {
            double val = vmin + (i / (double)steps) * (vmax - vmin);
            std::string color = value_to_color(val, vmin, vmax);
            double y_pos = y + i * (h / (double)steps);

            file << "<rect x=\"" << x << "\" y=\"" << y_pos << "\" ";
            file << "width=\"" << w << "\" height=\"" << (h / steps + 1) << "\" ";
            file << "fill=\"" << color << "\" stroke=\"none\"/>\n";
        }

        file << "<text x=\"" << (x + w + 5) << "\" y=\"" << (y + 10) << "\" ";
        file << "font-size=\"10\" fill=\"black\">";
        file << std::fixed << std::setprecision(2) << (vmax / 1e9);
        file << " GPa</text>\n";

        file << "<text x=\"" << (x + w + 5) << "\" y=\"" << (y + h) << "\" ";
        file << "font-size=\"10\" fill=\"black\">";
        file << std::fixed << std::setprecision(2) << (vmin / 1e9);
        file << " GPa</text>\n";
    }

public:
    static void write_mesh_with_bcs(const ElasticityFEM2D& fem, const std::string& filename,
                                    const std::vector<int>& support_nodes,
                                    const std::vector<int>& load_nodes) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + filename);
        }

        // Facteur d'échelle unique pour respecter le rapport d'aspect
        double margin_ratio = 0.1;  // 10% marge (proportionnelle à chaque dimension)
        double target_size = 600.0;  // taille cible en pixels pour la plus grande dimension
        double max_dim = std::max(fem.width, fem.height);
        double scale = target_size / (max_dim * (1.0 + 2.0 * margin_ratio));

        // Marges proportionnelles à chaque dimension pour conserver le rapport d'aspect
        double margin_x = margin_ratio * fem.width * scale;
        double margin_y = margin_ratio * fem.height * scale;

        int svg_width = (int)(fem.width * scale + 2.0 * margin_x);
        int svg_height = (int)(fem.height * scale + 2.0 * margin_y);

        // En-tête SVG
        file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        file << "<svg width=\"" << svg_width << "\" height=\"" << svg_height << "\" ";
        file << "xmlns=\"http://www.w3.org/2000/svg\">\n";
        file << "<rect width=\"" << svg_width << "\" height=\"" << svg_height
             << "\" fill=\"white\"/>\n";

        // Fonction lambda pour convertir coordonnées physiques en SVG
        auto phys_to_svg_x = [&](double x) { return margin_x + x * scale; };
        auto phys_to_svg_y = [&](double y) { return margin_y + (fem.height - y) * scale; };

        // Fond pour la région du maillage
        file << "<rect x=\"" << margin_x << "\" y=\"" << margin_y << "\" ";
        file << "width=\"" << (fem.width * scale) << "\" height=\"" << (fem.height * scale) << "\" ";
        file << "fill=\"#f9f9f9\" stroke=\"#ccc\" stroke-width=\"1\"/>\n";

        // Dessiner le maillage (fin, gris clair)
        file << "<!-- Mesh elements -->\n";
        file << "<g stroke=\"#ddd\" stroke-width=\"0.5\" fill=\"none\">\n";
        for (int j = 0; j < fem.ny - 1; ++j) {
            for (int i = 0; i < fem.nx - 1; ++i) {
                double x1 = phys_to_svg_x(i * (fem.width / (fem.nx - 1)));
                double y1 = phys_to_svg_y((j + 1) * (fem.height / (fem.ny - 1)));
                double w = (fem.width / (fem.nx - 1)) * scale;
                double h = (fem.height / (fem.ny - 1)) * scale;

                file << "<rect x=\"" << x1 << "\" y=\"" << y1 << "\" ";
                file << "width=\"" << w << "\" height=\"" << h << "\"/>\n";
            }
        }
        file << "</g>\n";

        // Dessiner les nœuds (petits points)
        file << "<!-- Mesh nodes -->\n";
        file << "<g fill=\"#999\" stroke=\"none\">\n";
        for (int j = 0; j < fem.ny; ++j) {
            for (int i = 0; i < fem.nx; ++i) {
                double x = phys_to_svg_x(i * (fem.width / (fem.nx - 1)));
                double y = phys_to_svg_y(j * (fem.height / (fem.ny - 1)));
                file << "<circle cx=\"" << x << "\" cy=\"" << y << "\" r=\"1\"/>\n";
            }
        }
        file << "</g>\n";

        // Dessiner les appuis (triangles vers le haut)
        file << "<!-- Supports (v=0) -->\n";
        file << "<g fill=\"#0066cc\" stroke=\"#003366\" stroke-width=\"1\">\n";
        for (int node_idx : support_nodes) {
            int i = node_idx % fem.nx;
            int j = node_idx / fem.nx;
            double x = phys_to_svg_x(i * (fem.width / (fem.nx - 1)));
            double y = phys_to_svg_y(j * (fem.height / (fem.ny - 1)));
            double size = 8.0;

            // Triangle pointant vers le haut
            file << "<polygon points=\"" << x << "," << (y - size) << " ";
            file << (x - size) << "," << (y + size) << " ";
            file << (x + size) << "," << (y + size) << "\"/>\n";
        }
        file << "</g>\n";

        // Dessiner les charges (flèches vers le bas)
        file << "<!-- Loads (applied displacement) -->\n";
        file << "<g stroke=\"#cc0000\" stroke-width=\"2\" fill=\"none\">\n";
        for (int node_idx : load_nodes) {
            int i = node_idx % fem.nx;
            int j = node_idx / fem.nx;
            double x = phys_to_svg_x(i * (fem.width / (fem.nx - 1)));
            double y = phys_to_svg_y(j * (fem.height / (fem.ny - 1)));
            double arrow_len = 15.0;
            double arrow_head = 4.0;

            // Flèche vers le bas
            file << "<line x1=\"" << x << "\" y1=\"" << (y - arrow_len) << "\" ";
            file << "x2=\"" << x << "\" y2=\"" << (y + arrow_len) << "\"/>\n";

            // Pointe de flèche
            file << "<polygon points=\"" << x << "," << (y + arrow_len) << " ";
            file << (x - arrow_head) << "," << (y + arrow_len - arrow_head) << " ";
            file << (x + arrow_head) << "," << (y + arrow_len - arrow_head) << "\" ";
            file << "fill=\"#cc0000\"/>\n";
        }
        file << "</g>\n";

        // Légende
        file << "<!-- Legend -->\n";
        file << "<g font-family=\"sans-serif\" font-size=\"11\" fill=\"#333\">\n";
        int legend_x = (int)(margin_x + 10);
        int legend_y = (int)(svg_height - margin_y + 15);

        file << "<circle cx=\"" << (legend_x + 10) << "\" cy=\"" << (legend_y + 3) << "\" r=\"3\" fill=\"#0066cc\"/>\n";
        file << "<text x=\"" << (legend_x + 25) << "\" y=\"" << legend_y << "\">Support (v=0)</text>\n";

        file << "<line x1=\"" << (legend_x + 10) << "\" y1=\"" << (legend_y + 15) << "\" ";
        file << "x2=\"" << (legend_x + 10) << "\" y2=\"" << (legend_y + 25) << "\" ";
        file << "stroke=\"#cc0000\" stroke-width=\"2\"/>\n";
        file << "<text x=\"" << (legend_x + 25) << "\" y=\"" << (legend_y + 22) << "\">Load (applied displacement)</text>\n";

        file << "</g>\n";

        // Axes et dimensions (optionnel)
        file << "<!-- Dimensions -->\n";
        file << "<g font-family=\"monospace\" font-size=\"10\" fill=\"#666\">\n";
        file << "<text x=\"" << (margin_x + fem.width * scale / 2 - 20) << "\" y=\""
             << (svg_height - margin_y + 35) << "\">";
        file << "L = " << std::fixed << std::setprecision(2) << fem.width << " m</text>\n";

        file << "<text x=\"" << (margin_x - 40) << "\" y=\"" << (margin_y + fem.height * scale / 2) << "\" "
             << "text-anchor=\"end\">";
        file << "h = " << std::fixed << std::setprecision(3) << fem.height << " m</text>\n";
        file << "</g>\n";

        file << "</svg>\n";
        file.close();

        std::cout << "Written: " << filename << "\n";
    }
};
