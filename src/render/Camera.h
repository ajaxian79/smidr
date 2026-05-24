#pragma once

#include "core/Math.h"

namespace smidr {

class Camera {
public:
    Vec3  target{0, 0, 0};
    float distance  = 8.f;
    float yaw       = 45.f;
    float pitch     = 30.f;
    float fov       = 45.f;
    float near_clip = 0.1f;
    float far_clip  = 1000.f;

    Vec3 eye() const;
    Mat4 view_matrix() const;
    Mat4 projection_matrix(float aspect) const;

    void orbit(float dx, float dy);
    void pan(float dx, float dy);
    void zoom(float delta);
    void frame(const AABB& bounds);
};

}  // namespace smidr
