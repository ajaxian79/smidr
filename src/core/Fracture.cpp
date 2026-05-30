#include "core/Fracture.h"
#include "core/Analysis.h"

#include <algorithm>
#include <cmath>

namespace smidr {

CutResult plane_cut(const Mesh& source, Vec3 plane_normal, float plane_offset) {
    CutResult result;
    Vec3 n = plane_normal.normalized();

    for (size_t i = 0; i + 2 < source.indices.size(); i += 3) {
        Vec3 v0 = source.vertices[source.indices[i]].position;
        Vec3 v1 = source.vertices[source.indices[i+1]].position;
        Vec3 v2 = source.vertices[source.indices[i+2]].position;
        float d0 = v0.dot(n) - plane_offset;
        float d1 = v1.dot(n) - plane_offset;
        float d2 = v2.dot(n) - plane_offset;
        Vec3 fn = (v1 - v0).cross(v2 - v0).normalized();

        auto add_tri = [](Mesh& m, Vec3 a, Vec3 b, Vec3 c, Vec3 n_) {
            uint32_t base = static_cast<uint32_t>(m.vertices.size());
            m.vertices.push_back({a, n_});
            m.vertices.push_back({b, n_});
            m.vertices.push_back({c, n_});
            m.indices.insert(m.indices.end(), {base, base+1, base+2});
        };

        int positive_count = (d0 > 0 ? 1 : 0) + (d1 > 0 ? 1 : 0) + (d2 > 0 ? 1 : 0);
        if (positive_count == 0) {
            add_tri(result.below, v0, v1, v2, fn);
        } else if (positive_count == 3) {
            add_tri(result.above, v0, v1, v2, fn);
        } else {
            Vec3 pts[3] = {v0, v1, v2};
            float dists[3] = {d0, d1, d2};
            std::vector<Vec3> above_pts, below_pts;
            for (int e = 0; e < 3; ++e) {
                int next = (e + 1) % 3;
                if (dists[e] >= 0) above_pts.push_back(pts[e]);
                else below_pts.push_back(pts[e]);
                if ((dists[e] >= 0) != (dists[next] >= 0)) {
                    float t = dists[e] / (dists[e] - dists[next]);
                    Vec3 ip = pts[e] + (pts[next] - pts[e]) * t;
                    above_pts.push_back(ip);
                    below_pts.push_back(ip);
                }
            }
            for (size_t k = 1; k + 1 < above_pts.size(); ++k)
                add_tri(result.above, above_pts[0], above_pts[k], above_pts[k+1], fn);
            for (size_t k = 1; k + 1 < below_pts.size(); ++k)
                add_tri(result.below, below_pts[0], below_pts[k], below_pts[k+1], fn);
        }
    }

    auto cap = [&](Mesh& m, Vec3 cap_normal) {
        std::vector<Vec3> boundary_points;
        for (size_t i = 0; i < m.vertices.size(); ++i) {
            float d = m.vertices[i].position.dot(n) - plane_offset;
            if (std::abs(d) < 1e-4f) boundary_points.push_back(m.vertices[i].position);
        }
        if (boundary_points.size() < 3) return;
        Vec3 centroid{0, 0, 0};
        for (auto& p : boundary_points) centroid += p;
        centroid = centroid * (1.f / static_cast<float>(boundary_points.size()));
        Vec3 u_axis = (boundary_points[0] - centroid).normalized();
        Vec3 v_axis = n.cross(u_axis).normalized();
        std::sort(boundary_points.begin(), boundary_points.end(),
                   [&](Vec3 a, Vec3 b) {
                       float aa = std::atan2((a - centroid).dot(v_axis), (a - centroid).dot(u_axis));
                       float ba = std::atan2((b - centroid).dot(v_axis), (b - centroid).dot(u_axis));
                       return aa < ba;
                   });
        uint32_t base = static_cast<uint32_t>(m.vertices.size());
        m.vertices.push_back({centroid, cap_normal});
        for (auto& p : boundary_points) m.vertices.push_back({p, cap_normal});
        for (size_t i = 0; i < boundary_points.size(); ++i) {
            m.indices.push_back(base);
            m.indices.push_back(base + 1 + static_cast<uint32_t>(i));
            m.indices.push_back(base + 1 + static_cast<uint32_t>((i + 1) % boundary_points.size()));
        }
    };
    cap(result.above, n * -1.f);
    cap(result.below, n);
    return result;
}

std::vector<Vec3> generate_impact_seeds(Vec3 impact, AABB bb, int count,
                                          float radius_scale, uint32_t seed) {
    std::vector<Vec3> out;
    uint32_t rng = seed;
    auto rand01 = [&]() {
        rng = rng * 1664525u + 1013904223u;
        return static_cast<float>(rng & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
    };
    Vec3 ext = bb.extent();
    float max_radius = std::max({ext.x, ext.y, ext.z}) * radius_scale;
    for (int i = 0; i < count; ++i) {
        float u = rand01(), v = rand01(), w = rand01();
        float theta = u * kTwoPi;
        float phi = std::acos(2.f * v - 1.f);
        float r = std::pow(w, 1.f / 3.f) * max_radius;
        Vec3 p{r * std::sin(phi) * std::cos(theta),
               r * std::sin(phi) * std::sin(theta),
               r * std::cos(phi)};
        out.push_back(impact + p);
    }
    return out;
}

std::vector<Mesh> voronoi_fracture(const Mesh& source, const std::vector<Vec3>& seeds) {
    std::vector<Mesh> fragments;
    if (seeds.empty()) {
        fragments.push_back(source);
        return fragments;
    }

    for (size_t si = 0; si < seeds.size(); ++si) {
        Mesh piece = source;
        for (size_t sj = 0; sj < seeds.size(); ++sj) {
            if (si == sj) continue;
            Vec3 mid = (seeds[si] + seeds[sj]) * 0.5f;
            Vec3 normal = (seeds[si] - seeds[sj]).normalized();
            float d = mid.dot(normal);
            auto cut = plane_cut(piece, normal, d);
            piece = cut.above;
        }
        if (!piece.indices.empty()) fragments.push_back(piece);
    }
    return fragments;
}

}  // namespace smidr
