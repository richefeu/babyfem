#pragma once
#include "fem_2d.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>

enum class ColorMap { VIRIDIS, COOLWARM };

// ============================================================
// SVGVisualization  —  fluent builder API
// ============================================================
//
//   SVGVisualization(problem)
//     .width(800)
//     .margins(60, 40)
//     .von_mises()           // or .stress_xx() / .stress_yy() / .stress_xy()
//     .mesh_overlay()        // optional: draw element edges on top
//     .write("out.svg");
//
// Colormaps: viridis (default for von Mises), coolwarm (default for
// signed stress components). Override with .colormap(ColorMap::VIRIDIS).
// Fix the color scale with .clamp(vmin, vmax).
// ============================================================

class SVGVisualization {
public:
    explicit SVGVisualization(const ElasticityFEM2D& fem) : fem_(fem) {}

    // --- Layout ----------------------------------------------------------
    SVGVisualization& width(double px)     { target_w_ = px; return *this; }

    SVGVisualization& margins(double x_px, double y_px) {
        mx_px_ = x_px; my_px_ = y_px; use_ratio_ = false; return *this;
    }
    SVGVisualization& margin(double ratio) {
        margin_ratio_ = ratio; use_ratio_ = true; return *this;
    }

    // --- Color scale -----------------------------------------------------
    SVGVisualization& colormap(ColorMap cm) { cm_ = cm; cm_set_ = true; return *this; }
    SVGVisualization& clamp(double lo, double hi) {
        clamp_lo_ = lo; clamp_hi_ = hi; use_clamp_ = true; return *this;
    }

    // --- Content ---------------------------------------------------------
    SVGVisualization& von_mises()    { field_ = Field::VM;  return *this; }
    SVGVisualization& stress_xx()    { field_ = Field::SXX; return *this; }
    SVGVisualization& stress_yy()    { field_ = Field::SYY; return *this; }
    SVGVisualization& stress_xy()    { field_ = Field::SXY; return *this; }
    SVGVisualization& mesh()         { show_mesh_    = true; return *this; }
    SVGVisualization& mesh_overlay() { show_overlay_ = true; return *this; }
    SVGVisualization& deformed()     { show_deformed_ = true; return *this; }
    SVGVisualization& deform_scale(double s) { deform_scale_ = s; return *this; }
    SVGVisualization& boundary_conditions(const std::vector<int>& sup,
                                          const std::vector<int>& loads) {
        sup_ = sup; loads_ = loads; show_bcs_ = true; return *this;
    }

    // --- Write -----------------------------------------------------------
    void write(const std::string& filename) {
        std::ofstream f(filename);
        if (!f) throw std::runtime_error("Cannot open: " + filename);

        bool has_field = (field_ != Field::NONE);
        compute_layout(has_field);

        f << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
          << "<svg width=\"" << W_ << "\" height=\"" << H_
          << "\" xmlns=\"http://www.w3.org/2000/svg\">\n"
          << "<rect width=\"" << W_ << "\" height=\"" << H_ << "\" fill=\"white\"/>\n";

        if (has_field) {
            draw_field(f);
        } else {
            // Background rect for mesh/deformed views
            f << "<rect x=\"" << mx_ << "\" y=\"" << (TITLE_H + my_)
              << "\" width=\""  << (fem_.width  * sx_)
              << "\" height=\"" << (fem_.height * sy_)
              << "\" fill=\"#f8f8f8\" stroke=\"#ccc\" stroke-width=\"0.5\"/>\n";
        }

        if (show_deformed_) draw_deformed(f);
        else if (show_mesh_) draw_mesh(f);

        if (show_bcs_) draw_bcs(f);

        write_title(f);
        write_dim_label(f);

        f << "</svg>\n";
        std::cout << "Written: " << filename << "\n";
    }

private:
    // -----------------------------------------------------------------
    enum class Field { NONE, VM, SXX, SYY, SXY };

    const ElasticityFEM2D& fem_;

    // Layout options
    double target_w_     = 600;
    double mx_px_        = 50,  my_px_       = 50;
    double margin_ratio_ = 0.05;
    bool   use_ratio_    = false;
    double deform_scale_ = 100;

    // Color options
    bool     cm_set_    = false;
    ColorMap cm_        = ColorMap::VIRIDIS;
    double   clamp_lo_  = 0, clamp_hi_ = 0;
    bool     use_clamp_ = false;

