#pragma once

#include "core/Math.h"
#include "core/Primitive.h"

#include <vector>

namespace smidr {

Mesh ball_pivoting(const std::vector<Vec3>& points, float ball_radius);

Mesh alpha_shape(const std::vector<Vec3>& points, float alpha);

Mesh poisson_indicator_marching(const std::vector<Vec3>& points,
                                  const std::vector<Vec3>& normals,
                                  int resolution = 48);

std::vector<Vec3> estimate_normals_from_neighbors(const std::vector<Vec3>& points,
                                                    int k = 10);

}  // namespace smidr
