#include "core/Octree.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace smidr {

bool Octree::ray_aabb(const Ray& ray, const AABB& box, float& tmin) const {
    float t1, t2;
    tmin = 0.f;
    float tmax = 1e30f;
    auto slab = [&](float o, float d, float mn, float mx) {
        if (std::abs(d) < 1e-8f) return o >= mn && o <= mx;
        t1 = (mn - o) / d; t2 = (mx - o) / d;
        if (t1 > t2) std::swap(t1, t2);
        tmin = std::max(tmin, t1);
        tmax = std::min(tmax, t2);
        return tmin <= tmax;
    };
    if (!slab(ray.origin.x, ray.direction.x, box.min_pt.x, box.max_pt.x)) return false;
    if (!slab(ray.origin.y, ray.direction.y, box.min_pt.y, box.max_pt.y)) return false;
    if (!slab(ray.origin.z, ray.direction.z, box.min_pt.z, box.max_pt.z)) return false;
    return true;
}

int Octree::build_recursive(const std::vector<int>& indices, AABB bounds,
                              int depth, int max_depth, int max_leaf_size) {
    int idx = static_cast<int>(nodes_.size());
    nodes_.push_back({});
    nodes_[idx].bounds = bounds;

    if (static_cast<int>(indices.size()) <= max_leaf_size || depth >= max_depth) {
        nodes_[idx].tri_indices = indices;
        return idx;
    }

    Vec3 c = bounds.center();
    std::vector<int> octant[8];
    for (int t : indices) {
        Vec3 tri_c = (triangles_[t].v0 + triangles_[t].v1 + triangles_[t].v2) * (1.f/3.f);
        int o = (tri_c.x > c.x ? 1 : 0) | (tri_c.y > c.y ? 2 : 0) | (tri_c.z > c.z ? 4 : 0);
        octant[o].push_back(t);
    }

    bool split = false;
    for (int i = 0; i < 8; ++i) {
        if (octant[i].empty()) continue;
        if (octant[i].size() == indices.size()) {
            nodes_[idx].tri_indices = indices;
            return idx;
        }
        split = true;
        Vec3 mn = bounds.min_pt, mx = bounds.max_pt;
        if (i & 1) mn.x = c.x; else mx.x = c.x;
        if (i & 2) mn.y = c.y; else mx.y = c.y;
        if (i & 4) mn.z = c.z; else mx.z = c.z;
        AABB cb; cb.min_pt = mn; cb.max_pt = mx;
        int child = build_recursive(octant[i], cb, depth + 1, max_depth, max_leaf_size);
        nodes_[idx].children[i] = child;
    }
    if (split) nodes_[idx].is_leaf = false;
    else nodes_[idx].tri_indices = indices;
    return idx;
}

void Octree::build(const std::vector<TriangleRef>& tris, int max_depth, int max_leaf_size) {
    triangles_ = tris;
    nodes_.clear();
    if (triangles_.empty()) return;

    AABB bounds;
    for (auto& t : triangles_) {
        bounds.expand(t.v0); bounds.expand(t.v1); bounds.expand(t.v2);
    }
    std::vector<int> indices(triangles_.size());
    for (size_t i = 0; i < triangles_.size(); ++i) indices[i] = static_cast<int>(i);
    build_recursive(indices, bounds, 0, max_depth, max_leaf_size);
}

std::optional<HitResult> Octree::ray_cast_node(const Ray& ray, int node_idx) const {
    if (node_idx < 0 || node_idx >= static_cast<int>(nodes_.size())) return std::nullopt;
    const auto& node = nodes_[node_idx];
    float tmin;
    if (!ray_aabb(ray, node.bounds, tmin)) return std::nullopt;

    std::optional<HitResult> best;
    if (node.is_leaf) {
        for (int ti : node.tri_indices) {
            auto& t = triangles_[ti];
            float th = ray_triangle(ray, t.v0, t.v1, t.v2);
            if (th > 0.f && (!best || th < best->distance)) {
                HitResult h;
                h.node_id = t.node_id;
                h.distance = th;
                h.point = ray.origin + ray.direction * th;
                h.normal = (t.v1 - t.v0).cross(t.v2 - t.v0).normalized();
                best = h;
            }
        }
    } else {
        for (int c : node.children) {
            if (c < 0) continue;
            auto h = ray_cast_node(ray, c);
            if (h && (!best || h->distance < best->distance)) best = h;
        }
    }
    return best;
}

std::optional<HitResult> Octree::ray_cast(const Ray& ray) const {
    if (nodes_.empty()) return std::nullopt;
    return ray_cast_node(ray, 0);
}

}  // namespace smidr
