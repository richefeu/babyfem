#pragma once
// colormap.hpp — petites palettes de couleurs partagées par les outils de la
// série (babyfem, kidfem, ...). Convertit une valeur scalaire en couleur SVG.
//
// Deux palettes :
//   - VIRIDIS  : violet -> bleu -> turquoise -> vert -> jaune (champs positifs,
//                p. ex. von Mises)
//   - COOLWARM : bleu -> blanc -> rouge (champs signés, symétriques autour de 0)

#include <algorithm>
#include <string>
#include <cstdio>
#include <utility>

enum class ColorMap { VIRIDIS, COOLWARM };

namespace colormap {

struct RGB { int r, g, b; };

// t ∈ [0, 1] -> couleur interpolée le long de la palette.
inline RGB rgb(double t, ColorMap cm) {
    t = std::max(0.0, std::min(1.0, t));
    struct CP { double t; int r, g, b; };
    static const CP V[5] = {
        {0.00,  68,   1,  84}, {0.25,  59,  82, 139},
        {0.50,  33, 145, 140}, {0.75,  94, 201,  98}, {1.00, 253, 231,  37}};
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

inline std::string to_hex(RGB c) {
    char buf[8];
    snprintf(buf, sizeof(buf), "#%02x%02x%02x", c.r, c.g, c.b);
    return buf;
}

// Couleur hex d'une valeur v dans l'intervalle [vmin, vmax].
inline std::string hex(double v, double vmin, double vmax, ColorMap cm) {
    double t = (vmax > vmin) ? (v - vmin) / (vmax - vmin) : 0.5;
    return to_hex(rgb(t, cm));
}

// Préfixe d'unité SI adapté à l'ordre de grandeur (pour les contraintes en Pa).
inline std::pair<double, std::string> auto_unit(double absmax) {
    if (absmax >= 0.9e9) return {1e9, "GPa"};
    if (absmax >= 0.9e6) return {1e6, "MPa"};
    if (absmax >= 0.9e3) return {1e3, "kPa"};
    return {1.0, "Pa"};
}

} // namespace colormap
