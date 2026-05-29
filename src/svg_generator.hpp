#pragma once
#include "fem_2d.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <string>

// Modern SVG visualization API
class SVGVisualization {
private:
    const ElasticityFEM2D& fem_;
    double margin_ratio_;
    double target_size_;
    double deform_scale_;

    bool show_mesh_;
    bool show_von_mises_;
    bool show_deformed_;
    bool show_bcs_;

    std::vector<int> support_nodes_;
    std::vector<int> load_nodes_;

    // Computed values
    double scale_;
    double margin_x_;
    double margin_y_;
    int svg_width_;
    int svg_height_;
    double scale_x_;
    double scale_y_;

    void compute_scales() {
        double max_dim = std::max(fem_.width, fem_.height);
        scale_ = target_size_ / (max_dim * (1.0 + 2.0 * margin_ratio_));
        margin_x_ = margin_ratio_ * fem_.width * scale_;
        margin_y_ = margin_ratio_ * fem_.height * scale_;
        scale_x_ = scale_;
        scale_y_ = scale_;
    }

public:
    SVGVisualization(const ElasticityFEM2D& fem)
        : fem_(fem),
          margin_ratio_(0.05),
          target_size_(600.0),
          deform_scale_(100.0),
          show_mesh_(false),
          show_von_mises_(false),
          show_deformed_(false),
          show_bcs_(false),
          scale_(0), margin_x_(0), margin_y_(0),
          svg_width_(0), svg_height_(0), scale_x_(0), scale_y_(0) {
        compute_scales();
    }

    // Configuration methods (return *this for chaining)
    SVGVisualization& margin(double ratio) {
        margin_ratio_ = ratio;
        compute_scales();
        return *this;
    }

    SVGVisualization& width(double target_px) {
        target_size_ = target_px;
        compute_scales();
        return *this;
    }

    SVGVisualization& deform_scale(double scale) {
        deform_scale_ = scale;
        return *this;
    }

    // Element selection methods
    SVGVisualization& mesh() {
        show_mesh_ = true;
        return *this;
    }

    SVGVisualization& von_mises() {
        show_von_mises_ = true;
        return *this;
    }

    SVGVisualization& deformed() {
        show_deformed_ = true;
        return *this;
    }

    SVGVisualization& boundary_conditions(const std::vector<int>& supports,
                                          const std::vector<int>& loads) {
        support_nodes_ = supports;
        load_nodes_ = loads;
        show_bcs_ = true;
        return *this;
    }

    // Generate and write SVG
    void write(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + filename);
        }

        // Compute final dimensions based on what we're drawing
        int extra_width = show_von_mises_ ? 100 : 0;
        svg_width_ = (int)(fem_.width * scale_x_ + 2.0 * margin_x_ + extra_width);
        svg_height_ = (int)(fem_.height * scale_y_ + 2.0 * margin_y_);

        // SVG header
        file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        file << "<svg width=\"" << svg_width_ << "\" height=\"" << svg_height_ << "\" ";
        file << "xmlns=\"http://www.w3.org/2000/svg\">\n";
        file << "<rect width=\"" << svg_width_ << "\" height=\"" << svg_height_
             << "\" fill=\"white\"/>\n";

        // Draw each requested element
        if (show_von_mises_) {
            draw_von_mises_impl(file);
        } else if (show_mesh_ || show_deformed_ || show_bcs_) {
            // Background for mesh region
            file << "<rect x=\"" << margin_x_ << "\" y=\"" << margin_y_ << "\" ";
            file << "width=\"" << (fem_.width * scale_x_) << "\" height=\"" << (fem_.height * scale_y_) << "\" ";
            file << "fill=\"#f9f9f9\" stroke=\"#ccc\" stroke-width=\"1\"/>\n";
        }

        if (show_deformed_) {
            draw_deformed_impl(file);
        } else if (show_mesh_) {
            draw_mesh_impl(file);
        }

        if (show_bcs_) {
            draw_bcs_impl(file);
        }

        // Dimensions label
        file << "<!-- Dimensions -->\n";
        file << "<g font-family=\"monospace\" font-size=\"10\" fill=\"#666\">\n";
        file << "<text x=\"" << (margin_x_ + fem_.width * scale_x_ / 2 - 20) << "\" y=\""
             << (svg_height_ - margin_y_ + 35) << "\">";
        file << "L = " << std::fixed << std::setprecision(2) << fem_.width << " m</text>\n";
        file << "</g>\n";

        file << "</svg>\n";
        file.close();

        std::cout << "Written: " << filename << "\n";
    }

