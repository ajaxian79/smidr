#pragma once

#include "core/Math.h"
#include "core/Primitive.h"

#include <vector>

namespace smidr {

Mesh convex_hull_3d(const std::vector<Vec3>& points);

struct DelaunayResult {
    std::vector<Vec3> vertices;
    std::vector<uint32_t> triangles;
};

DelaunayResult delaunay_2d(const std::vector<Vec3>& points);

struct VoronoiCell {
    Vec3 site;
    std::vector<Vec3> vertices;
    std::vector<int> neighbor_sites;
};

std::vector<VoronoiCell> voronoi_2d(const std::vector<Vec3>& points,
                                       float bound_min_x, float bound_max_x,
                                       float bound_min_y, float bound_max_y);

}  // namespace smidr