    // Content flags
    Field field_        = Field::NONE;
    bool  show_mesh_    = false;
    bool  show_overlay_ = false;
    bool  show_deformed_= false;
    bool  show_bcs_     = false;
    std::vector<int> sup_, loads_;

    // Computed layout (set by compute_layout)
    double mx_ = 0, my_ = 0, sx_ = 0, sy_ = 0;
    int    W_  = 0, H_  = 0;

    static constexpr int CB_W     = 18;  // colorbar width (px)
    static constexpr int CB_GAP   = 14;  // gap between plot and colorbar
    static constexpr int CB_TEXT  = 70;  // space for tick labels
    static constexpr int TITLE_H  = 30;  // space reserved above plot for title
    static constexpr int DIM_H    = 22;  // space below plot for dimension label

    // -----------------------------------------------------------------
    void compute_layout(bool colorbar) {
        sx_ = sy_ = target_w_ / std::max(fem_.width, fem_.height);

        if (use_ratio_) {
            mx_ = margin_ratio_ * fem_.width  * sx_;
            my_ = margin_ratio_ * fem_.height * sy_;
        } else {
            mx_ = mx_px_;
            my_ = my_px_;
        }

        int extra = colorbar ? (CB_GAP + CB_W + CB_GAP + CB_TEXT) : 0;
        W_ = (int)(2 * mx_ + fem_.width  * sx_ + extra);
        H_ = (int)(TITLE_H  + 2 * my_ + fem_.height * sy_ + DIM_H);
    }

    // Node pixel coordinates (i=col, j=row in FEM grid)
    double px(int i) const { return mx_ + i * fem_.dx * sx_; }
    double py(int j) const { return TITLE_H + my_ + (fem_.height - j * fem_.dy) * sy_; }

    // Element pixel dimensions
    double ew() const { return fem_.dx * sx_; }
    double eh() const { return fem_.dy * sy_; }

    // =================================================================
    // Colormap
    // =================================================================
    struct RGB { int r, g, b; };

    static RGB color_rgb(double t, ColorMap cm) {
        t = std::max(0.0, std::min(1.0, t));
        struct CP { double t; int r, g, b; };
        // Viridis: dark purple → blue → teal → green → yellow
        static const CP V[5] = {
            {0.00,  68,   1,  84}, {0.25,  59,  82, 139},
            {0.50,  33, 145, 140}, {0.75,  94, 201,  98}, {1.00, 253, 231,  37}};
        // Coolwarm: blue → white → red  (symmetric around 0)
        static const CP C[5] = {
            {0.00,  59,  76, 192}, {0.25, 144, 178, 254},
            {0.50, 220, 220, 220}, {0.75, 245, 156, 125}, {1.00, 180,   4,  38}};
        const CP* p = (cm == ColorMap::VIRIDIS) ? V : C;
        int s = 3;
        for (int i = 0; i < 4; ++i) if (t <= p[i+1].t) { s = i; break; }
        double u = (p[s+1].t > p[s].t) ? (t - p[s].t) / (p[s+1].t - p[s].t) : 0.0;
        return { (int)(p[s].r + u*(p[s+1].r - p[s].r)),
                 (int)(p[s].g + u*(p[s+1].g - p[s].g)),
                 (int)(p[s].b + u*(p[s+1].b - p[s].b)) };
    }

    static std::string to_hex(RGB c) {
        char buf[8];
        snprintf(buf, sizeof(buf), "#%02x%02x%02x", c.r, c.g, c.b);
        return buf;
    }

    static std::string val_hex(double v, double vmin, double vmax, ColorMap cm) {
        double t = (vmax > vmin) ? (v - vmin) / (vmax - vmin) : 0.5;
        return to_hex(color_rgb(t, cm));
    }

    // Auto-select SI unit prefix from the largest absolute value
    static std::pair<double, std::string> auto_unit(double absmax) {
        if (absmax >= 0.9e9) return {1e9, "GPa"};
        if (absmax >= 0.9e6) return {1e6, "MPa"};
        if (absmax >= 0.9e3) return {1e3, "kPa"};
        return {1.0, "Pa"};
    }

    // =================================================================
    // Title  and dimension label
    // =================================================================
    void write_title(std::ofstream& f) const {
        std::string text;
        if      (field_ == Field::VM)  text = "von Mises";
        else if (field_ == Field::SXX) text = "σxx";
        else if (field_ == Field::SYY) text = "σyy";
        else if (field_ == Field::SXY) text = "σxy";
        else if (show_deformed_)       text = "Déformée";
        else                           text = "Maillage";

        double cx = mx_ + fem_.width * sx_ * 0.5;
        f << "<text x=\"" << cx << "\" y=\"" << (TITLE_H - 8)
          << "\" font-family=\"sans-serif\" font-size=\"14\" font-weight=\"bold\""
          << " text-anchor=\"middle\" fill=\"#222\">" << text << "</text>\n";
    }

