#include "core/NURBS.h"

#include <algorithm>
#include <cmath>

#include "io/json.hpp"

namespace smidr {

float NURBSCurve::basis(int i, int p, float u) const {
    if (p == 0)
        return (u >= knots[i] && u < knots[i + 1]) ? 1.f : 0.f;

    float left = 0.f, right = 0.f;
    float denom_left = knots[i + p] - knots[i];
    float denom_right = knots[i + p + 1] - knots[i + 1];

    if (std::abs(denom_left) > 1e-10f)
        left = (u - knots[i]) / denom_left * basis(i, p - 1, u);
    if (std::abs(denom_right) > 1e-10f)
        right = (knots[i + p + 1] - u) / denom_right * basis(i + 1, p - 1, u);

    return left + right;
}

Vec3 NURBSCurve::evaluate(float u) const {
    if (control_points.empty()) return {};
    int n = static_cast<int>(control_points.size());

    u = std::max(knots.front(), std::min(u, knots.back() - 1e-6f));

    Vec3 num{0, 0, 0};
    float denom = 0.f;
    for (int i = 0; i < n; ++i) {
        float w = (i < static_cast<int>(weights.size())) ? weights[i] : 1.f;
        float b = basis(i, degree, u) * w;
        num += control_points[i] * b;
        denom += b;
    }
    return denom > 1e-10f ? num * (1.f / denom) : control_points[0];
}

Vec3 NURBSCurve::tangent(float u) const {
    float du = 0.001f;
    Vec3 a = evaluate(u - du);
    Vec3 b = evaluate(u + du);
    return (b - a).normalized();
}

std::vector<Vec3> NURBSCurve::tessellate(int segments) const {
    std::vector<Vec3> pts;
    if (knots.empty()) return pts;
    float u_min = knots[degree], u_max = knots[knots.size() - degree - 1];
    for (int i = 0; i <= segments; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(segments);
        float u = u_min + t * (u_max - u_min);
        pts.push_back(evaluate(u));
    }
    return pts;
}

NURBSCurve NURBSCurve::make_line(Vec3 a, Vec3 b) {
    NURBSCurve c;
    c.degree = 1;
    c.control_points = {a, b};
    c.weights = {1.f, 1.f};
    c.knots = {0, 0, 1, 1};
    return c;
}

NURBSCurve NURBSCurve::make_circle(float radius, Vec3 center) {
    NURBSCurve c;
    c.degree = 2;
    float w = std::cos(kPi / 4.f);
    c.control_points = {
        center + Vec3{radius, 0, 0},
        center + Vec3{radius, radius, 0},
        center + Vec3{0, radius, 0},
        center + Vec3{-radius, radius, 0},
        center + Vec3{-radius, 0, 0},
        center + Vec3{-radius, -radius, 0},
        center + Vec3{0, -radius, 0},
        center + Vec3{radius, -radius, 0},
        center + Vec3{radius, 0, 0},
    };
    c.weights = {1, w, 1, w, 1, w, 1, w, 1};
    c.knots = {0, 0, 0, 0.25f, 0.25f, 0.5f, 0.5f, 0.75f, 0.75f, 1, 1, 1};
    return c;
}

NURBSCurve NURBSCurve::make_arc(Vec3 center, float radius, float start, float end) {
    NURBSCurve c;
    c.degree = 2;
    float mid = (start + end) * 0.5f;
    float w = std::cos((end - start) * 0.5f);
    c.control_points = {
        center + Vec3{std::cos(start), std::sin(start), 0} * radius,
        center + Vec3{std::cos(mid), std::sin(mid), 0} * (radius / w),
        center + Vec3{std::cos(end), std::sin(end), 0} * radius,
    };
    c.weights = {1, w, 1};
    c.knots = {0, 0, 0, 1, 1, 1};
    return c;
}

float NURBSSurface::basis_u(int i, int p, float u) const {
    if (p == 0) return (u >= knots_u[i] && u < knots_u[i+1]) ? 1.f : 0.f;
    float left = 0.f, right = 0.f;
    float dl = knots_u[i+p] - knots_u[i];
    float dr = knots_u[i+p+1] - knots_u[i+1];
    if (std::abs(dl) > 1e-10f) left = (u - knots_u[i]) / dl * basis_u(i, p-1, u);
    if (std::abs(dr) > 1e-10f) right = (knots_u[i+p+1] - u) / dr * basis_u(i+1, p-1, u);
    return left + right;
}

float NURBSSurface::basis_v(int i, int p, float v) const {
    if (p == 0) return (v >= knots_v[i] && v < knots_v[i+1]) ? 1.f : 0.f;
    float left = 0.f, right = 0.f;
    float dl = knots_v[i+p] - knots_v[i];
    float dr = knots_v[i+p+1] - knots_v[i+1];
    if (std::abs(dl) > 1e-10f) left = (v - knots_v[i]) / dl * basis_v(i, p-1, v);
    if (std::abs(dr) > 1e-10f) right = (knots_v[i+p+1] - v) / dr * basis_v(i+1, p-1, v);
    return left + right;
}

Vec3 NURBSSurface::evaluate(float u, float v) const {
    u = std::max(knots_u.front(), std::min(u, knots_u.back() - 1e-6f));
    v = std::max(knots_v.front(), std::min(v, knots_v.back() - 1e-6f));

    Vec3 num{0,0,0};
    float denom = 0.f;
    for (int j = 0; j < num_v; ++j) {
        float bv = basis_v(j, degree_v, v);
        for (int i = 0; i < num_u; ++i) {
            int idx = j * num_u + i;
            float w = (idx < static_cast<int>(weights.size())) ? weights[idx] : 1.f;
            float bu = basis_u(i, degree_u, u);
            float bw = bu * bv * w;
            num += control_points[idx] * bw;
            denom += bw;
        }
    }
    return denom > 1e-10f ? num * (1.f / denom) : Vec3{0,0,0};
}

Vec3 NURBSSurface::normal(float u, float v) const {
    float du = 0.001f, dv = 0.001f;
    Vec3 pu = evaluate(u + du, v) - evaluate(u - du, v);
    Vec3 pv = evaluate(u, v + dv) - evaluate(u, v - dv);
    return pu.cross(pv).normalized();
}

Mesh NURBSSurface::tessellate(int res_u, int res_v) const {
    Mesh mesh;
    if (knots_u.empty() || knots_v.empty()) return mesh;

    float u_min = knots_u[degree_u], u_max = knots_u[knots_u.size() - degree_u - 1];
    float v_min = knots_v[degree_v], v_max = knots_v[knots_v.size() - degree_v - 1];

    for (int j = 0; j <= res_v; ++j) {
        float v = v_min + static_cast<float>(j) / static_cast<float>(res_v) * (v_max - v_min);
        for (int i = 0; i <= res_u; ++i) {
            float u = u_min + static_cast<float>(i) / static_cast<float>(res_u) * (u_max - u_min);
            mesh.vertices.push_back({evaluate(u, v), normal(u, v)});
        }
    }

    for (int j = 0; j < res_v; ++j)
        for (int i = 0; i < res_u; ++i) {
            uint32_t a = static_cast<uint32_t>(j * (res_u + 1) + i);
            uint32_t b = a + static_cast<uint32_t>(res_u + 1);
            mesh.indices.insert(mesh.indices.end(), {a, b, a+1, a+1, b, b+1});
        }
    return mesh;
}

NURBSSurface NURBSSurface::make_plane(Vec3 origin, Vec3 u_axis, Vec3 v_axis,
                                       float u_size, float v_size) {
    NURBSSurface s;
    s.degree_u = 1; s.degree_v = 1;
    s.num_u = 2; s.num_v = 2;
    s.control_points = {
        origin, origin + u_axis * u_size,
        origin + v_axis * v_size, origin + u_axis * u_size + v_axis * v_size
    };
    s.weights = {1,1,1,1};
    s.knots_u = {0,0,1,1};
    s.knots_v = {0,0,1,1};
    return s;
}

NURBSSurface NURBSSurface::make_cylinder(float radius, float height, int) {
    NURBSSurface s;
    s.degree_u = 2; s.degree_v = 1;
    float w = std::cos(kPi / 4.f);
    s.num_u = 9; s.num_v = 2;
    s.control_points.resize(18);
    s.weights.resize(18);

    Vec3 circle_pts[9] = {
        {radius,0,0}, {radius,radius,0}, {0,radius,0}, {-radius,radius,0},
        {-radius,0,0}, {-radius,-radius,0}, {0,-radius,0}, {radius,-radius,0}, {radius,0,0}
    };
    float circle_w[9] = {1,w,1,w,1,w,1,w,1};

    for (int i = 0; i < 9; ++i) {
        s.control_points[i] = circle_pts[i];
        s.control_points[i + 9] = circle_pts[i] + Vec3{0, 0, height};
        s.weights[i] = circle_w[i];
        s.weights[i + 9] = circle_w[i];
    }
    s.knots_u = {0,0,0, 0.25f,0.25f, 0.5f,0.5f, 0.75f,0.75f, 1,1,1};
    s.knots_v = {0,0,1,1};
    return s;
}

Mesh BREPFace::tessellate(int resolution) const {
    return surface.tessellate(resolution, resolution);
}

Mesh BREPSolid::tessellate(int resolution) const {
    Mesh result;
    for (auto& face : faces) {
        Mesh fm = face.tessellate(resolution);
        uint32_t offset = static_cast<uint32_t>(result.vertices.size());
        for (auto& v : fm.vertices) result.vertices.push_back(v);
        for (auto idx : fm.indices) result.indices.push_back(idx + offset);
    }
    return result;
}

AABB BREPSolid::bounds() const {
    AABB bb;
    for (auto& face : faces)
        for (auto& cp : face.surface.control_points)
            bb.expand(cp);
    return bb;
}

NURBSPrimitive::NURBSPrimitive() {
    BREPFace top;
    top.surface = NURBSSurface::make_plane({-1,1,-1}, {1,0,0}, {0,0,1}, 2.f, 2.f);
    BREPFace bottom;
    bottom.surface = NURBSSurface::make_plane({-1,0,-1}, {1,0,0}, {0,0,1}, 2.f, 2.f);
    bottom.reversed = true;
    BREPFace side;
    side.surface = NURBSSurface::make_cylinder(1.f, 1.f);
    solid.faces = {top, bottom, side};
}

Mesh NURBSPrimitive::generate_mesh(int detail) const {
    return solid.tessellate(detail);
}

AABB NURBSPrimitive::local_bounds() const {
    return solid.bounds();
}

std::unique_ptr<Primitive> NURBSPrimitive::clone() const {
    return std::make_unique<NURBSPrimitive>(*this);
}

void NURBSPrimitive::to_json(nlohmann::json& j) const {
    j["type"] = "bot";
    j["note"] = "NURBS solid (serialized as mesh)";
}

}  // namespace smidr
