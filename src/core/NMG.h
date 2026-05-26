#pragma once

#include "core/Math.h"
#include "core/Primitive.h"

#include <cstdint>
#include <vector>

namespace smidr {

struct NMGVertex {
    Vec3 position;
    uint32_t id = 0;
};

struct NMGEdge {
    uint32_t v0 = 0, v1 = 0;
    uint32_t left_face = UINT32_MAX;
    uint32_t right_face = UINT32_MAX;
    uint32_t id = 0;
};

struct NMGLoop {
    std::vector<uint32_t> edge_ids;
    bool is_outer = true;
};

struct NMGFace {
    std::vector<NMGLoop> loops;
    Vec3 normal{0, 1, 0};
    uint32_t id = 0;
};

struct NMGShell {
    std::vector<NMGVertex> vertices;
    std::vector<NMGEdge>   edges;
    std::vector<NMGFace>   faces;

    uint32_t add_vertex(Vec3 pos);
    uint32_t add_edge(uint32_t v0, uint32_t v1);
    uint32_t add_face(const std::vector<uint32_t>& vert_ids);

    bool is_closed() const;
    bool is_manifold() const;
    int  euler_characteristic() const;

    Mesh to_mesh() const;

    void merge_vertices(float tolerance = 1e-5f);
    void flip_face(uint32_t face_id);
    void compute_face_normals();

    bool validate() const;

    static NMGShell from_mesh(const Mesh& mesh, float merge_tol = 1e-5f);
    static NMGShell make_box(Vec3 min_pt, Vec3 max_pt);
    static NMGShell make_tetrahedron(Vec3 a, Vec3 b, Vec3 c, Vec3 d);
};

Mesh nmg_boolean_union(const NMGShell& a, const NMGShell& b);
Mesh nmg_boolean_intersection(const NMGShell& a, const NMGShell& b);
Mesh nmg_boolean_difference(const NMGShell& a, const NMGShell& b);

}  // namespace smidr
