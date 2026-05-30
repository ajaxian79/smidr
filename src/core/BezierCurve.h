#pragma once

#include "core/Math.h"
#include "core/Primitive.h"

#include <vector>

namespace smidr {

class BezierCurvePrimitive : public Primitive {
public:
    std::vector<Vec3> control_points;
    int degree = 3;
    int tessellation = 32;
    float tube_radius = 0.02f;
    bool closed = false;

    BezierCurvePrimitive();

    PrimitiveType type() const override { return PrimitiveType::Sketch; }
    Mesh generate_mesh(int detail = 32) const override;
    AABB local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;

    Vec3 evaluate(float t) const;
    Vec3 tangent(float t) const;
    std::vector<Vec3> tessellate() const;
};

class LoftPrimitive : public Primitive {
public:
    std::vector<BezierCurvePrimitive> profiles;
    int profile_segments = 32;

    LoftPrimitive();

    PrimitiveType type() const override { return PrimitiveType::Bot; }
    Mesh generate_mesh(int detail = 32) const override;
    AABB local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

}  // namespace smidr