    void write_dim_label(std::ofstream& f) const {
        double cx = mx_ + fem_.width * sx_ * 0.5;
        double y  = TITLE_H + my_ + fem_.height * sy_ + 14;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(4)
            << "L = " << fem_.width << " m  ×  H = " << fem_.height << " m";
        f << "<text x=\"" << cx << "\" y=\"" << y
          << "\" font-family=\"sans-serif\" font-size=\"10\""
          << " text-anchor=\"middle\" fill=\"#999\">" << oss.str() << "</text>\n";
    }

    // =================================================================
    // Colorbar  (vertical, top = vmax, bottom = vmin)
    // =================================================================
    void write_colorbar(std::ofstream& f,
                        double vmin, double vmax,
                        ColorMap cm,
                        const std::string& label) const {
        double cb_x = mx_ + fem_.width * sx_ + CB_GAP;
        double cb_y = TITLE_H + my_;
        double cb_h = fem_.height * sy_;

        auto [us, un] = auto_unit(std::max(std::abs(vmin), std::abs(vmax)));

        // Continuous gradient: 200 thin rects
        constexpr int STEPS = 200;
        for (int k = 0; k < STEPS; ++k) {
            double t  = (double)k / STEPS;
            double v  = vmax - t * (vmax - vmin);   // top→bottom : vmax→vmin
            double ry = cb_y + t * cb_h;
            double rh = cb_h / STEPS + 1.0;         // +1 avoids hairline gaps
            f << "<rect x=\"" << cb_x << "\" y=\"" << ry
              << "\" width=\"" << CB_W << "\" height=\"" << rh
              << "\" fill=\"" << val_hex(v, vmin, vmax, cm) << "\" stroke=\"none\"/>\n";
        }

        // Thin border around the colorbar
        f << "<rect x=\"" << cb_x << "\" y=\"" << cb_y
          << "\" width=\"" << CB_W << "\" height=\"" << cb_h
          << "\" fill=\"none\" stroke=\"#777\" stroke-width=\"0.5\"/>\n";

        // Field label  (e.g. "σ_M")  and unit above the colorbar
        f << "<text x=\"" << cb_x << "\" y=\"" << (cb_y - 16)
          << "\" font-family=\"sans-serif\" font-size=\"12\" font-weight=\"bold\" fill=\"#222\">"
          << label << "</text>\n";
        f << "<text x=\"" << cb_x << "\" y=\"" << (cb_y - 4)
          << "\" font-family=\"sans-serif\" font-size=\"10\" fill=\"#555\">"
          << "[" << un << "]</text>\n";

        // 6 ticks  (k = 0 … 5,  top to bottom)
        for (int k = 0; k <= 5; ++k) {
            double t  = (double)k / 5;
            double v  = vmax - t * (vmax - vmin);
            double ty = cb_y + t * cb_h;

            // Tick mark on left edge of colorbar
            f << "<line x1=\"" << cb_x << "\" y1=\"" << ty
              << "\" x2=\"" << (cb_x - 4) << "\" y2=\"" << ty
              << "\" stroke=\"#444\" stroke-width=\"0.8\"/>\n";

            // Numeric label to the right of the colorbar
            std::ostringstream oss;
            double vs = v / us;
            oss << std::fixed << std::setprecision(std::abs(vs) < 100 ? 2 : 1) << vs;
            f << "<text x=\"" << (cb_x + CB_W + 5) << "\" y=\"" << (ty + 4)
              << "\" font-family=\"sans-serif\" font-size=\"10\" fill=\"#333\">"
              << oss.str() << "</text>\n";
        }
    }

