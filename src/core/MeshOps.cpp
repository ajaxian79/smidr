#include "core/MeshOps.h"
#include "core/Analysis.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <utility>

namespace smidr {

Mesh weld_vertices(const Mesh& mesh, float tolerance) {
    Mesh out;
    if (mesh.vertices.empty()) return out;
    float tol2 = tolerance * tolerance;

    std::vector<uint32_t> remap(mesh.vertices.size());
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        remap[i] = static_cast<uint32_t>(out.vertices.size());
        bool found = false;
        for (uint32_t j = 0; j < out.vertices.size(); ++j) {
            Vec3 d = out.vertices[j].position - mesh.vertices[i].position;
            if (d.dot(d) < tol2) { remap[i] = j; found = true; break; }
        }
        if (!found) out.vertices.push_back(mesh.vertices[i]);
    }
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        uint32_t a = remap[mesh.indices[i]];
        uint32_t b = remap[mesh.indices[i+1]];
        uint32_t c = remap[mesh.indices[i+2]];
        if (a != b && b != c && a != c)
            out.indices.insert(out.indices.end(), {a, b, c});
    }
    return out;
}

Mesh remove_degenerate_triangles(const Mesh& mesh, float min_area) {
    Mesh out;
    out.vertices = mesh.vertices;
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        Vec3 a = mesh.vertices[mesh.indices[i]].position;
        Vec3 b = mesh.vertices[mesh.indices[i+1]].position;
        Vec3 c = mesh.vertices[mesh.indices[i+2]].position;
        float area = (b - a).cross(c - a).length() * 0.5f;
        if (area > min_area)
            out.indices.insert(out.indices.end(),
                {mesh.indices[i], mesh.indices[i+1], mesh.indices[i+2]});
    }
    return out;
}

Mesh recompute_normals(const Mesh& mesh, bool smooth) {
    Mesh out = mesh;
    if (smooth) {
        for (auto& v : out.vertices) v.normal = {0, 0, 0};
        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            Vec3 a = out.vertices[mesh.indices[i]].position;
            Vec3 b = out.vertices[mesh.indices[i+1]].position;
            Vec3 c = out.vertices[mesh.indices[i+2]].position;
            Vec3 fn = (b - a).cross(c - a).normalized();
            out.vertices[mesh.indices[i]].normal += fn;
            out.vertices[mesh.indices[i+1]].normal += fn;
            out.vertices[mesh.indices[i+2]].normal += fn;
        }
        for (auto& v : out.vertices) v.normal = v.normal.normalized();
    } else {
        out.vertices.clear();
        out.indices.clear();
        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            Vec3 a = mesh.vertices[mesh.indices[i]].position;
            Vec3 b = mesh.vertices[mesh.indices[i+1]].position;
            Vec3 c = mesh.vertices[mesh.indices[i+2]].position;
            Vec3 n = (b - a).cross(c - a).normalized();
            uint32_t base = static_cast<uint32_t>(out.vertices.size());
            out.vertices.push_back({a, n});
            out.vertices.push_back({b, n});
            out.vertices.push_back({c, n});
            out.indices.insert(out.indices.end(), {base, base+1, base+2});
        }
    }
    return out;
}

