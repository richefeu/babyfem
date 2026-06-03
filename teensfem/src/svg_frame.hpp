#pragma once
// svg_frame.hpp — rendu SVG d'une structure à barres (teensFEM).
//
// Sait dessiner : la structure et ses appuis/charges, la déformée amplifiée, et
// les diagrammes d'efforts internes (moment fléchissant M, effort normal N,
// effort tranchant V) tracés perpendiculairement aux barres.

#include "frame.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <cmath>
#include <cstdio>
#include <algorithm>

class SVGFrame {
public:
    explicit SVGFrame(const Frame& fr) : fr_(fr) {}

    SVGFrame& width(double px)         { W_ = px; return *this; }
    SVGFrame& deform_scale(double s)   { dscale_disp_ = s; return *this; }
    SVGFrame& diagram_scale(double s)  { dscale_diag_ = s; return *this; }

    void render(const std::string& field, const std::string& file) {
        compute_bbox(field);
        std::ofstream f(file);
        if (!f) throw std::runtime_error("Impossible d'écrire : " + file);
        f << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
          << "<svg width=\"" << W_ << "\" height=\"" << H_
          << "\" xmlns=\"http://www.w3.org/2000/svg\">\n"
          << "<rect width=\"" << W_ << "\" height=\"" << H_ << "\" fill=\"white\"/>\n";

        if (field == "deformed") {
            draw_members(f, "#bbb", 1.0, false);
            draw_deformed(f);
        } else if (field == "moment" || field == "shear" || field == "normal") {
            draw_diagram(f, field);
            draw_members(f, "#111", 1.6, false);
        } else { // structure / loads : modèle + schéma de chargement complet
            draw_members(f, "#111", 1.6, false);
            draw_distributed_loads(f);
            draw_loads(f);
            draw_hinges(f);
        }
        draw_supports(f);
        draw_nodes(f);
        draw_title(f, field);
        f << "</svg>\n";
        std::cout << "Written: " << file << "\n";
    }

private:
    const Frame& fr_;
    double W_ = 800, H_ = 0;
    double dscale_disp_ = 0, dscale_diag_ = 0;  // 0 -> auto
    static constexpr double TITLE_H = 28;
    double X0_, X1_, Y0_, Y1_, s_, mpx_;
    double size_ = 1, diag_max_ = 0;

    double px(double x) const { return mpx_ + (x - X0_) * s_; }
    double py(double y) const { return TITLE_H + mpx_ + (Y1_ - y) * s_; }

    void node_xy(int n, double& x, double& y) const { x = fr_.nodes[n].x; y = fr_.nodes[n].y; }

    double max_disp() const {
        double m = 0;
        for (int n = 0; n < fr_.n_nodes(); ++n)
            m = std::max(m, std::hypot(fr_.d[Frame::dof_u(n)], fr_.d[Frame::dof_v(n)]));
        return m;
    }

    double field_value(int e, double x, const std::string& field) const {
        double N, V, M; fr_.internal_at(e, x, N, V, M);
        if (field == "moment") return M;
        if (field == "shear")  return V;
        return N;
    }

    double max_field(const std::string& field) const {
        double m = 0;
        for (size_t e = 0; e < fr_.members.size(); ++e) {
            double L, c, s; fr_.geom(fr_.members[e], L, c, s);
            for (int k = 0; k <= 24; ++k)
                m = std::max(m, std::abs(field_value((int)e, L * k / 24.0, field)));
        }
        return m;
    }

    void compute_bbox(const std::string& field) {
        double xmin = 1e30, xmax = -1e30, ymin = 1e30, ymax = -1e30;
        for (int n = 0; n < fr_.n_nodes(); ++n) {
            xmin = std::min(xmin, fr_.nodes[n].x); xmax = std::max(xmax, fr_.nodes[n].x);
            ymin = std::min(ymin, fr_.nodes[n].y); ymax = std::max(ymax, fr_.nodes[n].y);
        }
        size_ = std::max(xmax - xmin, ymax - ymin);
        if (size_ < 1e-9) size_ = 1;

        double pad = 0.12 * size_;   // marge pour appuis / charges
        if (field == "structure" || field == "loads") pad = 0.20 * size_;
        if (field == "deformed") {
            if (dscale_disp_ <= 0) {
                double dm = max_disp();
                dscale_disp_ = (dm > 0) ? 0.12 * size_ / dm : 1.0;
            }
            pad = std::max(pad, max_disp() * dscale_disp_ + 0.05 * size_);
        } else if (field == "moment" || field == "shear" || field == "normal") {
            diag_max_ = max_field(field);
            if (dscale_diag_ <= 0)
                dscale_diag_ = (diag_max_ > 0) ? 0.22 * size_ / diag_max_ : 1.0;
            pad = std::max(pad, diag_max_ * dscale_diag_ + 0.05 * size_);
        }
        X0_ = xmin - pad; X1_ = xmax + pad; Y0_ = ymin - pad; Y1_ = ymax + pad;

        double bw = X1_ - X0_, bh = Y1_ - Y0_;
        mpx_ = 0.04 * W_;
        s_ = (W_ - 2 * mpx_) / bw;
        H_ = bh * s_ + 2 * mpx_ + TITLE_H;
    }

