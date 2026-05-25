#pragma once

#include "core/Math.h"
#include "core/Primitive.h"
#include "core/Scene.h"
#include "core/MeshGenerator.h"

#include <optional>

namespace smidr {

struct Ray {
    Vec3 origin;
    Vec3 direction;
};

struct HitResult {
    NodeId node_id;
    float  distance;
    Vec3   point;
    Vec3   normal;
};

Ray screen_to_ray(float screen_x, float screen_y,
                  float viewport_w, float viewport_h,
                  const Mat4& view, const Mat4& proj);

std::optional<HitResult> ray_cast(const Ray& ray,
                                   const MeshGenerator& meshes);

float ray_triangle(const Ray& ray, Vec3 v0, Vec3 v1, Vec3 v2);

}  // namespace smidr