private:
    // Implementation methods for drawing each element
    void draw_mesh_impl(std::ofstream& file) {
        file << "<!-- Mesh elements -->\n";
        file << "<g stroke=\"#ddd\" stroke-width=\"0.5\" fill=\"none\">\n";
        for (int j = 0; j < fem_.ny - 1; ++j) {
            for (int i = 0; i < fem_.nx - 1; ++i) {
                double x1 = margin_x_ + i * (fem_.width / (fem_.nx - 1)) * scale_x_;
                double y1 = margin_y_ + (fem_.height - (j + 1) * (fem_.height / (fem_.ny - 1))) * scale_y_;
                double w = (fem_.width / (fem_.nx - 1)) * scale_x_;
                double h = (fem_.height / (fem_.ny - 1)) * scale_y_;

                file << "<rect x=\"" << x1 << "\" y=\"" << y1 << "\" ";
                file << "width=\"" << w << "\" height=\"" << h << "\"/>\n";
            }
        }
        file << "</g>\n";

        // Mesh nodes
        file << "<!-- Mesh nodes -->\n";
        file << "<g fill=\"#999\" stroke=\"none\">\n";
        for (int j = 0; j < fem_.ny; ++j) {
            for (int i = 0; i < fem_.nx; ++i) {
                double x = margin_x_ + i * (fem_.width / (fem_.nx - 1)) * scale_x_;
                double y = margin_y_ + (fem_.height - j * (fem_.height / (fem_.ny - 1))) * scale_y_;
                file << "<circle cx=\"" << x << "\" cy=\"" << y << "\" r=\"1\"/>\n";
            }
        }
        file << "</g>\n";
    }

    void draw_von_mises_impl(std::ofstream& file) {
        Matrix vm = fem_.von_mises();

        // Find min/max for non-zero values only
        double vm_min = 1e308, vm_max = -1e308;
        for (int j = 0; j < fem_.ny; ++j) {
            for (int i = 0; i < fem_.nx; ++i) {
                double val = vm(j, i);
                if (val > 1e6) {
                    vm_min = std::min(vm_min, val);
                    vm_max = std::max(vm_max, val);
                }
            }
        }

        if (vm_min > vm_max) {
            vm_min = 1e6;
            vm_max = 1e10;
        }

        // Draw colored elements
        for (int j = 0; j < fem_.ny - 1; ++j) {
            for (int i = 0; i < fem_.nx - 1; ++i) {
                double val = 0.25 * (vm(j, i) + vm(j+1, i) + vm(j+1, i+1) + vm(j, i+1));
                std::string color = value_to_color(val, vm_min, vm_max);

                double x1 = margin_x_ + i * (fem_.width / (fem_.nx - 1)) * scale_x_;
                double y1 = margin_y_ + (fem_.height - (j + 1) * (fem_.height / (fem_.ny - 1))) * scale_y_;
                double w = (fem_.width / (fem_.nx - 1)) * scale_x_;
                double h = (fem_.height / (fem_.ny - 1)) * scale_y_;

                file << "<rect x=\"" << x1 << "\" y=\"" << y1 << "\" ";
                file << "width=\"" << w << "\" height=\"" << h << "\" ";
                file << "fill=\"" << color << "\" stroke=\"none\"/>\n";
            }
        }

        // Colorbar
        write_colorbar(file, (int)(margin_x_ + fem_.width * scale_x_ + 20), (int)margin_y_, 20, (int)(fem_.height * scale_y_), vm_min, vm_max);
    }

    void draw_deformed_impl(std::ofstream& file) {
        // Undeformed geometry (light gray)
        file << "<g stroke=\"lightgray\" fill=\"none\" stroke-width=\"1\">\n";
        for (int j = 0; j < fem_.ny - 1; ++j) {
            for (int i = 0; i < fem_.nx - 1; ++i) {
                double x1 = margin_x_ + i * (fem_.width / (fem_.nx - 1)) * scale_x_;
                double y1 = margin_y_ + (fem_.height - (j + 1) * (fem_.height / (fem_.ny - 1))) * scale_y_;
                double w = (fem_.width / (fem_.nx - 1)) * scale_x_;
                double h = (fem_.height / (fem_.ny - 1)) * scale_y_;

                file << "<rect x=\"" << x1 << "\" y=\"" << y1 << "\" ";
                file << "width=\"" << w << "\" height=\"" << h << "\"/>\n";
            }
        }
        file << "</g>\n";

        // Deformed geometry (blue)
        file << "<g stroke=\"blue\" fill=\"none\" stroke-width=\"2\">\n";
        for (int j = 0; j < fem_.ny - 1; ++j) {
            for (int i = 0; i < fem_.nx - 1; ++i) {
                double xi[] = {(double)i, (double)(i+1), (double)(i+1), (double)i, (double)i};
                double yi[] = {(double)j, (double)j, (double)(j+1), (double)(j+1), (double)j};

                file << "<polyline points=\"";
                for (int k = 0; k < 5; ++k) {
                    double x = margin_x_ + (xi[k] * (fem_.width / (fem_.nx - 1)) +
                               fem_.u[fem_.node_idx((int)xi[k], (int)yi[k])] * deform_scale_) * scale_x_;
                    double y = margin_y_ + (fem_.height - (yi[k] * (fem_.height / (fem_.ny - 1)) +
                               fem_.v[fem_.node_idx((int)xi[k], (int)yi[k])] * deform_scale_)) * scale_y_;
                    file << x << "," << y;
                    if (k < 4) file << " ";
                }
                file << "\"/>\n";
            }
        }
        file << "</g>\n";
    }

    void draw_bcs_impl(std::ofstream& file) {
        // Supports (blue triangles)
        file << "<!-- Supports (v=0) -->\n";
        file << "<g fill=\"#0066cc\" stroke=\"#003366\" stroke-width=\"1\">\n";
        for (int node_idx : support_nodes_) {
            int i = node_idx % fem_.nx;
            int j = node_idx / fem_.nx;
            double x = margin_x_ + i * (fem_.width / (fem_.nx - 1)) * scale_x_;
            double y = margin_y_ + (fem_.height - j * (fem_.height / (fem_.ny - 1))) * scale_y_;
            double size = 8.0;

            file << "<polygon points=\"" << x << "," << (y - size) << " ";
            file << (x - size) << "," << (y + size) << " ";
            file << (x + size) << "," << (y + size) << "\"/>\n";
        }
        file << "</g>\n";

        // Loads (red arrows)
        file << "<!-- Loads (applied displacement) -->\n";
        file << "<g stroke=\"#cc0000\" stroke-width=\"2\" fill=\"none\">\n";
        for (int node_idx : load_nodes_) {
            int i = node_idx % fem_.nx;
            int j = node_idx / fem_.nx;
            double x = margin_x_ + i * (fem_.width / (fem_.nx - 1)) * scale_x_;
            double y = margin_y_ + (fem_.height - j * (fem_.height / (fem_.ny - 1))) * scale_y_;
            double arrow_len = 15.0;
            double arrow_head = 4.0;

            file << "<line x1=\"" << x << "\" y1=\"" << (y - arrow_len) << "\" ";
            file << "x2=\"" << x << "\" y2=\"" << (y + arrow_len) << "\"/>\n";

            file << "<polygon points=\"" << x << "," << (y + arrow_len) << " ";
            file << (x - arrow_head) << "," << (y + arrow_len - arrow_head) << " ";
            file << (x + arrow_head) << "," << (y + arrow_len - arrow_head) << "\" ";
            file << "fill=\"#cc0000\"/>\n";
        }
        file << "</g>\n";
    }

    static std::string value_to_color(double val, double vmin, double vmax) {
        double offset = vmax * 1e-4;
        double log_val = std::log10(val + offset);
        double log_vmin = std::log10(vmin + offset);
        double log_vmax = std::log10(vmax + offset);

        double norm = (log_val - log_vmin) / (log_vmax - log_vmin);
        norm = std::max(0.0, std::min(1.0, norm));

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

// Legacy API (kept for compatibility, uses SVGVisualization internally)
class SVGGenerator {
public:
    static void write_mesh_with_bcs(const ElasticityFEM2D& fem, const std::string& filename,
                                    const std::vector<int>& support_nodes,
                                    const std::vector<int>& load_nodes) {
        SVGVisualization(fem)
            .margin(0.1)
            .width(600)
            .mesh()
            .boundary_conditions(support_nodes, load_nodes)
            .write(filename);
    }
    static void write_von_mises(const ElasticityFEM2D& fem, const std::string& filename) {
        SVGVisualization(fem)
            .margin(0.05)
            .width(600)
            .von_mises()
            .write(filename);
    }

public:
    static void write_deformed(const ElasticityFEM2D& fem, const std::string& filename,
                               double deform_scale = 100.0) {
        SVGVisualization(fem)
            .margin(0.05)
            .width(600)
            .deform_scale(deform_scale)
            .mesh()
            .deformed()
            .write(filename);
    }
};
