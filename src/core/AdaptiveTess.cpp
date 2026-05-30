#include "core/AdaptiveTess.h"
#include "core/Curvature.h"
#include "core/MeshOps.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace smidr {

Mesh refine_long_edges(const Mesh& mesh, float max_edge_len) {
    Mesh in = weld_vertices(mesh, 1e-5f);
    Mesh out;
    out.vertices = in.vertices;

    std::unordered_map<uint64_t, uint32_t> edge_midpoint;
    auto edge_key = [](uint32_t a, uint32_t b) {
        if (a > b) std::swap(a, b);
        return (static_cast<uint64_t>(a) << 32) | b;
    };
    auto get_mid = [&](uint32_t a, uint32_t b) -> uint32_t {
        Vec3 d = in.vertices[a].position - in.vertices[b].position;
        if (d.length() <= max_edge_len) return UINT32_MAX;
        uint64_t k = edge_key(a, b);
        auto it = edge_midpoint.find(k);
        if (it != edge_midpoint.end()) return it->second;
        Vec3 mid = (in.vertices[a].position + in.vertices[b].position) * 0.5f;
        Vec3 n = (in.vertices[a].normal + in.vertices[b].normal).normalized();
        uint32_t id = static_cast<uint32_t>(out.vertices.size());
        out.vertices.push_back({mid, n});
        edge_midpoint[k] = id;
        return id;
    };

    for (size_t i = 0; i + 2 < in.indices.size(); i += 3) {
        uint32_t a = in.indices[i], b = in.indices[i+1], c = in.indices[i+2];
        uint32_t ab = get_mid(a, b);
        uint32_t bc = get_mid(b, c);
        uint32_t ca = get_mid(c, a);

        int split_mask = (ab != UINT32_MAX ? 1 : 0)
                       | (bc != UINT32_MAX ? 2 : 0)
                       | (ca != UINT32_MAX ? 4 : 0);

        switch (split_mask) {
            case 0:
                out.indices.insert(out.indices.end(), {a, b, c}); break;
            case 1:
                out.indices.insert(out.indices.end(), {a, ab, c, ab, b, c}); break;
            case 2:
                out.indices.insert(out.indices.end(), {a, b, bc, a, bc, c}); break;
            case 4:
                out.indices.insert(out.indices.end(), {a, b, ca, b, c, ca}); break;
            case 3:
                out.indices.insert(out.indices.end(), {a, ab, c, ab, bc, c, ab, b, bc}); break;
            case 5:
                out.indices.insert(out.indices.end(), {a, ab, ca, ab, b, ca, b, c, ca}); break;
            case 6:
                out.indices.insert(out.indices.end(), {a, b, bc, a, bc, ca, ca, bc, c}); break;
            case 7:
                out.indices.insert(out.indices.end(), {a, ab, ca, ab, b, bc, bc, c, ca, ab, bc, ca}); break;
        }
    }
    return out;
}

Mesh collapse_short_edges(const Mesh& mesh, float min_edge_len) {
    Mesh in = weld_vertices(mesh, 1e-5f);
    std::vector<uint32_t> remap(in.vertices.size());
    for (size_t i = 0; i < in.vertices.size(); ++i) remap[i] = static_cast<uint32_t>(i);
    float min2 = min_edge_len * min_edge_len;

    for (size_t i = 0; i + 2 < in.indices.size(); i += 3) {
        for (int j = 0; j < 3; ++j) {
            uint32_t a = remap[in.indices[i + static_cast<size_t>(j)]];
            uint32_t b = remap[in.indices[i + static_cast<size_t>((j+1)%3)]];
            if (a == b) continue;
            Vec3 d = in.vertices[a].position - in.vertices[b].position;
            if (d.dot(d) < min2) {
                uint32_t lo = std::min(a, b), hi = std::max(a, b);
                for (auto& r : remap) if (r == hi) r = lo;
            }
        }
    }

    Mesh out;
    std::unordered_map<uint32_t, uint32_t> compact;
    for (size_t i = 0; i < in.vertices.size(); ++i) {
        uint32_t m = remap[i];
        if (compact.find(m) == compact.end()) {
            compact[m] = static_cast<uint32_t>(out.vertices.size());
            out.vertices.push_back(in.vertices[m]);
        }
    }
    for (size_t i = 0; i + 2 < in.indices.size(); i += 3) {
        uint32_t a = compact[remap[in.indices[i]]];
        uint32_t b = compact[remap[in.indices[i+1]]];
        uint32_t c = compact[remap[in.indices[i+2]]];
        if (a != b && b != c && a != c) {
            out.indices.insert(out.indices.end(), {a, b, c});
        }
    }
    return out;
}

