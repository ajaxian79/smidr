#pragma once

#include "core/Math.h"
#include "core/Primitive.h"

#include <vector>

namespace smidr {

struct VertexCurvature {
    float mean = 0.f;
    float gaussian = 0.f;
    Vec3  principal_direction_1;
    Vec3  principal_direction_2;
    float k1 = 0.f, k2 = 0.f;
};

std::vector<VertexCurvature> compute_curvature(const Mesh& mesh);

Mesh colorize_curvature(const Mesh& mesh, bool gaussian_not_mean = false);

struct CurvatureStats {
    float min_mean, max_mean, avg_mean;
    float min_gauss, max_gauss, avg_gauss;
    int flat_vertices, concave_vertices, convex_vertices;
};
CurvatureStats curvature_stats(const std::vector<VertexCurvature>& curv);

}  // namespace smidr