    // =================================================================
    // Field map  (generic for all stress quantities)
    // =================================================================
    void draw_field(std::ofstream& f) {
        // Collect node values into a flat array
        std::vector<double> data(fem_.nx * fem_.ny);
        std::string label;
        ColorMap cm;

        switch (field_) {
            case Field::VM: {
                Matrix vm = fem_.von_mises();
                for (int j = 0; j < fem_.ny; ++j)
                    for (int i = 0; i < fem_.nx; ++i)
                        data[j*fem_.nx + i] = vm(j, i);
                label = "σ_M";   // σ_M
                cm    = cm_set_ ? cm_ : ColorMap::VIRIDIS;
                break;
            }
            case Field::SXX:
                for (int j = 0; j < fem_.ny; ++j)
                    for (int i = 0; i < fem_.nx; ++i)
                        data[j*fem_.nx + i] = fem_.stress_xx(j, i);
                label = "σ_xx";
                cm    = cm_set_ ? cm_ : ColorMap::COOLWARM;
                break;
            case Field::SYY:
                for (int j = 0; j < fem_.ny; ++j)
                    for (int i = 0; i < fem_.nx; ++i)
                        data[j*fem_.nx + i] = fem_.stress_yy(j, i);
                label = "σ_yy";
                cm    = cm_set_ ? cm_ : ColorMap::COOLWARM;
                break;
            case Field::SXY:
                for (int j = 0; j < fem_.ny; ++j)
                    for (int i = 0; i < fem_.nx; ++i)
                        data[j*fem_.nx + i] = fem_.stress_xy(j, i);
                label = "σ_xy";
                cm    = cm_set_ ? cm_ : ColorMap::COOLWARM;
                break;
            default: return;
        }

        // Color range
        double vmin, vmax;
        if (use_clamp_) {
            vmin = clamp_lo_; vmax = clamp_hi_;
        } else {
            vmin = *std::min_element(data.begin(), data.end());
            vmax = *std::max_element(data.begin(), data.end());
            if (vmin >= vmax) vmax = vmin + 1.0;
        }

        // Colored quads (average of 4 corner values per element)
        double pw = ew(), ph = eh();
        for (int j = 0; j < fem_.ny - 1; ++j) {
            for (int i = 0; i < fem_.nx - 1; ++i) {
                double v = 0.25 * (data[ j   *fem_.nx + i  ] + data[(j+1)*fem_.nx + i  ] +
                                   data[(j+1)*fem_.nx + i+1] + data[ j   *fem_.nx + i+1]);
                f << "<rect x=\"" << px(i) << "\" y=\"" << py(j+1)
                  << "\" width=\""  << pw << "\" height=\"" << ph
                  << "\" fill=\""   << val_hex(v, vmin, vmax, cm)
                  << "\" stroke=\"none\"/>\n";
            }
        }

        if (show_overlay_) draw_mesh_overlay(f);

        write_colorbar(f, vmin, vmax, cm, label);
    }

    // =================================================================
    // Mesh
    // =================================================================
    void draw_mesh_overlay(std::ofstream& f) const {
        f << "<g stroke=\"rgba(0,0,0,0.18)\" stroke-width=\"0.3\" fill=\"none\">\n";
        for (int j = 0; j < fem_.ny - 1; ++j)
            for (int i = 0; i < fem_.nx - 1; ++i)
                f << "<rect x=\"" << px(i) << "\" y=\"" << py(j+1)
                  << "\" width=\"" << ew() << "\" height=\"" << eh() << "\"/>\n";
        f << "</g>\n";
    }

    void draw_mesh(std::ofstream& f) const {
        f << "<g stroke=\"#aaa\" stroke-width=\"0.5\" fill=\"none\">\n";
        for (int j = 0; j < fem_.ny - 1; ++j)
            for (int i = 0; i < fem_.nx - 1; ++i)
                f << "<rect x=\"" << px(i) << "\" y=\"" << py(j+1)
                  << "\" width=\"" << ew() << "\" height=\"" << eh() << "\"/>\n";
        f << "</g>\n";
        f << "<g fill=\"#888\">\n";
        for (int j = 0; j < fem_.ny; ++j)
            for (int i = 0; i < fem_.nx; ++i)
                f << "<circle cx=\"" << px(i) << "\" cy=\"" << py(j) << "\" r=\"1\"/>\n";
        f << "</g>\n";
    }