Mesh simplify_decimate(const Mesh& mesh, float target_ratio) {
    Mesh welded = weld_vertices(mesh, 1e-5f);
    if (welded.indices.size() < 9) return welded;

    int target_tris = static_cast<int>(static_cast<float>(welded.indices.size() / 3) * target_ratio);
    if (target_tris < 4) target_tris = 4;

    Mesh result = welded;

    while (static_cast<int>(result.indices.size() / 3) > target_tris) {
        struct Edge { uint32_t a, b; size_t tri_idx; float len2; };
        std::vector<Edge> edges;
        for (size_t i = 0; i + 2 < result.indices.size(); i += 3) {
            for (int j = 0; j < 3; ++j) {
                uint32_t a = result.indices[i + static_cast<size_t>(j)];
                uint32_t b = result.indices[i + static_cast<size_t>((j+1)%3)];
                Vec3 d = result.vertices[a].position - result.vertices[b].position;
                edges.push_back({std::min(a,b), std::max(a,b), i, d.dot(d)});
            }
        }
        if (edges.empty()) break;

        auto shortest = std::min_element(edges.begin(), edges.end(),
            [](const Edge& x, const Edge& y) { return x.len2 < y.len2; });
        uint32_t a = shortest->a, b = shortest->b;

        Vec3 mid = (result.vertices[a].position + result.vertices[b].position) * 0.5f;
        Vec3 mn = (result.vertices[a].normal + result.vertices[b].normal).normalized();
        result.vertices[a].position = mid;
        result.vertices[a].normal = mn;

        std::vector<uint32_t> new_indices;
        for (size_t i = 0; i + 2 < result.indices.size(); i += 3) {
            uint32_t i0 = result.indices[i], i1 = result.indices[i+1], i2 = result.indices[i+2];
            if (i0 == b) i0 = a;
            if (i1 == b) i1 = a;
            if (i2 == b) i2 = a;
            if (i0 != i1 && i1 != i2 && i0 != i2)
                new_indices.insert(new_indices.end(), {i0, i1, i2});
        }
        result.indices = new_indices;
        if (result.indices.size() / 3 == welded.indices.size() / 3) break;
        welded = result;
    }
    return result;
}

Mesh subdivide_loop(const Mesh& mesh) {
    Mesh in = weld_vertices(mesh, 1e-5f);
    Mesh out;
    out.vertices = in.vertices;

    std::unordered_map<uint64_t, uint32_t> edge_midpoint;
    auto edge_key = [](uint32_t a, uint32_t b) {
        if (a > b) std::swap(a, b);
        return (static_cast<uint64_t>(a) << 32) | b;
    };
    auto get_mid = [&](uint32_t a, uint32_t b) {
        uint64_t k = edge_key(a, b);
        auto it = edge_midpoint.find(k);
        if (it != edge_midpoint.end()) return it->second;
        Vec3 mp = (in.vertices[a].position + in.vertices[b].position) * 0.5f;
        Vec3 mn = (in.vertices[a].normal + in.vertices[b].normal).normalized();
        uint32_t id = static_cast<uint32_t>(out.vertices.size());
        out.vertices.push_back({mp, mn});
        edge_midpoint[k] = id;
        return id;
    };

    for (size_t i = 0; i + 2 < in.indices.size(); i += 3) {
        uint32_t a = in.indices[i], b = in.indices[i+1], c = in.indices[i+2];
        uint32_t ab = get_mid(a, b);
        uint32_t bc = get_mid(b, c);
        uint32_t ca = get_mid(c, a);
        out.indices.insert(out.indices.end(), {
            a, ab, ca,
            b, bc, ab,
            c, ca, bc,
            ab, bc, ca,
        });
    }
    return out;
}

Mesh fill_holes(const Mesh& mesh) {
    Mesh in = weld_vertices(mesh, 1e-5f);
    Mesh out = in;

    struct EdgePair { uint32_t a, b; };
    std::unordered_map<uint64_t, int> edge_count;
    auto edge_key = [](uint32_t a, uint32_t b) {
        if (a > b) std::swap(a, b);
        return (static_cast<uint64_t>(a) << 32) | b;
    };
    for (size_t i = 0; i + 2 < in.indices.size(); i += 3) {
        for (int j = 0; j < 3; ++j) {
            uint32_t a = in.indices[i + static_cast<size_t>(j)];
            uint32_t b = in.indices[i + static_cast<size_t>((j+1)%3)];
            edge_count[edge_key(a, b)]++;
        }
    }

    std::vector<EdgePair> boundary;
    for (auto& [k, count] : edge_count) {
        if (count == 1) {
            uint32_t a = static_cast<uint32_t>(k >> 32);
            uint32_t b = static_cast<uint32_t>(k & 0xFFFFFFFF);
            boundary.push_back({a, b});
        }
    }

    if (!boundary.empty()) {
        Vec3 center{0, 0, 0};
        int count = 0;
        for (auto& e : boundary) {
            center += in.vertices[e.a].position;
            center += in.vertices[e.b].position;
            count += 2;
        }
        if (count > 0) center = center * (1.f / static_cast<float>(count));
        uint32_t cidx = static_cast<uint32_t>(out.vertices.size());
        out.vertices.push_back({center, {0, 1, 0}});
        for (auto& e : boundary) {
            out.indices.insert(out.indices.end(), {e.a, e.b, cidx});
        }
    }
    return out;
}