    void draw_members(std::ofstream& f, const std::string& col, double w, bool) const {
        f << "<g stroke=\"" << col << "\" stroke-width=\"" << w << "\" fill=\"none\">\n";
        for (const auto& m : fr_.members) {
            double x1, y1, x2, y2;
            node_xy(m.n1, x1, y1); node_xy(m.n2, x2, y2);
            f << "<line x1=\"" << px(x1) << "\" y1=\"" << py(y1)
              << "\" x2=\"" << px(x2) << "\" y2=\"" << py(y2) << "\"/>\n";
        }
        f << "</g>\n";
    }

    void draw_nodes(std::ofstream& f) const {
        for (int n = 0; n < fr_.n_nodes(); ++n) {
            double x, y; node_xy(n, x, y);
            f << "<circle cx=\"" << px(x) << "\" cy=\"" << py(y)
              << "\" r=\"2.5\" fill=\"#111\"/>\n";
        }
    }

    void draw_deformed(std::ofstream& f) const {
        f << "<g stroke=\"#c0392b\" stroke-width=\"1.8\" fill=\"none\">\n";
        for (const auto& m : fr_.members) {
            double x1 = fr_.nodes[m.n1].x + dscale_disp_ * fr_.d[Frame::dof_u(m.n1)];
            double y1 = fr_.nodes[m.n1].y + dscale_disp_ * fr_.d[Frame::dof_v(m.n1)];
            double x2 = fr_.nodes[m.n2].x + dscale_disp_ * fr_.d[Frame::dof_u(m.n2)];
            double y2 = fr_.nodes[m.n2].y + dscale_disp_ * fr_.d[Frame::dof_v(m.n2)];
            f << "<line x1=\"" << px(x1) << "\" y1=\"" << py(y1)
              << "\" x2=\"" << px(x2) << "\" y2=\"" << py(y2) << "\"/>\n";
        }
        f << "</g>\n";
    }

    // Diagramme d'effort interne, tracé perpendiculairement à chaque barre.
    void draw_diagram(std::ofstream& f, const std::string& field) const {
        std::string pos = "#2980b9", neg = "#e67e22";   // bleu / orange
        for (size_t e = 0; e < fr_.members.size(); ++e) {
            const auto& m = fr_.members[e];
            double L, c, s; fr_.geom(m, L, c, s);
            double x1, y1; node_xy(m.n1, x1, y1);
            double nx = -s, ny = c;                       // normale gauche unitaire
            const int K = 24;
            std::vector<std::array<double, 3>> samp(K + 1); // x, value, side-point exists
            std::vector<double> ox(K + 1), oy(K + 1), bxv(K + 1), byv(K + 1);
            for (int k = 0; k <= K; ++k) {
                double xl = L * k / K;
                double val = field_value((int)e, xl, field);
                double bx = x1 + c * xl, by = y1 + s * xl;            // point sur la barre
                bxv[k] = bx; byv[k] = by;
                ox[k] = bx + nx * val * dscale_diag_;
                oy[k] = by + ny * val * dscale_diag_;
                samp[k] = {xl, val, 0};
            }
            // Polygone de remplissage (barre -> courbe décalée)
            std::ostringstream poly;
            poly << px(bxv[0]) << "," << py(byv[0]) << " ";
            for (int k = 0; k <= K; ++k) poly << px(ox[k]) << "," << py(oy[k]) << " ";
            poly << px(bxv[K]) << "," << py(byv[K]);
            double vmid = samp[K/2][1];
            std::string col = (vmid >= 0) ? pos : neg;
            f << "<polygon points=\"" << poly.str() << "\" fill=\"" << col
              << "\" fill-opacity=\"0.25\" stroke=\"" << col << "\" stroke-width=\"1\"/>\n";
        }
        // étiquette d'échelle
        std::string unit = (field == "moment") ? "N.m" : "N";
        f << "<text x=\"" << (W_ - 6) << "\" y=\"" << (H_ - 6)
          << "\" font-family=\"sans-serif\" font-size=\"10\" text-anchor=\"end\""
          << " fill=\"#555\">max |" << (field == "moment" ? "M" : field == "shear" ? "V" : "N")
          << "| = " << std::scientific << std::setprecision(3) << diag_max_ << " " << unit << "</text>\n";
    }

