#pragma once

#include "core/Math.h"
#include "core/Primitive.h"

#include <vector>

namespace smidr {

struct NURBSCurve {
    int degree = 3;
    std::vector<Vec3>  control_points;
    std::vector<float> weights;
    std::vector<float> knots;

    Vec3 evaluate(float u) const;
    Vec3 tangent(float u) const;
    std::vector<Vec3> tessellate(int segments) const;

    static NURBSCurve make_circle(float radius, Vec3 center = {0,0,0});
    static NURBSCurve make_line(Vec3 a, Vec3 b);
    static NURBSCurve make_arc(Vec3 center, float radius, float start_angle, float end_angle);

private:
    float basis(int i, int p, float u) const;
};

struct NURBSSurface {
    int degree_u = 3, degree_v = 3;
    int num_u = 0, num_v = 0;
    std::vector<Vec3>  control_points;
    std::vector<float> weights;
    std::vector<float> knots_u, knots_v;

    Vec3 evaluate(float u, float v) const;
    Vec3 normal(float u, float v) const;
    Mesh tessellate(int res_u, int res_v) const;

    static NURBSSurface make_plane(Vec3 origin, Vec3 u_axis, Vec3 v_axis,
                                    float u_size, float v_size);
    static NURBSSurface make_cylinder(float radius, float height, int segments = 9);
    static NURBSSurface make_sphere(float radius, int segments = 9);

private:
    float basis_u(int i, int p, float u) const;
    float basis_v(int i, int p, float v) const;
};

struct TrimLoop {
    std::vector<NURBSCurve> curves;
    bool is_outer = true;
};

struct BREPFace {
    NURBSSurface surface;
    std::vector<TrimLoop> trims;
    bool reversed = false;

    Mesh tessellate(int resolution = 16) const;
};

struct BREPSolid {
    std::vector<BREPFace> faces;

    Mesh tessellate(int resolution = 16) const;
    AABB bounds() const;
};

class NURBSPrimitive : public Primitive {
public:
    BREPSolid solid;

    NURBSPrimitive();

    PrimitiveType type() const override { return PrimitiveType::Bot; }
    Mesh generate_mesh(int detail = 32) const override;
    AABB local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

}  // namespace smidr
