#include "core/BezierCurve.h"

#include <cmath>

#include "io/json.hpp"

namespace smidr {

BezierCurvePrimitive::BezierCurvePrimitive() {
    control_points = {{-1, 0, 0}, {-0.5f, 1.f, 0}, {0.5f, 1.f, 0}, {1, 0, 0}};
}

static int factorial(int n) { int r = 1; for (int i = 2; i <= n; ++i) r *= i; return r; }

static float bernstein(int i, int n, float t) {
    int c = factorial(n) / (factorial(i) * factorial(n - i));
    return static_cast<float>(c) * std::pow(t, i) * std::pow(1.f - t, n - i);
}

Vec3 BezierCurvePrimitive::evaluate(float t) const {
    if (control_points.empty()) return {};
    int n = static_cast<int>(control_points.size()) - 1;
    Vec3 p{0, 0, 0};
    for (int i = 0; i <= n; ++i) {
        p += control_points[i] * bernstein(i, n, t);
    }
    return p;
}

Vec3 BezierCurvePrimitive::tangent(float t) const {
    float h = 0.001f;
    return (evaluate(std::min(1.f, t + h)) - evaluate(std::max(0.f, t - h))).normalized();
}

std::vector<Vec3> BezierCurvePrimitive::tessellate() const {
    std::vector<Vec3> pts;
    for (int i = 0; i <= tessellation; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(tessellation);
        pts.push_back(evaluate(t));
    }
    if (closed && !pts.empty()) pts.push_back(pts.front());
    return pts;
}

Mesh BezierCurvePrimitive::generate_mesh(int) const {
    Mesh mesh;
    auto pts = tessellate();
    if (pts.size() < 2) return mesh;

    int ring_segs = 8;
    auto ring = [&](Vec3 center, Vec3 forward) {
        Vec3 up = std::abs(forward.y) < 0.9f ? forward.cross({0,1,0}).normalized()
                                              : forward.cross({1,0,0}).normalized();
        Vec3 right = forward.cross(up).normalized();
        uint32_t start = static_cast<uint32_t>(mesh.vertices.size());
        for (int i = 0; i <= ring_segs; ++i) {
            float a = kTwoPi * static_cast<float>(i) / static_cast<float>(ring_segs);
            Vec3 n = (up * std::cos(a) + right * std::sin(a));
            mesh.vertices.push_back({center + n * tube_radius, n});
        }
        return start;
    };

    std::vector<uint32_t> ring_starts;
    for (size_t i = 0; i < pts.size(); ++i) {
        Vec3 fwd;
        if (i + 1 < pts.size()) fwd = (pts[i+1] - pts[i]).normalized();
        else fwd = (pts[i] - pts[i-1]).normalized();
        ring_starts.push_back(ring(pts[i], fwd));
    }
    for (size_t i = 0; i + 1 < ring_starts.size(); ++i) {
        uint32_t a = ring_starts[i], b = ring_starts[i+1];
        for (int j = 0; j < ring_segs; ++j) {
            mesh.indices.insert(mesh.indices.end(), {
                a + static_cast<uint32_t>(j), b + static_cast<uint32_t>(j),
                a + static_cast<uint32_t>(j+1),
                a + static_cast<uint32_t>(j+1), b + static_cast<uint32_t>(j),
                b + static_cast<uint32_t>(j+1)
            });
        }
    }
    return mesh;
}

AABB BezierCurvePrimitive::local_bounds() const {
    AABB bb;
    for (auto& cp : control_points) bb.expand(cp);
    bb.min_pt = bb.min_pt - Vec3{tube_radius, tube_radius, tube_radius};
    bb.max_pt = bb.max_pt + Vec3{tube_radius, tube_radius, tube_radius};
    return bb;
}

std::unique_ptr<Primitive> BezierCurvePrimitive::clone() const {
    return std::make_unique<BezierCurvePrimitive>(*this);
}

void BezierCurvePrimitive::to_json(nlohmann::json& j) const {
    j["type"] = "sketch";
    j["bezier_degree"] = degree;
    j["tube_radius"] = tube_radius;
    auto& arr = j["control_points"];
    arr = nlohmann::json::array();
    for (auto& p : control_points) arr.push_back({p.x, p.y, p.z});
}

LoftPrimitive::LoftPrimitive() {
    BezierCurvePrimitive p1, p2;
    p1.control_points = {{-1, 0, 0}, {-0.5f, 0, 1}, {0.5f, 0, 1}, {1, 0, 0}};
    p2.control_points = {{-1, 2, 0}, {-0.7f, 2, 0.7f}, {0.7f, 2, 0.7f}, {1, 2, 0}};
    profiles = {p1, p2};
}

Mesh LoftPrimitive::generate_mesh(int) const {
    Mesh mesh;
    if (profiles.size() < 2) return mesh;

    std::vector<std::vector<Vec3>> rings;
    for (auto& p : profiles) {
        auto pts = p.tessellate();
        rings.push_back(pts);
    }
    if (rings.empty() || rings[0].empty()) return mesh;
    size_t n = rings[0].size();

    for (auto& ring : rings) {
        for (size_t i = 0; i < n && i < ring.size(); ++i) {
            mesh.vertices.push_back({ring[i], {0, 1, 0}});
        }
    }
    for (size_t r = 0; r + 1 < rings.size(); ++r) {
        for (size_t i = 0; i + 1 < n; ++i) {
            uint32_t a = static_cast<uint32_t>(r * n + i);
            uint32_t b = static_cast<uint32_t>((r + 1) * n + i);
            mesh.indices.insert(mesh.indices.end(), {a, b, a+1, a+1, b, b+1});
        }
    }
    return mesh;
}

AABB LoftPrimitive::local_bounds() const {
    AABB bb;
    for (auto& p : profiles) bb.merge(p.local_bounds());
    return bb;
}

std::unique_ptr<Primitive> LoftPrimitive::clone() const {
    return std::make_unique<LoftPrimitive>(*this);
}

void LoftPrimitive::to_json(nlohmann::json& j) const {
    j["type"] = "bot";
    j["note"] = "Loft primitive";
}

}  // namespace smidr
