#pragma once

#include "core/Math.h"
#include "core/Primitive.h"

#include <vector>

namespace smidr {

// Fracture a mesh into pieces using Voronoi cells from seed points.
// Each output mesh is a fragment.
std::vector<Mesh> voronoi_fracture(const Mesh& source,
                                       const std::vector<Vec3>& seeds);

// Generate seeds inside an AABB for impact fracture.
std::vector<Vec3> generate_impact_seeds(Vec3 impact_point, AABB bounds,
                                          int count, float radius_scale = 1.f,
                                          uint32_t seed = 12345u);

// Cut a mesh by a plane into above/below halves.
struct CutResult {
    Mesh above;
    Mesh below;
};
CutResult plane_cut(const Mesh& source, Vec3 plane_normal, float plane_offset);

}  // namespace smidr