    // =================================================================
    // Deformed shape
    // =================================================================
    void draw_deformed(std::ofstream& f) const {
        // Pixel coords of a deformed node
        auto xd = [&](int i, int j) {
            return mx_ + (i * fem_.dx + fem_.u[fem_.node_idx(i,j)] * deform_scale_) * sx_;
        };
        auto yd = [&](int i, int j) {
            return TITLE_H + my_
                 + (fem_.height - (j * fem_.dy + fem_.v[fem_.node_idx(i,j)] * deform_scale_)) * sy_;
        };

        // Undeformed reference: simple gray bounding box
        f << "<rect x=\"" << mx_ << "\" y=\"" << (TITLE_H + my_)
          << "\" width=\""  << (fem_.width  * sx_)
          << "\" height=\"" << (fem_.height * sy_)
          << "\" fill=\"none\" stroke=\"#ccc\" stroke-width=\"1\"/>\n";

        // Deformed outer contour (closed polygon)
        f << "<polygon fill=\"none\" stroke=\"#0055cc\" stroke-width=\"2\" points=\"";
        for (int i = 0;           i < fem_.nx;  ++i) f << xd(i, 0)          << "," << yd(i, 0)          << " ";
        for (int j = 1;           j < fem_.ny;  ++j) f << xd(fem_.nx-1, j)  << "," << yd(fem_.nx-1, j)  << " ";
        for (int i = fem_.nx - 2; i >= 0;       --i) f << xd(i, fem_.ny-1)  << "," << yd(i, fem_.ny-1)  << " ";
        for (int j = fem_.ny - 2; j > 0;        --j) f << xd(0, j)          << "," << yd(0, j)          << " ";
        f << "\"/>\n";

        // Internal fibers: a few horizontal lines + vertical cross-sections
        f << "<g stroke=\"#4488ee\" fill=\"none\" stroke-width=\"0.8\" stroke-dasharray=\"4,2\">\n";

        int sj = std::max(1, fem_.ny / 5);
        for (int j = sj; j < fem_.ny - 1; j += sj) {
            f << "<polyline points=\"";
            for (int i = 0; i < fem_.nx; ++i) f << xd(i, j) << "," << yd(i, j) << " ";
            f << "\"/>\n";
        }

        int si = std::max(1, fem_.nx / 10);
        for (int i = si; i < fem_.nx - 1; i += si) {
            f << "<polyline points=\"";
            for (int j = 0; j < fem_.ny; ++j) f << xd(i, j) << "," << yd(i, j) << " ";
            f << "\"/>\n";
        }
        f << "</g>\n";
    }

    // =================================================================
    // Boundary conditions  (supports = blue triangle, loads = red arrow)
    // =================================================================
    void draw_bcs(std::ofstream& f) const {
        f << "<g fill=\"#0055cc\" stroke=\"#003388\" stroke-width=\"0.8\">\n";
        for (int n : sup_) {
            double x = px(n % fem_.nx), y = py(n / fem_.nx);
            double s = 7.0;
            f << "<polygon points=\"" << x << "," << y
              << " " << (x - s) << "," << (y + s)
              << " " << (x + s) << "," << (y + s) << "\"/>\n";
        }
        f << "</g>\n";

        f << "<g stroke=\"#cc0000\" stroke-width=\"1.5\" fill=\"#cc0000\">\n";
        for (int n : loads_) {
            double x = px(n % fem_.nx), y = py(n / fem_.nx);
            constexpr double len = 14.0, head = 4.0;
            f << "<line x1=\""    << x        << "\" y1=\"" << (y - len)
              << "\" x2=\""       << x        << "\" y2=\"" << y << "\"/>\n";
            f << "<polygon points=\"" << x    << "," << y
              << " " << (x - head)    << "," << (y - head)
              << " " << (x + head)    << "," << (y - head) << "\"/>\n";
        }
        f << "</g>\n";
    }
};

// ============================================================
// SVGGenerator  —  legacy one-liner helpers
// ============================================================
class SVGGenerator {
public:
    static void write_von_mises(const ElasticityFEM2D& fem, const std::string& f) {
        SVGVisualization(fem).margins(50, 50).width(600).von_mises().write(f);
    }
    static void write_stress_xx(const ElasticityFEM2D& fem, const std::string& f) {
        SVGVisualization(fem).margins(50, 50).width(600).stress_xx().write(f);
    }
    static void write_stress_yy(const ElasticityFEM2D& fem, const std::string& f) {
        SVGVisualization(fem).margins(50, 50).width(600).stress_yy().write(f);
    }
    static void write_stress_xy(const ElasticityFEM2D& fem, const std::string& f) {
        SVGVisualization(fem).margins(50, 50).width(600).stress_xy().write(f);
    }
    static void write_mesh_with_bcs(const ElasticityFEM2D& fem, const std::string& f,
                                    const std::vector<int>& s, const std::vector<int>& l) {
        SVGVisualization(fem).margins(50, 50).width(600).mesh().boundary_conditions(s, l).write(f);
    }
    static void write_deformed(const ElasticityFEM2D& fem, const std::string& f,
                               double ds = 100.0) {
        SVGVisualization(fem).margins(50, 50).width(600).deform_scale(ds).mesh().deformed().write(f);
    }
};