Mesh adaptive_subdivide(const Mesh& mesh, float curvature_threshold, int max_iter) {
    Mesh current = mesh;
    for (int iter = 0; iter < max_iter; ++iter) {
        auto curv = compute_curvature(current);
        Mesh out;
        out.vertices = current.vertices;

        std::unordered_map<uint64_t, uint32_t> edge_midpoint;
        auto edge_key = [](uint32_t a, uint32_t b) {
            if (a > b) std::swap(a, b);
            return (static_cast<uint64_t>(a) << 32) | b;
        };
        auto get_mid = [&](uint32_t a, uint32_t b, bool should_split) -> uint32_t {
            if (!should_split) return UINT32_MAX;
            uint64_t k = edge_key(a, b);
            auto it = edge_midpoint.find(k);
            if (it != edge_midpoint.end()) return it->second;
            Vec3 mid = (current.vertices[a].position + current.vertices[b].position) * 0.5f;
            Vec3 n = (current.vertices[a].normal + current.vertices[b].normal).normalized();
            uint32_t id = static_cast<uint32_t>(out.vertices.size());
            out.vertices.push_back({mid, n});
            edge_midpoint[k] = id;
            return id;
        };

        for (size_t i = 0; i + 2 < current.indices.size(); i += 3) {
            uint32_t a = current.indices[i], b = current.indices[i+1], c = current.indices[i+2];
            float ca_curv = a < curv.size() ? std::abs(curv[a].mean) : 0;
            float cb_curv = b < curv.size() ? std::abs(curv[b].mean) : 0;
            float cc_curv = c < curv.size() ? std::abs(curv[c].mean) : 0;
            bool split_ab = (ca_curv + cb_curv) * 0.5f > curvature_threshold;
            bool split_bc = (cb_curv + cc_curv) * 0.5f > curvature_threshold;
            bool split_ca = (cc_curv + ca_curv) * 0.5f > curvature_threshold;

            uint32_t ab = get_mid(a, b, split_ab);
            uint32_t bc = get_mid(b, c, split_bc);
            uint32_t ca_m = get_mid(c, a, split_ca);

            int mask = (split_ab ? 1 : 0) | (split_bc ? 2 : 0) | (split_ca ? 4 : 0);
            switch (mask) {
                case 0: out.indices.insert(out.indices.end(), {a, b, c}); break;
                case 7: out.indices.insert(out.indices.end(),
                    {a, ab, ca_m, ab, b, bc, bc, c, ca_m, ab, bc, ca_m}); break;
                default: out.indices.insert(out.indices.end(), {a, b, c}); break;
            }
        }
        current = out;
    }
    return current;
}

Mesh remesh_adaptive(const Mesh& mesh, float target_edge, float curvature_weight) {
    auto curv = compute_curvature(mesh);
    float min_edge = target_edge * (1.f - curvature_weight);
    float max_edge = target_edge * (1.f + curvature_weight);
    Mesh refined = refine_long_edges(mesh, max_edge);
    Mesh decimated = collapse_short_edges(refined, min_edge);
    return decimated;
}

}  // namespace smidr
