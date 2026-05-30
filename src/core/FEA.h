#pragma once

#include "core/Math.h"
#include "core/Primitive.h"

#include <vector>

namespace smidr {

struct Tetrahedron {
    Vec3 v[4];
    float volume() const;
    float aspect_ratio() const;
    Vec3  centroid() const;
};

struct TetMesh {
    std::vector<Vec3>       vertices;
    std::vector<uint32_t>   indices;     // groups of 4
    std::vector<float>      quality;     // per-tet aspect ratio

    int tet_count() const { return static_cast<int>(indices.size() / 4); }
    Tetrahedron get_tet(int i) const;
};

struct FEAQuality {
    int total_tets;
    float min_volume, max_volume, avg_volume;
    float min_aspect, max_aspect, avg_aspect;
    int  degenerate_tets;
    int  inverted_tets;
};

TetMesh tetrahedralize_aabb_grid(const Mesh& boundary, int resolution = 8);

FEAQuality compute_fea_quality(const TetMesh& tet);

struct BoundaryCondition {
    enum Kind { Fixed, Force, Displacement, Temperature };
    Kind  kind = Fixed;
    std::vector<uint32_t> vertex_ids;
    Vec3  value;
};

struct FEAProblem {
    TetMesh mesh;
    std::vector<BoundaryCondition> conditions;
    float youngs_modulus = 200e9f;
    float poissons_ratio = 0.3f;
};

}  // namespace smidr
