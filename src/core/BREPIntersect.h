#pragma once

#include "core/NURBS.h"
#include "core/Math.h"

#include <vector>

namespace smidr {

struct SurfaceIntersection {
    std::vector<Vec3> points;
    bool closed = false;
};

struct ParametricPoint {
    float u_a = 0.f, v_a = 0.f;
    float u_b = 0.f, v_b = 0.f;
    Vec3  position;
};

std::vector<ParametricPoint> find_seed_points(const NURBSSurface& a,
                                                const NURBSSurface& b,
                                                int subdivision = 16);

SurfaceIntersection march_intersection(const NURBSSurface& a,
                                         const NURBSSurface& b,
                                         ParametricPoint seed,
                                         float step = 0.05f,
                                         int max_steps = 200);

std::vector<SurfaceIntersection> intersect_surfaces(const NURBSSurface& a,
                                                      const NURBSSurface& b,
                                                      int subdivision = 16);

bool aabb_intersect(const AABB& a, const AABB& b);

AABB surface_bounds(const NURBSSurface& s);

}  // namespace smidr