    std::map<int, std::set<int>> fixed_dofs() const {
        std::map<int, std::set<int>> fd;
        for (const auto& [n, comp, val] : fr_.supports) { (void)val; fd[n].insert(comp); }
        return fd;
    }

    void draw_supports(std::ofstream& f) const {
        double sz = 9;
        for (const auto& [n, comps] : fixed_dofs()) {
            double x, y; node_xy(n, x, y);
            double cx = px(x), cy = py(y);
            bool u = comps.count(0), v = comps.count(1), r = comps.count(2);
            if (u && v && r) {
                // encastrement : petit rectangle hachuré
                f << "<rect x=\"" << (cx - sz) << "\" y=\"" << (cy - sz) << "\" width=\"" << (2*sz)
                  << "\" height=\"" << (2*sz) << "\" fill=\"none\" stroke=\"#27ae60\" stroke-width=\"1.5\"/>\n";
            } else if (u && v) {
                // articulation (appui double) : triangle
                f << "<polygon points=\"" << cx << "," << cy << " " << (cx - sz) << "," << (cy + sz)
                  << " " << (cx + sz) << "," << (cy + sz) << "\" fill=\"none\" stroke=\"#27ae60\" stroke-width=\"1.5\"/>\n";
            } else {
                // appui simple : triangle + ligne (rouleau)
                f << "<polygon points=\"" << cx << "," << cy << " " << (cx - sz) << "," << (cy + sz)
                  << " " << (cx + sz) << "," << (cy + sz) << "\" fill=\"none\" stroke=\"#27ae60\" stroke-width=\"1.5\"/>\n";
                f << "<line x1=\"" << (cx - sz) << "\" y1=\"" << (cy + sz + 3) << "\" x2=\"" << (cx + sz)
                  << "\" y2=\"" << (cy + sz + 3) << "\" stroke=\"#27ae60\" stroke-width=\"1.5\"/>\n";
            }
        }
    }

    // Format compact d'une grandeur (k préfixe au-delà de 1000).
    static std::string fmt_si(double v, const std::string& unit) {
        char buf[48]; double a = std::abs(v);
        if (a >= 1e3) snprintf(buf, sizeof(buf), "%.3g k%s", v / 1e3, unit.c_str());
        else          snprintf(buf, sizeof(buf), "%.3g %s",  v,        unit.c_str());
        return buf;
    }

    // Pointe de flèche au point (hx,hy), orientée selon (dx,dy).
    void arrowhead(std::ofstream& f, double hx, double hy, double dx, double dy,
                   const std::string& col) const {
        double L = std::hypot(dx, dy); if (L < 1e-9) return; dx /= L; dy /= L;
        double a = 7, w = 4;
        double a1x = hx - dx*a - dy*w, a1y = hy - dy*a + dx*w;
        double a2x = hx - dx*a + dy*w, a2y = hy - dy*a - dx*w;
        f << "<polygon points=\"" << hx << "," << hy << " " << a1x << "," << a1y
          << " " << a2x << "," << a2y << "\" fill=\"" << col << "\"/>\n";
    }

    // Charges nodales : forces (flèches droites) et moments (flèches courbes).
    void draw_loads(std::ofstream& f) const {
        const std::string col = "#8e44ad";
        double La = 32;
        for (const auto& [n, comp, val] : fr_.nodal_loads) {
            double x, y; node_xy(n, x, y);
            double cx = px(x), cy = py(y);
            if (comp == 2) {
                draw_moment_arrow(f, cx, cy, val > 0, col);
                f << "<text x=\"" << (cx + 15) << "\" y=\"" << (cy - 12)
                  << "\" font-family=\"sans-serif\" font-size=\"9\" fill=\"" << col
                  << "\">" << fmt_si(val, "N.m") << "</text>\n";
                continue;
            }
            double dx = (comp == 0) ? (val > 0 ? 1 : -1) : 0;
            double dy = (comp == 1) ? (val > 0 ? -1 : 1) : 0;   // y écran inversé
            f << "<line x1=\"" << (cx - dx*La) << "\" y1=\"" << (cy - dy*La) << "\" x2=\"" << cx
              << "\" y2=\"" << cy << "\" stroke=\"" << col << "\" stroke-width=\"2\"/>\n";
            arrowhead(f, cx, cy, dx, dy, col);
            f << "<text x=\"" << (cx - dx*La) << "\" y=\"" << (cy - dy*La - 4)
              << "\" font-family=\"sans-serif\" font-size=\"9\" text-anchor=\"middle\""
              << " fill=\"" << col << "\">" << fmt_si(std::abs(val), "N") << "</text>\n";
        }
    }

