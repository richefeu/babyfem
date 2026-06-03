#pragma once
// svg_tri.hpp — rendu SVG d'un maillage triangulaire et de ses champs (kidfem).
//
// Rendu à plat (« flat shading ») : chaque triangle est colorié selon la valeur
// constante du champ sur l'élément (cohérent avec le CST). Sait aussi tracer le
// maillage, la déformée, et marquer les nœuds de conditions aux limites.
// La palette de couleurs vient du cœur partagé (core/colormap.hpp).

#include "fem_tri.hpp"
#include "colormap.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

class SVGTri {
public:
    explicit SVGTri(const ElasticityFEM_T3& fem) : fem_(fem), mesh_(fem.mesh) {}

    SVGTri& width(double px)       { target_w_ = px; return *this; }
    SVGTri& margin(double ratio)   { margin_ratio_ = ratio; return *this; }
    SVGTri& deform_scale(double s) { deform_scale_ = s; return *this; }

    SVGTri& field(const std::string& name) { field_ = name; return *this; }
    SVGTri& show_mesh()     { mesh_overlay_ = true; return *this; }
    SVGTri& show_deformed() { deformed_ = true; return *this; }
    SVGTri& boundary_conditions(const std::vector<int>& sup, const std::vector<int>& load) {
        sup_ = sup; load_ = load; show_bc_ = true; return *this;
    }

    void write(const std::string& filename) {
        mesh_.bbox(xmin_, xmax_, ymin_, ymax_);
        double w = xmax_ - xmin_, h = ymax_ - ymin_;
        if (w <= 0) w = 1; if (h <= 0) h = 1;
        m_ = margin_ratio_ * std::max(w, h);
        s_ = target_w_ / (w + 2 * m_);            // échelle uniforme (aspect conservé)
        W_ = target_w_;
        H_ = (h + 2 * m_) * s_ + TITLE_H_;

        std::ofstream f(filename);
        if (!f) throw std::runtime_error("Impossible d'écrire : " + filename);
        f << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
          << "<svg width=\"" << W_ << "\" height=\"" << H_
          << "\" xmlns=\"http://www.w3.org/2000/svg\">\n"
          << "<rect width=\"" << W_ << "\" height=\"" << H_ << "\" fill=\"white\"/>\n";

        if (!field_.empty()) draw_field(f);
        if (deformed_)       draw_deformed(f);
        else if (mesh_overlay_) draw_mesh(f);
        if (show_bc_)        draw_bc(f);
        draw_title(f);

        f << "</svg>\n";
        std::cout << "Written: " << filename << "\n";
    }

private:
    const ElasticityFEM_T3& fem_;
    const Mesh& mesh_;
    std::string field_;
    double target_w_ = 700, margin_ratio_ = 0.08, deform_scale_ = 100;
    bool mesh_overlay_ = false, deformed_ = false, show_bc_ = false;
    std::vector<int> sup_, load_;
    static constexpr double TITLE_H_ = 28;

    double xmin_, xmax_, ymin_, ymax_, m_, s_, W_, H_;

    double px(double x) const { return (x - xmin_ + m_) * s_; }
    double py(double y) const { return TITLE_H_ + (ymax_ - y + m_) * s_; }  // y vers le bas

    // Valeurs du champ par élément + bornes de l'échelle de couleur.
    bool element_values(std::vector<double>& vals, double& vmin, double& vmax,
                        ColorMap& cm) const {
        if      (field_ == "von_mises") { vals = fem_.e_vm;  cm = ColorMap::VIRIDIS; }
        else if (field_ == "stress_xx") { vals = fem_.e_sxx; cm = ColorMap::COOLWARM; }
        else if (field_ == "stress_yy") { vals = fem_.e_syy; cm = ColorMap::COOLWARM; }
        else if (field_ == "stress_xy") { vals = fem_.e_sxy; cm = ColorMap::COOLWARM; }
        else return false;
        vmin = *std::min_element(vals.begin(), vals.end());
        vmax = *std::max_element(vals.begin(), vals.end());
        if (cm == ColorMap::COOLWARM) {                 // symétrique autour de 0
            double a = std::max(std::abs(vmin), std::abs(vmax));
            vmin = -a; vmax = a;
        }
        return true;
    }

