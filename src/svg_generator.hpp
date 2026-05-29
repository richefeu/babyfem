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

        // Échelle de dessin
        int svg_width = 800;
        int svg_height = 400;
        double scale_x = svg_width / fem.width;
        double scale_y = svg_height / fem.height;

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

                // Coordonnées du rectangle
                double x1 = i * (fem.width / (fem.nx - 1)) * scale_x;
                double y1 = (fem.height - (j + 1) * (fem.height / (fem.ny - 1))) * scale_y;
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
            double x = i * (fem.width / (fem.nx - 1)) * scale_x;
            file << "<line x1=\"" << x << "\" y1=\"0\" x2=\"" << x << "\" y2=\""
                 << svg_height << "\" stroke=\"lightgray\" stroke-width=\"0.5\"/>\n";
        }
        for (int j = 0; j < fem.ny; j += (fem.ny / 10 + 1)) {
            double y = (fem.height - j * (fem.height / (fem.ny - 1))) * scale_y;
            file << "<line x1=\"0\" y1=\"" << y << "\" x2=\"" << svg_width << "\" y2=\""
                 << y << "\" stroke=\"lightgray\" stroke-width=\"0.5\"/>\n";
        }

        // Colorbar
        write_colorbar(file, svg_width + 50, 20, 20, svg_height - 40, vm_min, vm_max);

        file << "</svg>\n";
        file.close();

        std::cout << "Written: " << filename << "\n";
    }

public:
    static void write_deformed(const ElasticityFEM2D& fem, const std::string& filename,
                               double scale = 100.0) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + filename);
        }

        int svg_width = 800;
        int svg_height = 400;
        double scale_x = svg_width / fem.width;
        double scale_y = svg_height / fem.height;

        file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        file << "<svg width=\"" << (svg_width + 100) << "\" height=\"" << svg_height << "\" ";
        file << "xmlns=\"http://www.w3.org/2000/svg\">\n";
        file << "<rect width=\"" << (svg_width + 100) << "\" height=\"" << svg_height
             << "\" fill=\"white\"/>\n";

        // Géométrie inerte (gris clair)
        file << "<g stroke=\"lightgray\" fill=\"none\" stroke-width=\"1\">\n";
        for (int j = 0; j < fem.ny - 1; ++j) {
            for (int i = 0; i < fem.nx - 1; ++i) {
                double x1 = i * (fem.width / (fem.nx - 1)) * scale_x;
                double y1 = (fem.height - (j + 1) * (fem.height / (fem.ny - 1))) * scale_y;
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
                    double x = (xi[k] * (fem.width / (fem.nx - 1)) +
                               fem.u[fem.node_idx((int)xi[k], (int)yi[k])] * scale) * scale_x;
                    double y = (fem.height - (yi[k] * (fem.height / (fem.ny - 1)) +
                               fem.v[fem.node_idx((int)xi[k], (int)yi[k])] * scale)) * scale_y;
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
};
