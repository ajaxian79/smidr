#include "core/RayCast.h"

#include <cmath>
#include <limits>

namespace smidr {

static Mat4 invert_simple(const Mat4& m) {
    Mat4 inv;
    float det;
    const float* s = m.data();
    float* d = inv.m.data();

    d[0]  =  s[5]*s[10]*s[15] - s[5]*s[11]*s[14] - s[9]*s[6]*s[15] + s[9]*s[7]*s[14] + s[13]*s[6]*s[11] - s[13]*s[7]*s[10];
    d[4]  = -s[4]*s[10]*s[15] + s[4]*s[11]*s[14] + s[8]*s[6]*s[15] - s[8]*s[7]*s[14] - s[12]*s[6]*s[11] + s[12]*s[7]*s[10];
    d[8]  =  s[4]*s[9]*s[15]  - s[4]*s[11]*s[13] - s[8]*s[5]*s[15] + s[8]*s[7]*s[13] + s[12]*s[5]*s[11] - s[12]*s[7]*s[9];
    d[12] = -s[4]*s[9]*s[14]  + s[4]*s[10]*s[13] + s[8]*s[5]*s[14] - s[8]*s[6]*s[13] - s[12]*s[5]*s[10] + s[12]*s[6]*s[9];

    d[1]  = -s[1]*s[10]*s[15] + s[1]*s[11]*s[14] + s[9]*s[2]*s[15] - s[9]*s[3]*s[14] - s[13]*s[2]*s[11] + s[13]*s[3]*s[10];
    d[5]  =  s[0]*s[10]*s[15] - s[0]*s[11]*s[14] - s[8]*s[2]*s[15] + s[8]*s[3]*s[14] + s[12]*s[2]*s[11] - s[12]*s[3]*s[10];
    d[9]  = -s[0]*s[9]*s[15]  + s[0]*s[11]*s[13] + s[8]*s[1]*s[15] - s[8]*s[3]*s[13] - s[12]*s[1]*s[11] + s[12]*s[3]*s[9];
    d[13] =  s[0]*s[9]*s[14]  - s[0]*s[10]*s[13] - s[8]*s[1]*s[14] + s[8]*s[2]*s[13] + s[12]*s[1]*s[10] - s[12]*s[2]*s[9];

    d[2]  =  s[1]*s[6]*s[15]  - s[1]*s[7]*s[14]  - s[5]*s[2]*s[15] + s[5]*s[3]*s[14] + s[13]*s[2]*s[7]  - s[13]*s[3]*s[6];
    d[6]  = -s[0]*s[6]*s[15]  + s[0]*s[7]*s[14]  + s[4]*s[2]*s[15] - s[4]*s[3]*s[14] - s[12]*s[2]*s[7]  + s[12]*s[3]*s[6];
    d[10] =  s[0]*s[5]*s[15]  - s[0]*s[7]*s[13]  - s[4]*s[1]*s[15] + s[4]*s[3]*s[13] + s[12]*s[1]*s[7]  - s[12]*s[3]*s[5];
    d[14] = -s[0]*s[5]*s[14]  + s[0]*s[6]*s[13]  + s[4]*s[1]*s[14] - s[4]*s[2]*s[13] - s[12]*s[1]*s[6]  + s[12]*s[2]*s[5];

    d[3]  = -s[1]*s[6]*s[11]  + s[1]*s[7]*s[10]  + s[5]*s[2]*s[11] - s[5]*s[3]*s[10] - s[9]*s[2]*s[7]   + s[9]*s[3]*s[6];
    d[7]  =  s[0]*s[6]*s[11]  - s[0]*s[7]*s[10]  - s[4]*s[2]*s[11] + s[4]*s[3]*s[10] + s[8]*s[2]*s[7]   - s[8]*s[3]*s[6];
    d[11] = -s[0]*s[5]*s[11]  + s[0]*s[7]*s[9]   + s[4]*s[1]*s[11] - s[4]*s[3]*s[9]  - s[8]*s[1]*s[7]   + s[8]*s[3]*s[5];
    d[15] =  s[0]*s[5]*s[10]  - s[0]*s[6]*s[9]   - s[4]*s[1]*s[10] + s[4]*s[2]*s[9]  + s[8]*s[1]*s[6]   - s[8]*s[2]*s[5];

    det = s[0]*d[0] + s[1]*d[4] + s[2]*d[8] + s[3]*d[12];
    if (std::abs(det) < 1e-12f) return Mat4::identity();

    float inv_det = 1.f / det;
    for (int i = 0; i < 16; ++i) d[i] *= inv_det;
    return inv;
}

Ray screen_to_ray(float sx, float sy, float vw, float vh,
                  const Mat4& view, const Mat4& proj) {
    float nx = (2.f * sx / vw) - 1.f;
    float ny = 1.f - (2.f * sy / vh);

    Mat4 inv_proj = invert_simple(proj);
    Mat4 inv_view = invert_simple(view);

    Vec3 near_ndc{nx, ny, -1.f};
    Vec3 far_ndc{nx, ny, 1.f};

    Vec3 near_eye = inv_proj.transform_point(near_ndc);
    Vec3 far_eye  = inv_proj.transform_point(far_ndc);

    Vec3 near_world = inv_view.transform_point(near_eye);
    Vec3 far_world  = inv_view.transform_point(far_eye);

    return {near_world, (far_world - near_world).normalized()};
}

float ray_triangle(const Ray& ray, Vec3 v0, Vec3 v1, Vec3 v2) {
    Vec3 e1 = v1 - v0;
    Vec3 e2 = v2 - v0;
    Vec3 h = ray.direction.cross(e2);
    float a = e1.dot(h);
    if (std::abs(a) < 1e-8f) return -1.f;

    float f = 1.f / a;
    Vec3 s = ray.origin - v0;
    float u = f * s.dot(h);
    if (u < 0.f || u > 1.f) return -1.f;

    Vec3 q = s.cross(e1);
    float v = f * ray.direction.dot(q);
    if (v < 0.f || u + v > 1.f) return -1.f;

    float t = f * e2.dot(q);
    return t > 1e-6f ? t : -1.f;
}

std::optional<HitResult> ray_cast(const Ray& ray,
                                   const MeshGenerator& meshes) {
    float best_t = std::numeric_limits<float>::max();
    HitResult best{};
    bool found = false;

    for (auto& entry : meshes.entries()) {
        auto& m = entry.mesh;
        for (size_t i = 0; i + 2 < m.indices.size(); i += 3) {
            Vec3 v0 = entry.transform.transform_point(m.vertices[m.indices[i]].position);
            Vec3 v1 = entry.transform.transform_point(m.vertices[m.indices[i+1]].position);
            Vec3 v2 = entry.transform.transform_point(m.vertices[m.indices[i+2]].position);

            float t = ray_triangle(ray, v0, v1, v2);
            if (t > 0.f && t < best_t) {
                best_t = t;
                Vec3 n = (v1 - v0).cross(v2 - v0).normalized();
                best = {entry.node_id, t, ray.origin + ray.direction * t, n};
                found = true;
            }
        }
    }

    if (found) return best;
    return std::nullopt;
}

}  // namespace smidr