    // Flèche courbe figurant un moment (sens trigo si ccw).
    void draw_moment_arrow(std::ofstream& f, double cx, double cy, bool ccw,
                           const std::string& col) const {
        double R = 12; int N = 22;
        double start = ccw ? 2.3 : -2.3, end = ccw ? -2.3 : 2.3;  // y écran inversé
        std::ostringstream pts; double lx = 0, ly = 0, prx = 0, pry = 0;
        for (int k = 0; k <= N; ++k) {
            double t = k / (double)N, ang = start + (end - start) * t;
            double xx = cx + R * std::cos(ang), yy = cy + R * std::sin(ang);
            pts << xx << "," << yy << " ";
            prx = lx; pry = ly; lx = xx; ly = yy;
        }
        f << "<polyline fill=\"none\" stroke=\"" << col << "\" stroke-width=\"2\" points=\""
          << pts.str() << "\"/>\n";
        arrowhead(f, lx, ly, lx - prx, ly - pry, col);
    }

    // Charges réparties : flèches le long de la barre, dans la direction (wx,wy).
    void draw_distributed_loads(std::ofstream& f) const {
        const std::string col = "#1f7a8c";
        for (const auto& m : fr_.members) {
            double w = std::hypot(m.wx, m.wy);
            if (w < 1e-12) continue;
            double x1, y1, x2, y2; node_xy(m.n1, x1, y1); node_xy(m.n2, x2, y2);
            double p1x = px(x1), p1y = py(y1), p2x = px(x2), p2y = py(y2);
            double Lpx = std::hypot(p2x - p1x, p2y - p1y);
            double dx = m.wx / w, dy = -m.wy / w;            // direction écran (y inversé)
            double len = 18;
            int n = std::max(2, (int)std::round(Lpx / 26));
            std::ostringstream tails;
            for (int k = 0; k <= n; ++k) {
                double t = k / (double)n;
                double bx = p1x + (p2x - p1x) * t, by = p1y + (p2y - p1y) * t;
                double tx = bx - dx * len, ty = by - dy * len;
                tails << tx << "," << ty << " ";
                f << "<line x1=\"" << tx << "\" y1=\"" << ty << "\" x2=\"" << bx
                  << "\" y2=\"" << by << "\" stroke=\"" << col << "\" stroke-width=\"1\"/>\n";
                arrowhead(f, bx, by, dx, dy, col);
            }
            f << "<polyline fill=\"none\" stroke=\"" << col << "\" stroke-width=\"1.2\" points=\""
              << tails.str() << "\"/>\n";
            double mx = 0.5*(p1x+p2x) - dx*(len+9), my = 0.5*(p1y+p2y) - dy*(len+9);
            f << "<text x=\"" << mx << "\" y=\"" << my << "\" font-family=\"sans-serif\""
              << " font-size=\"9\" text-anchor=\"middle\" fill=\"" << col << "\">"
              << fmt_si(w, "N/m") << "</text>\n";
        }
    }

    // Rotules (extrémités articulées) : petit cercle ouvert décalé sur la barre.
    void draw_hinges(std::ofstream& f) const {
        for (const auto& m : fr_.members) {
            double x1, y1, x2, y2; node_xy(m.n1, x1, y1); node_xy(m.n2, x2, y2);
            double p1x = px(x1), p1y = py(y1), p2x = px(x2), p2y = py(y2);
            double L = std::hypot(p2x - p1x, p2y - p1y); if (L < 1e-9) continue;
            double ux = (p2x - p1x) / L, uy = (p2y - p1y) / L, off = 9;
            if (!m.rigid1) hinge_circle(f, p1x + ux*off, p1y + uy*off);
            if (!m.rigid2) hinge_circle(f, p2x - ux*off, p2y - uy*off);
        }
    }
    void hinge_circle(std::ofstream& f, double cx, double cy) const {
        f << "<circle cx=\"" << cx << "\" cy=\"" << cy
          << "\" r=\"3.4\" fill=\"white\" stroke=\"#111\" stroke-width=\"1.2\"/>\n";
    }

    void draw_title(std::ofstream& f, const std::string& field) const {
        std::map<std::string, std::string> names = {
            {"structure", "Modele et chargement"}, {"loads", "Modele et chargement"},
            {"deformed", "Deformee"},
            {"moment", "Moment flechissant M"}, {"shear", "Effort tranchant V"},
            {"normal", "Effort normal N"}};
        std::string t = names.count(field) ? names[field] : field;
        f << "<text x=\"" << (W_ / 2) << "\" y=\"18\" font-family=\"sans-serif\""
          << " font-size=\"14\" font-weight=\"bold\" text-anchor=\"middle\" fill=\"#222\">"
          << t << "</text>\n";
    }
};