    void draw_field(std::ofstream& f) {
        std::vector<double> vals; double vmin, vmax; ColorMap cm;
        if (!element_values(vals, vmin, vmax, cm)) return;
        for (int e = 0; e < mesh_.n_tris(); ++e) {
            const auto& t = mesh_.tris[e];
            std::string col = colormap::hex(vals[e], vmin, vmax, cm);
            f << "<polygon points=\"";
            for (int k = 0; k < 3; ++k)
                f << px(mesh_.nodes[t[k]][0]) << "," << py(mesh_.nodes[t[k]][1]) << " ";
            f << "\" fill=\"" << col << "\" stroke=\"" << col << "\" stroke-width=\"0.3\"/>\n";
        }
        draw_colorbar(f, vmin, vmax, cm);
    }

    void draw_mesh(std::ofstream& f) const {
        f << "<g fill=\"none\" stroke=\"#444\" stroke-width=\"0.4\">\n";
        for (const auto& t : mesh_.tris) {
            f << "<polygon points=\"";
            for (int k = 0; k < 3; ++k)
                f << px(mesh_.nodes[t[k]][0]) << "," << py(mesh_.nodes[t[k]][1]) << " ";
            f << "\"/>\n";
        }
        f << "</g>\n";
    }

    void draw_deformed(std::ofstream& f) const {
        f << "<g fill=\"none\" stroke=\"#c0392b\" stroke-width=\"0.5\">\n";
        for (const auto& t : mesh_.tris) {
            f << "<polygon points=\"";
            for (int k = 0; k < 3; ++k) {
                int n = t[k];
                double x = mesh_.nodes[n][0] + deform_scale_ * fem_.u[n];
                double y = mesh_.nodes[n][1] + deform_scale_ * fem_.v[n];
                f << px(x) << "," << py(y) << " ";
            }
            f << "\"/>\n";
        }
        f << "</g>\n";
    }

    void draw_bc(std::ofstream& f) const {
        for (int n : sup_)
            f << "<circle cx=\"" << px(mesh_.nodes[n][0]) << "\" cy=\"" << py(mesh_.nodes[n][1])
              << "\" r=\"2.4\" fill=\"#2471a3\"/>\n";
        for (int n : load_)
            f << "<circle cx=\"" << px(mesh_.nodes[n][0]) << "\" cy=\"" << py(mesh_.nodes[n][1])
              << "\" r=\"2.4\" fill=\"#e67e22\"/>\n";
    }

    void draw_colorbar(std::ofstream& f, double vmin, double vmax, ColorMap cm) const {
        double bx = W_ - 18, by = TITLE_H_ + 10, bw = 8, bh = H_ - TITLE_H_ - 30;
        int N = 32;
        auto unit_pair = colormap::auto_unit(std::max(std::abs(vmin), std::abs(vmax)));
        double scale = unit_pair.first; const std::string& unit = unit_pair.second;
        for (int i = 0; i < N; ++i) {
            double t = 1.0 - (i + 0.5) / N;
            std::string col = colormap::to_hex(colormap::rgb(t, cm));
            f << "<rect x=\"" << bx << "\" y=\"" << (by + i * bh / N)
              << "\" width=\"" << bw << "\" height=\"" << (bh / N + 0.5)
              << "\" fill=\"" << col << "\"/>\n";
        }
        auto label = [&](double y, double v) {
            f << "<text x=\"" << (bx - 2) << "\" y=\"" << (y + 3)
              << "\" font-family=\"sans-serif\" font-size=\"9\" text-anchor=\"end\""
              << " fill=\"#333\">" << std::fixed << std::setprecision(2) << (v / scale) << "</text>\n";
        };
        label(by, vmax); label(by + bh, vmin);
        f << "<text x=\"" << bx << "\" y=\"" << (by - 3)
          << "\" font-family=\"sans-serif\" font-size=\"9\" text-anchor=\"middle\""
          << " fill=\"#333\">" << unit << "</text>\n";
    }

    void draw_title(std::ofstream& f) const {
        std::string text = field_.empty() ? (deformed_ ? "Deformee" : "Maillage") : field_;
        f << "<text x=\"" << (W_ / 2) << "\" y=\"18\""
          << " font-family=\"sans-serif\" font-size=\"14\" font-weight=\"bold\""
          << " text-anchor=\"middle\" fill=\"#222\">" << text << "</text>\n";
    }
};
