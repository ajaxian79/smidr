#include "core/Reconstruct.h"
#include "core/SDF.h"
#include "core/ConvexHull.h"

#include <algorithm>
#include <cmath>

namespace smidr {

std::vector<Vec3> estimate_normals_from_neighbors(const std::vector<Vec3>& pts, int k) {
    std::vector<Vec3> normals(pts.size());
    for (size_t i = 0; i < pts.size(); ++i) {
        std::vector<std::pair<float, size_t>> dist;
        for (size_t j = 0; j < pts.size(); ++j) {
            if (i == j) continue;
            dist.push_back({(pts[i] - pts[j]).length(), j});
        }
        std::partial_sort(dist.begin(),
                           dist.begin() + std::min(static_cast<size_t>(k), dist.size()),
                           dist.end());
        Vec3 c = pts[i];
        int taken = std::min(k, static_cast<int>(dist.size()));
        for (int n = 0; n < taken; ++n) c += pts[dist[n].second];
        c = c * (1.f / static_cast<float>(taken + 1));

        Vec3 v0{0, 0, 0}, v1{0, 0, 0}, v2{0, 0, 0};
        for (int n = 0; n < taken; ++n) {
            Vec3 d = pts[dist[n].second] - c;
            v0 += Vec3{d.x * d.x, d.y * d.y, d.z * d.z};
            v1 += Vec3{d.x * d.y, d.x * d.z, d.y * d.z};
            (void)v2;
        }
        Vec3 best{0, 1, 0};
        if (v0.x < v0.y && v0.x < v0.z) best = {1, 0, 0};
        else if (v0.z < v0.x && v0.z < v0.y) best = {0, 0, 1};
        normals[i] = best;
    }
    return normals;
}

Mesh alpha_shape(const std::vector<Vec3>& pts, float alpha) {
    if (pts.size() < 4) return {};
    Mesh hull = convex_hull_3d(pts);
    Mesh result;
    float a2 = alpha * alpha;
    for (size_t i = 0; i + 2 < hull.indices.size(); i += 3) {
        Vec3 v0 = hull.vertices[hull.indices[i]].position;
        Vec3 v1 = hull.vertices[hull.indices[i+1]].position;
        Vec3 v2 = hull.vertices[hull.indices[i+2]].position;
        Vec3 e1 = v1 - v0, e2 = v2 - v0;
        float area2 = e1.cross(e2).length() * e1.cross(e2).length() * 0.25f;
        if (area2 > a2) continue;
        uint32_t base = static_cast<uint32_t>(result.vertices.size());
        Vec3 n = e1.cross(e2).normalized();
        result.vertices.push_back({v0, n});
        result.vertices.push_back({v1, n});
        result.vertices.push_back({v2, n});
        result.indices.insert(result.indices.end(), {base, base+1, base+2});
    }
    return result;
}

Mesh ball_pivoting(const std::vector<Vec3>& pts, float ball_radius) {
    if (pts.size() < 3) return {};
    return alpha_shape(pts, ball_radius * 2.f);
}

Mesh poisson_indicator_marching(const std::vector<Vec3>& pts,
                                  const std::vector<Vec3>& normals,
                                  int resolution) {
    if (pts.empty()) return {};
    AABB bb;
    for (auto& p : pts) bb.expand(p);
    Vec3 ext = bb.extent() * 0.2f;
    bb.min_pt -= Vec3{ext.x, ext.y, ext.z};
    bb.max_pt += Vec3{ext.x, ext.y, ext.z};

    SDF indicator = [pts, normals](Vec3 p) {
        float val = 0.f;
        for (size_t i = 0; i < pts.size(); ++i) {
            Vec3 d = p - pts[i];
            float r = d.length();
            float r2 = r * r + 0.01f;
            Vec3 n = i < normals.size() ? normals[i] : Vec3{0, 1, 0};
            val += n.dot(d) / (r2 * r2 + 0.001f);
        }
        return val;
    };
    return march_cubes(indicator, bb, resolution, 0.f);
}

}  // namespace smidr
