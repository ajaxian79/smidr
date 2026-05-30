#include "core/EdgeOps.h"
#include "core/MeshOps.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace smidr {

Mesh offset_mesh(const Mesh& mesh, float distance) {
    Mesh result = recompute_normals(mesh, true);
    for (auto& v : result.vertices) {
        v.position = v.position + v.normal * distance;
    }
    return recompute_normals(result, true);
}

Mesh shell_mesh(const Mesh& mesh, float thickness) {
    Mesh outer = mesh;
    Mesh inner = offset_mesh(mesh, -thickness);
    for (auto& v : inner.vertices) v.normal = v.normal * -1.f;

    Mesh result;
    result.vertices = outer.vertices;
    for (auto idx : outer.indices) result.indices.push_back(idx);

    uint32_t base = static_cast<uint32_t>(result.vertices.size());
    for (auto& v : inner.vertices) result.vertices.push_back(v);
    for (size_t i = 0; i + 2 < inner.indices.size(); i += 3) {
        result.indices.push_back(inner.indices[i] + base);
        result.indices.push_back(inner.indices[i+2] + base);
        result.indices.push_back(inner.indices[i+1] + base);
    }

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
    for (auto& [k, count] : edge_count) {
        if (count == 1) {
            uint32_t a = static_cast<uint32_t>(k >> 32);
            uint32_t b = static_cast<uint32_t>(k & 0xFFFFFFFF);
            uint32_t a2 = a + base, b2 = b + base;
            Vec3 e = (mesh.vertices[b].position - mesh.vertices[a].position);
            Vec3 n = e.cross(Vec3{0, 1, 0}).normalized();
            uint32_t rbase = static_cast<uint32_t>(result.vertices.size());
            result.vertices.push_back({mesh.vertices[a].position, n});
            result.vertices.push_back({mesh.vertices[b].position, n});
            result.vertices.push_back({inner.vertices[b].position, n});
            result.vertices.push_back({inner.vertices[a].position, n});
            result.indices.insert(result.indices.end(), {rbase, rbase+1, rbase+2, rbase, rbase+2, rbase+3});
            (void)a2; (void)b2;
        }
    }
    return result;
}

Mesh chamfer_edges(const Mesh& mesh, float dist) {
    Mesh welded = weld_vertices(mesh, 1e-5f);
    Mesh result;
    result.vertices = welded.vertices;
    result.indices = welded.indices;

    std::vector<Vec3> shrink(welded.vertices.size(), {0, 0, 0});
    std::vector<int>  shrink_count(welded.vertices.size(), 0);
    for (size_t i = 0; i + 2 < welded.indices.size(); i += 3) {
        Vec3 c = (welded.vertices[welded.indices[i]].position
                  + welded.vertices[welded.indices[i+1]].position
                  + welded.vertices[welded.indices[i+2]].position) * (1.f/3.f);
        for (int j = 0; j < 3; ++j) {
            uint32_t v = welded.indices[i + static_cast<size_t>(j)];
            Vec3 dir = (c - welded.vertices[v].position).normalized();
            shrink[v] += dir * dist;
            shrink_count[v]++;
        }
    }
    for (size_t i = 0; i < result.vertices.size(); ++i) {
        if (shrink_count[i] > 0) {
            result.vertices[i].position += shrink[i] * (1.f / static_cast<float>(shrink_count[i]));
        }
    }
    return recompute_normals(result, true);
}

Mesh fillet_edges(const Mesh& mesh, float radius, int segments) {
    Mesh subdivided = mesh;
    for (int i = 0; i < segments; ++i) {
        subdivided = subdivide_loop(subdivided);
    }
    return laplacian_smooth(subdivided, 3, radius * 2.f);
}

Mesh inset_faces(const Mesh& mesh, float amount) {
    Mesh result = mesh;
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        Vec3 v0 = mesh.vertices[mesh.indices[i]].position;
        Vec3 v1 = mesh.vertices[mesh.indices[i+1]].position;
        Vec3 v2 = mesh.vertices[mesh.indices[i+2]].position;
        Vec3 c = (v0 + v1 + v2) * (1.f/3.f);

        Vec3 i0 = v0 + (c - v0).normalized() * amount;
        Vec3 i1 = v1 + (c - v1).normalized() * amount;
        Vec3 i2 = v2 + (c - v2).normalized() * amount;
        Vec3 n = (v1 - v0).cross(v2 - v0).normalized();

        uint32_t base = static_cast<uint32_t>(result.vertices.size());
        result.vertices.push_back({i0, n});
        result.vertices.push_back({i1, n});
        result.vertices.push_back({i2, n});

        result.indices.insert(result.indices.end(),
            {mesh.indices[i], mesh.indices[i+1], base+1,
             mesh.indices[i], base+1, base,
             mesh.indices[i+1], mesh.indices[i+2], base+2,
             mesh.indices[i+1], base+2, base+1,
             mesh.indices[i+2], mesh.indices[i], base,
             mesh.indices[i+2], base, base+2,
             base, base+1, base+2});
    }
    return result;
}

Mesh extrude_faces(const Mesh& mesh, Vec3 direction, float distance) {
    Mesh result = mesh;
    Vec3 d = direction.normalized() * distance;
    uint32_t base = static_cast<uint32_t>(result.vertices.size());

    for (auto& v : mesh.vertices) {
        result.vertices.push_back({v.position + d, v.normal});
    }
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        result.indices.push_back(mesh.indices[i] + base);
        result.indices.push_back(mesh.indices[i+2] + base);
        result.indices.push_back(mesh.indices[i+1] + base);
    }

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
    for (auto& [k, count] : edge_count) {
        if (count == 1) {
            uint32_t a = static_cast<uint32_t>(k >> 32);
            uint32_t b = static_cast<uint32_t>(k & 0xFFFFFFFF);
            uint32_t a2 = a + base, b2 = b + base;
            result.indices.insert(result.indices.end(), {a, b, b2, a, b2, a2});
        }
    }
    return result;
}

}  // namespace smidr
