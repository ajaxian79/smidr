#pragma once

#include "core/Math.h"
#include "core/Primitive.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <vector>

namespace smidr {

using SDF = std::function<float(Vec3)>;

namespace sdf {

inline SDF sphere(Vec3 c, float r) {
    return [c, r](Vec3 p) { return (p - c).length() - r; };
}

inline SDF box(Vec3 c, Vec3 he) {
    return [c, he](Vec3 p) {
        Vec3 q{std::abs(p.x - c.x) - he.x,
                std::abs(p.y - c.y) - he.y,
                std::abs(p.z - c.z) - he.z};
        Vec3 q_pos{std::max(q.x, 0.f), std::max(q.y, 0.f), std::max(q.z, 0.f)};
        return q_pos.length() + std::min(std::max({q.x, q.y, q.z}), 0.f);
    };
}

inline SDF cylinder(Vec3 c, float r, float h) {
    return [c, r, h](Vec3 p) {
        Vec3 pl{p.x - c.x, p.z - c.z, 0};
        float d_xy = std::sqrt(pl.x*pl.x + pl.y*pl.y) - r;
        float d_y = std::abs(p.y - c.y) - h * 0.5f;
        float d_pos = std::sqrt(std::max(d_xy, 0.f) * std::max(d_xy, 0.f)
                                 + std::max(d_y, 0.f) * std::max(d_y, 0.f));
        return d_pos + std::min(std::max(d_xy, d_y), 0.f);
    };
}

inline SDF torus(Vec3 c, float R, float r) {
    return [c, R, r](Vec3 p) {
        Vec3 q = p - c;
        float xz = std::sqrt(q.x*q.x + q.z*q.z) - R;
        return std::sqrt(xz*xz + q.y*q.y) - r;
    };
}

inline SDF op_union(SDF a, SDF b) {
    return [a, b](Vec3 p) { return std::min(a(p), b(p)); };
}

inline SDF op_intersect(SDF a, SDF b) {
    return [a, b](Vec3 p) { return std::max(a(p), b(p)); };
}

inline SDF op_subtract(SDF a, SDF b) {
    return [a, b](Vec3 p) { return std::max(a(p), -b(p)); };
}

inline SDF op_smooth_union(SDF a, SDF b, float k) {
    return [a, b, k](Vec3 p) {
        float da = a(p), db = b(p);
        float h = std::max(k - std::abs(da - db), 0.f) / k;
        return std::min(da, db) - h * h * k * 0.25f;
    };
}

inline SDF op_smooth_subtract(SDF a, SDF b, float k) {
    return [a, b, k](Vec3 p) {
        float da = a(p), db = -b(p);
        float h = std::max(k - std::abs(da - db), 0.f) / k;
        return std::max(da, db) + h * h * k * 0.25f;
    };
}

}  // namespace sdf

Mesh march_cubes(const SDF& field, AABB bounds, int resolution = 64, float iso = 0.f);

class SDFPrimitive : public Primitive {
public:
    SDF field;
    AABB bounds{{-1, -1, -1}, {1, 1, 1}};
    int  resolution = 48;

    SDFPrimitive();

    PrimitiveType type() const override { return PrimitiveType::Bot; }
    Mesh generate_mesh(int detail = 32) const override;
    AABB local_bounds() const override { return bounds; }
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

}  // namespace smidr
