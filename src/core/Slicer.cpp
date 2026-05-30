#include "core/Slicer.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <unordered_map>

namespace smidr {

struct Segment { Vec3 a, b; };

static std::vector<Segment> slice_triangles(const Mesh& mesh, Vec3 n, float d) {
    std::vector<Segment> segments;
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        Vec3 v0 = mesh.vertices[mesh.indices[i]].position;
        Vec3 v1 = mesh.vertices[mesh.indices[i+1]].position;
        Vec3 v2 = mesh.vertices[mesh.indices[i+2]].position;
        float d0 = v0.dot(n) - d;
        float d1 = v1.dot(n) - d;
        float d2 = v2.dot(n) - d;

        Vec3 verts[3] = {v0, v1, v2};
        float dists[3] = {d0, d1, d2};

        std::vector<Vec3> crossings;
        for (int e = 0; e < 3; ++e) {
            int a = e, b = (e + 1) % 3;
            if ((dists[a] >= 0) != (dists[b] >= 0)) {
                float t = dists[a] / (dists[a] - dists[b]);
                crossings.push_back(verts[a] + (verts[b] - verts[a]) * t);
            }
        }
        if (crossings.size() == 2) {
            segments.push_back({crossings[0], crossings[1]});
        }
    }
    return segments;
}

std::vector<Polyline> slice_mesh(const Mesh& mesh, Vec3 plane_normal, float plane_offset) {
    Vec3 n = plane_normal.normalized();
    auto segs = slice_triangles(mesh, n, plane_offset);
    std::vector<Polyline> polylines;
    if (segs.empty()) return polylines;

    std::vector<bool> used(segs.size(), false);
    const float tol = 1e-4f;
    const float tol2 = tol * tol;

    auto close = [&](Vec3 a, Vec3 b) {
        Vec3 d = a - b;
        return d.dot(d) < tol2;
    };

    while (true) {
        size_t seed = SIZE_MAX;
        for (size_t i = 0; i < segs.size(); ++i) if (!used[i]) { seed = i; break; }
        if (seed == SIZE_MAX) break;

        Polyline pl;
        pl.points.push_back(segs[seed].a);
        pl.points.push_back(segs[seed].b);
        used[seed] = true;

        bool extended = true;
        while (extended) {
            extended = false;
            Vec3 back = pl.points.back();
            for (size_t i = 0; i < segs.size(); ++i) {
                if (used[i]) continue;
                if (close(segs[i].a, back)) {
                    pl.points.push_back(segs[i].b); used[i] = true; extended = true; break;
                }
                if (close(segs[i].b, back)) {
                    pl.points.push_back(segs[i].a); used[i] = true; extended = true; break;
                }
            }
        }
        if (pl.points.size() >= 2 && close(pl.points.front(), pl.points.back())) {
            pl.closed = true;
            pl.points.pop_back();
        }
        polylines.push_back(pl);
    }
    return polylines;
}

std::vector<std::vector<Polyline>> slice_z_stack(const Mesh& mesh,
                                                   float z_min, float z_max, float step) {
    std::vector<std::vector<Polyline>> out;
    if (step <= 0) return out;
    for (float z = z_min; z <= z_max; z += step) {
        out.push_back(slice_mesh(mesh, {0, 0, 1}, z));
    }
    return out;
}

bool export_slices_dxf(const std::filesystem::path& path,
                       const std::vector<std::vector<Polyline>>& slices,
                       float z_min, float step) {
    std::ofstream f(path);
    if (!f.is_open()) return false;

    f << "0\nSECTION\n2\nENTITIES\n";
    for (size_t li = 0; li < slices.size(); ++li) {
        float z = z_min + static_cast<float>(li) * step;
        (void)z;
        for (auto& pl : slices[li]) {
            for (size_t i = 0; i + 1 < pl.points.size(); ++i) {
                Vec3 a = pl.points[i], b = pl.points[i+1];
                f << "0\nLINE\n8\nSLICE\n";
                f << "10\n" << a.x << "\n20\n" << a.y << "\n30\n" << a.z << "\n";
                f << "11\n" << b.x << "\n21\n" << b.y << "\n31\n" << b.z << "\n";
            }
            if (pl.closed && pl.points.size() > 2) {
                Vec3 a = pl.points.back(), b = pl.points.front();
                f << "0\nLINE\n8\nSLICE\n";
                f << "10\n" << a.x << "\n20\n" << a.y << "\n30\n" << a.z << "\n";
                f << "11\n" << b.x << "\n21\n" << b.y << "\n31\n" << b.z << "\n";
            }
        }
    }
    f << "0\nENDSEC\n0\nEOF\n";
    return f.good();
}

}  // namespace smidr
