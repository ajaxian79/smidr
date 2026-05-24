#include "render/Camera.h"

#include <algorithm>
#include <cmath>

namespace smidr {

Vec3 Camera::eye() const {
    float yr = yaw * kDegToRad;
    float pr = pitch * kDegToRad;
    float cp = std::cos(pr), sp = std::sin(pr);
    float cy = std::cos(yr), sy = std::sin(yr);
    return target + Vec3{cp * sy, sp, cp * cy} * distance;
}

Mat4 Camera::view_matrix() const {
    return Mat4::look_at(eye(), target, {0, 1, 0});
}

Mat4 Camera::projection_matrix(float aspect) const {
    return Mat4::perspective(fov * kDegToRad, aspect, near_clip, far_clip);
}

void Camera::orbit(float dx, float dy) {
    yaw   += dx * 0.3f;
    pitch += dy * 0.3f;
    pitch  = std::clamp(pitch, -89.f, 89.f);
}

void Camera::pan(float dx, float dy) {
    float yr = yaw * kDegToRad;
    Vec3 right{std::cos(yr), 0, -std::sin(yr)};
    Vec3 up{0, 1, 0};
    float scale = distance * 0.002f;
    target = target + right * (-dx * scale) + up * (dy * scale);
}

void Camera::zoom(float delta) {
    distance *= (1.f - delta * 0.1f);
    distance = std::clamp(distance, 0.1f, 500.f);
}

void Camera::frame(const AABB& bounds) {
    target = bounds.center();
    Vec3 ext = bounds.extent();
    float max_ext = std::max({ext.x, ext.y, ext.z});
    distance = max_ext * 3.f;
    if (distance < 0.5f) distance = 5.f;
}

}  // namespace smidr