Mesh laplacian_smooth(const Mesh& mesh, int iterations, float strength) {
    Mesh out = weld_vertices(mesh, 1e-5f);

    std::vector<std::vector<uint32_t>> adj(out.vertices.size());
    for (size_t i = 0; i + 2 < out.indices.size(); i += 3) {
        for (int j = 0; j < 3; ++j) {
            uint32_t a = out.indices[i + static_cast<size_t>(j)];
            uint32_t b = out.indices[i + static_cast<size_t>((j+1)%3)];
            if (std::find(adj[a].begin(), adj[a].end(), b) == adj[a].end()) adj[a].push_back(b);
            if (std::find(adj[b].begin(), adj[b].end(), a) == adj[b].end()) adj[b].push_back(a);
        }
    }

    for (int it = 0; it < iterations; ++it) {
        std::vector<Vec3> new_pos(out.vertices.size());
        for (size_t i = 0; i < out.vertices.size(); ++i) {
            if (adj[i].empty()) { new_pos[i] = out.vertices[i].position; continue; }
            Vec3 sum{0, 0, 0};
            for (uint32_t n : adj[i]) sum += out.vertices[n].position;
            sum = sum * (1.f / static_cast<float>(adj[i].size()));
            new_pos[i] = out.vertices[i].position * (1.f - strength) + sum * strength;
        }
        for (size_t i = 0; i < out.vertices.size(); ++i)
            out.vertices[i].position = new_pos[i];
    }
    return recompute_normals(out, true);
}

bool is_watertight(const Mesh& mesh) {
    std::unordered_map<uint64_t, int> edge_count;
    auto edge_key = [](uint32_t a, uint32_t b) {
        if (a > b) std::swap(a, b);
        return (static_cast<uint64_t>(a) << 32) | b;
    };
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        for (int j = 0; j < 3; ++j) {
            uint32_t a = mesh.indices[i + static_cast<size_t>(j)];
            uint32_t b = mesh.indices[i + static_cast<size_t>((j+1)%3)];
            edge_count[edge_key(a, b)]++;
        }
    }
    for (auto& [k, count] : edge_count)
        if (count != 2) return false;
    return !edge_count.empty();
}

MeshStats compute_mesh_stats(const Mesh& mesh) {
    MeshStats s{};
    s.vertex_count = static_cast<int>(mesh.vertices.size());
    s.triangle_count = static_cast<int>(mesh.indices.size() / 3);

    std::unordered_map<uint64_t, int> edge_count;
    auto edge_key = [](uint32_t a, uint32_t b) {
        if (a > b) std::swap(a, b);
        return (static_cast<uint64_t>(a) << 32) | b;
    };
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        for (int j = 0; j < 3; ++j) {
            uint32_t a = mesh.indices[i + static_cast<size_t>(j)];
            uint32_t b = mesh.indices[i + static_cast<size_t>((j+1)%3)];
            edge_count[edge_key(a, b)]++;
        }
    }
    s.edge_count = static_cast<int>(edge_count.size());
    for (auto& [k, c] : edge_count) {
        if (c == 1) s.boundary_edge_count++;
        if (c > 2) s.non_manifold_edge_count++;
    }

    s.total_area = compute_surface_area(mesh);
    s.total_volume = is_watertight(mesh) ? compute_volume(mesh) : 0.f;

    int euler = s.vertex_count - s.edge_count + s.triangle_count;
    s.genus_estimate = (2 - euler) / 2;
    return s;
}

}  // namespace smidr
