#include "core/PrimitivesAdvanced.h"

#include <algorithm>
#include <cmath>

#include "io/json.hpp"

namespace smidr {

// ── Superellipsoid ─────────────────────────────────────────────

static float sign_pow(float base, float exp) {
    if (base == 0.f) return 0.f;
    float s = base < 0.f ? -1.f : 1.f;
    return s * std::pow(std::abs(base), exp);
}

Mesh Superellipsoid::generate_mesh(int detail) const {
    Mesh mesh;
    int stacks = detail, slices = detail * 2;
    for (int i = 0; i <= stacks; ++i) {
        float phi = kPi * static_cast<float>(i) / static_cast<float>(stacks) - kPi * 0.5f;
        float cp = std::cos(phi), sp = std::sin(phi);
        for (int j = 0; j <= slices; ++j) {
            float theta = kTwoPi * static_cast<float>(j) / static_cast<float>(slices);
            float ct = std::cos(theta), st = std::sin(theta);
            Vec3 pos{radii.x * sign_pow(cp, n1) * sign_pow(ct, n2),
                     radii.y * sign_pow(sp, n1),
                     radii.z * sign_pow(cp, n1) * sign_pow(st, n2)};
            Vec3 n{sign_pow(cp, 2.f - n1) * sign_pow(ct, 2.f - n2) / radii.x,
                   sign_pow(sp, 2.f - n1) / radii.y,
                   sign_pow(cp, 2.f - n1) * sign_pow(st, 2.f - n2) / radii.z};
            mesh.vertices.push_back({pos, n.normalized()});
        }
    }
    for (int i = 0; i < stacks; ++i)
        for (int j = 0; j < slices; ++j) {
            uint32_t a = static_cast<uint32_t>(i * (slices + 1) + j);
            uint32_t b = a + static_cast<uint32_t>(slices + 1);
            mesh.indices.insert(mesh.indices.end(), {a, b, a+1, a+1, b, b+1});
        }
    return mesh;
}

AABB Superellipsoid::local_bounds() const {
    return {{-radii.x, -radii.y, -radii.z}, {radii.x, radii.y, radii.z}};
}
std::unique_ptr<Primitive> Superellipsoid::clone() const { return std::make_unique<Superellipsoid>(*this); }
void Superellipsoid::to_json(nlohmann::json& j) const {
    j["type"] = "superellipsoid"; j["radii"] = {radii.x, radii.y, radii.z};
    j["n1"] = n1; j["n2"] = n2;
}

// ── Particle ───────────────────────────────────────────────────

Mesh Particle::generate_mesh(int detail) const {
    Mesh mesh;
    Vec3 dir = height_vec.normalized();
    float h = height_vec.length();
    Vec3 up = (std::abs(dir.y) < 0.99f) ? Vec3{0,1,0} : Vec3{1,0,0};
    Vec3 right = dir.cross(up).normalized();
    Vec3 fwd = right.cross(dir).normalized();

    auto ring = [&](Vec3 center, float r, Vec3 n_bias, int segs) {
        uint32_t start = static_cast<uint32_t>(mesh.vertices.size());
        for (int i = 0; i <= segs; ++i) {
            float theta = kTwoPi * static_cast<float>(i) / static_cast<float>(segs);
            float ct = std::cos(theta), st = std::sin(theta);
            Vec3 p = center + (right * ct + fwd * st) * r;
            Vec3 n = ((right * ct + fwd * st) + n_bias).normalized();
            mesh.vertices.push_back({p, n});
        }
        return start;
    };

    int segs = detail;
    uint32_t bot = ring(base, base_radius, {0,0,0}, segs);
    uint32_t top = ring(base + height_vec, top_radius, {0,0,0}, segs);
    for (int i = 0; i < segs; ++i) {
        uint32_t a = bot + static_cast<uint32_t>(i), b = top + static_cast<uint32_t>(i);
        mesh.indices.insert(mesh.indices.end(), {a, a+1, b, b, a+1, b+1});
    }

    auto cap = [&](Vec3 center, float r, Vec3 normal, int segs_c) {
        uint32_t ci = static_cast<uint32_t>(mesh.vertices.size());
        mesh.vertices.push_back({center, normal});
        for (int i = 0; i <= segs_c; ++i) {
            float theta = kTwoPi * static_cast<float>(i) / static_cast<float>(segs_c);
            mesh.vertices.push_back({center + (right * std::cos(theta) + fwd * std::sin(theta)) * r, normal});
        }
        for (int i = 0; i < segs_c; ++i) {
            if (normal.dot(dir) > 0)
                mesh.indices.insert(mesh.indices.end(), {ci, ci+1+static_cast<uint32_t>(i), ci+2+static_cast<uint32_t>(i)});
            else
                mesh.indices.insert(mesh.indices.end(), {ci, ci+2+static_cast<uint32_t>(i), ci+1+static_cast<uint32_t>(i)});
        }
    };
    cap(base, base_radius, -dir, segs);
    cap(base + height_vec, top_radius, dir, segs);

    return mesh;
}

AABB Particle::local_bounds() const {
    float r = std::max(base_radius, top_radius);
    AABB bb;
    bb.expand(base + Vec3{-r, -r, -r}); bb.expand(base + Vec3{r, r, r});
    Vec3 top = base + height_vec;
    bb.expand(top + Vec3{-r, -r, -r}); bb.expand(top + Vec3{r, r, r});
    return bb;
}
std::unique_ptr<Primitive> Particle::clone() const { return std::make_unique<Particle>(*this); }
void Particle::to_json(nlohmann::json& j) const {
    j["type"] = "particle";
    j["base"] = {base.x, base.y, base.z};
    j["height_vec"] = {height_vec.x, height_vec.y, height_vec.z};
    j["base_radius"] = base_radius; j["top_radius"] = top_radius;
}

// ── Arbn ───────────────────────────────────────────────────────

Arbn::Arbn() {
    planes = {
        {{0,1,0}, 1}, {{0,-1,0}, 1}, {{1,0,0}, 1},
        {{-1,0,0}, 1}, {{0,0,1}, 1}, {{0,0,-1}, 1}
    };
}

Mesh Arbn::generate_mesh(int) const {
    Mesh mesh;
    if (planes.size() < 4) return mesh;

    constexpr int grid = 6;
    float step = 2.f / grid;
    for (auto& pl : planes) {
        Vec3 t1, t2;
        if (std::abs(pl.normal.x) < 0.9f) t1 = pl.normal.cross({1,0,0}).normalized();
        else t1 = pl.normal.cross({0,1,0}).normalized();
        t2 = pl.normal.cross(t1).normalized();
        Vec3 center = pl.normal * pl.dist;

        auto base = static_cast<uint32_t>(mesh.vertices.size());
        float sz = pl.dist * 1.5f + 1.f;
        Vec3 corners[4] = {
            center + t1*sz + t2*sz, center - t1*sz + t2*sz,
            center - t1*sz - t2*sz, center + t1*sz - t2*sz
        };
        for (auto& c : corners) mesh.vertices.push_back({c, pl.normal});
        mesh.indices.insert(mesh.indices.end(), {base, base+1, base+2, base, base+2, base+3});
    }
    return mesh;
}

AABB Arbn::local_bounds() const {
    float m = 2.f;
    for (auto& p : planes) m = std::max(m, std::abs(p.dist) + 1.f);
    return {{-m,-m,-m}, {m,m,m}};
}
std::unique_ptr<Primitive> Arbn::clone() const { return std::make_unique<Arbn>(*this); }
void Arbn::to_json(nlohmann::json& j) const {
    j["type"] = "arbn";
    auto& arr = j["planes"]; arr = nlohmann::json::array();
    for (auto& p : planes) arr.push_back({{p.normal.x, p.normal.y, p.normal.z}, p.dist});
}

// ── RPC (right parabolic cylinder) ─────────────────────────────

Mesh RPC::generate_mesh(int detail) const {
    Mesh mesh;
    float half_h = height * 0.5f;
    for (int i = 0; i <= detail; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(detail);
        float x = (t * 2.f - 1.f) * half_width;
        float z = depth * (1.f - (x / half_width) * (x / half_width));
        Vec3 n{2.f * x / (half_width * half_width) * depth, 0.f, 1.f};
        mesh.vertices.push_back({{x, half_h, z}, n.normalized()});
        mesh.vertices.push_back({{x, -half_h, z}, n.normalized()});
    }
    for (int i = 0; i < detail; ++i) {
        uint32_t a = static_cast<uint32_t>(i * 2);
        mesh.indices.insert(mesh.indices.end(), {a, a+1, a+2, a+2, a+1, a+3});
    }
    auto base_v = static_cast<uint32_t>(mesh.vertices.size());
    mesh.vertices.push_back({{-half_width, half_h, 0}, {0,0,-1}});
    mesh.vertices.push_back({{half_width, half_h, 0}, {0,0,-1}});
    mesh.vertices.push_back({{half_width, -half_h, 0}, {0,0,-1}});
    mesh.vertices.push_back({{-half_width, -half_h, 0}, {0,0,-1}});
    mesh.indices.insert(mesh.indices.end(), {base_v, base_v+1, base_v+2, base_v, base_v+2, base_v+3});
    return mesh;
}
AABB RPC::local_bounds() const { return {{-half_width, -height*0.5f, 0}, {half_width, height*0.5f, depth}}; }
std::unique_ptr<Primitive> RPC::clone() const { return std::make_unique<RPC>(*this); }
void RPC::to_json(nlohmann::json& j) const {
    j["type"] = "rpc"; j["height"] = height; j["half_width"] = half_width; j["depth"] = depth;
}

// ── RHC (right hyperbolic cylinder) ────────────────────────────

Mesh RHC::generate_mesh(int detail) const {
    Mesh mesh;
    float half_h = height * 0.5f;
    for (int i = 0; i <= detail; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(detail);
        float x = (t * 2.f - 1.f) * half_width;
        float r = x / half_width;
        float z = apex_dist * (std::sqrt(1.f + r * r) - 1.f);
        Vec3 n{-r / std::sqrt(1.f + r * r) * apex_dist, 0.f, 1.f};
        mesh.vertices.push_back({{x, half_h, z}, n.normalized()});
        mesh.vertices.push_back({{x, -half_h, z}, n.normalized()});
    }
    for (int i = 0; i < detail; ++i) {
        uint32_t a = static_cast<uint32_t>(i * 2);
        mesh.indices.insert(mesh.indices.end(), {a, a+1, a+2, a+2, a+1, a+3});
    }
    return mesh;
}
AABB RHC::local_bounds() const {
    float z_max = apex_dist * (std::sqrt(2.f) - 1.f);
    return {{-half_width, -height*0.5f, 0}, {half_width, height*0.5f, z_max}};
}
std::unique_ptr<Primitive> RHC::clone() const { return std::make_unique<RHC>(*this); }
void RHC::to_json(nlohmann::json& j) const {
    j["type"] = "rhc"; j["height"] = height; j["half_width"] = half_width; j["apex_dist"] = apex_dist;
}

// ── EPA (elliptic paraboloid) ──────────────────────────────────

Mesh EPA::generate_mesh(int detail) const {
    Mesh mesh;
    for (int i = 0; i <= detail; ++i) {
        float v = static_cast<float>(i) / static_cast<float>(detail);
        float r = std::sqrt(v);
        float y = v * height;
        for (int j = 0; j <= detail * 2; ++j) {
            float theta = kTwoPi * static_cast<float>(j) / static_cast<float>(detail * 2);
            float ct = std::cos(theta), st = std::sin(theta);
            float x = r * semi_major * ct;
            float z = r * semi_minor * st;
            Vec3 n{-2.f * x / (semi_major * semi_major), 1.f, -2.f * z / (semi_minor * semi_minor)};
            mesh.vertices.push_back({{x, y, z}, n.normalized()});
        }
    }
    int slices = detail * 2;
    for (int i = 0; i < detail; ++i)
        for (int j = 0; j < slices; ++j) {
            uint32_t a = static_cast<uint32_t>(i * (slices + 1) + j);
            uint32_t b = a + static_cast<uint32_t>(slices + 1);
            mesh.indices.insert(mesh.indices.end(), {a, b, a+1, a+1, b, b+1});
        }
    return mesh;
}
AABB EPA::local_bounds() const { return {{-semi_major, 0, -semi_minor}, {semi_major, height, semi_minor}}; }
std::unique_ptr<Primitive> EPA::clone() const { return std::make_unique<EPA>(*this); }
void EPA::to_json(nlohmann::json& j) const {
    j["type"] = "epa"; j["height"] = height; j["semi_major"] = semi_major; j["semi_minor"] = semi_minor;
}

// ── EHY (elliptic hyperboloid) ─────────────────────────────────

Mesh EHY::generate_mesh(int detail) const {
    Mesh mesh;
    for (int i = 0; i <= detail; ++i) {
        float v = static_cast<float>(i) / static_cast<float>(detail);
        float y = v * height;
        float scale = std::sqrt(1.f + (y / apex_dist) * (y / apex_dist));
        for (int j = 0; j <= detail * 2; ++j) {
            float theta = kTwoPi * static_cast<float>(j) / static_cast<float>(detail * 2);
            float x = scale * semi_major * std::cos(theta);
            float z = scale * semi_minor * std::sin(theta);
            Vec3 n{x / (semi_major * semi_major * scale), 0, z / (semi_minor * semi_minor * scale)};
            mesh.vertices.push_back({{x, y, z}, n.normalized()});
        }
    }
    int slices = detail * 2;
    for (int i = 0; i < detail; ++i)
        for (int j = 0; j < slices; ++j) {
            uint32_t a = static_cast<uint32_t>(i * (slices + 1) + j);
            uint32_t b = a + static_cast<uint32_t>(slices + 1);
            mesh.indices.insert(mesh.indices.end(), {a, b, a+1, a+1, b, b+1});
        }
    return mesh;
}
AABB EHY::local_bounds() const {
    float s = std::sqrt(1.f + (height/apex_dist)*(height/apex_dist));
    return {{-semi_major*s, 0, -semi_minor*s}, {semi_major*s, height, semi_minor*s}};
}
std::unique_ptr<Primitive> EHY::clone() const { return std::make_unique<EHY>(*this); }
void EHY::to_json(nlohmann::json& j) const {
    j["type"] = "ehy"; j["height"] = height; j["semi_major"] = semi_major;
    j["semi_minor"] = semi_minor; j["apex_dist"] = apex_dist;
}

// ── ETO (elliptical torus) ─────────────────────────────────────

Mesh ETO::generate_mesh(int detail) const {
    Mesh mesh;
    int rings = detail, sides = detail;
    for (int i = 0; i <= rings; ++i) {
        float u = kTwoPi * static_cast<float>(i) / static_cast<float>(rings);
        float cu = std::cos(u), su = std::sin(u);
        for (int j = 0; j <= sides; ++j) {
            float v = kTwoPi * static_cast<float>(j) / static_cast<float>(sides);
            float cv = std::cos(v), sv = std::sin(v);
            float r = major_radius + semi_major * cv;
            Vec3 pos{r * cu, semi_minor * sv, r * su};
            Vec3 n{cv * cu, sv * semi_major / semi_minor, cv * su};
            mesh.vertices.push_back({pos, n.normalized()});
        }
    }
    for (int i = 0; i < rings; ++i)
        for (int j = 0; j < sides; ++j) {
            uint32_t a = static_cast<uint32_t>(i * (sides + 1) + j);
            uint32_t b = a + static_cast<uint32_t>(sides + 1);
            mesh.indices.insert(mesh.indices.end(), {a, b, a+1, a+1, b, b+1});
        }
    return mesh;
}
AABB ETO::local_bounds() const {
    float R = major_radius + semi_major;
    return {{-R, -semi_minor, -R}, {R, semi_minor, R}};
}
std::unique_ptr<Primitive> ETO::clone() const { return std::make_unique<ETO>(*this); }
void ETO::to_json(nlohmann::json& j) const {
    j["type"] = "eto"; j["major_radius"] = major_radius;
    j["semi_major"] = semi_major; j["semi_minor"] = semi_minor;
}

// ── Hyperboloid ────────────────────────────────────────────────

Mesh Hyperboloid::generate_mesh(int detail) const {
    Mesh mesh;
    int stacks = detail, slices = detail * 2;
    for (int i = 0; i <= stacks; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(stacks);
        float y = (t - 0.5f) * height;
        float r = neck_radius + (base_radius - neck_radius) * std::abs(2.f * t - 1.f);
        for (int j = 0; j <= slices; ++j) {
            float theta = kTwoPi * static_cast<float>(j) / static_cast<float>(slices);
            float ct = std::cos(theta), st = std::sin(theta);
            float slope = (base_radius - neck_radius) * (t < 0.5f ? -1.f : 1.f) * 2.f / height;
            Vec3 n{ct, -slope, st};
            mesh.vertices.push_back({{ct * r, y, st * r}, n.normalized()});
        }
    }
    for (int i = 0; i < stacks; ++i)
        for (int j = 0; j < slices; ++j) {
            uint32_t a = static_cast<uint32_t>(i * (slices + 1) + j);
            uint32_t b = a + static_cast<uint32_t>(slices + 1);
            mesh.indices.insert(mesh.indices.end(), {a, b, a+1, a+1, b, b+1});
        }
    return mesh;
}
AABB Hyperboloid::local_bounds() const {
    return {{-base_radius, -height*0.5f, -base_radius}, {base_radius, height*0.5f, base_radius}};
}
std::unique_ptr<Primitive> Hyperboloid::clone() const { return std::make_unique<Hyperboloid>(*this); }
void Hyperboloid::to_json(nlohmann::json& j) const {
    j["type"] = "hyperboloid"; j["height"] = height;
    j["base_radius"] = base_radius; j["neck_radius"] = neck_radius;
}

// ── Bot (Bag of Triangles — BRL-CAD's mesh primitive) ──────────

Bot::Bot() {
    bot_vertices = {{-1,0,-1},{1,0,-1},{0,0,1},{0,1,0}};
    bot_faces = {0,1,2, 0,1,3, 1,2,3, 2,0,3};
}

Mesh Bot::generate_mesh(int) const {
    Mesh mesh;
    for (size_t i = 0; i + 2 < bot_faces.size(); i += 3) {
        Vec3 v0 = bot_vertices[bot_faces[i]];
        Vec3 v1 = bot_vertices[bot_faces[i+1]];
        Vec3 v2 = bot_vertices[bot_faces[i+2]];
        Vec3 n = (v1 - v0).cross(v2 - v0).normalized();
        auto base = static_cast<uint32_t>(mesh.vertices.size());
        mesh.vertices.push_back({v0, n});
        mesh.vertices.push_back({v1, n});
        mesh.vertices.push_back({v2, n});
        mesh.indices.insert(mesh.indices.end(), {base, base+1, base+2});
    }
    return mesh;
}

AABB Bot::local_bounds() const {
    AABB bb;
    for (auto& v : bot_vertices) bb.expand(v);
    return bb;
}
std::unique_ptr<Primitive> Bot::clone() const { return std::make_unique<Bot>(*this); }
void Bot::to_json(nlohmann::json& j) const {
    j["type"] = "bot";
    auto& verts = j["vertices"]; verts = nlohmann::json::array();
    for (auto& v : bot_vertices) verts.push_back({v.x, v.y, v.z});
    j["faces"] = bot_faces;
}

// ── Sketch ─────────────────────────────────────────────────────

SketchPrimitive::SketchPrimitive() {
    SketchSegment seg;
    seg.type = SketchSegment::Line;
    seg.points = {{-1,0,0},{1,0,0},{1,1,0},{-1,1,0},{-1,0,0}};
    segments.push_back(seg);
}

std::vector<Vec3> SketchPrimitive::tessellate_profile(int detail) const {
    std::vector<Vec3> pts;
    for (auto& seg : segments) {
        if (seg.type == SketchSegment::Line) {
            for (auto& p : seg.points) pts.push_back(p);
        } else if (seg.type == SketchSegment::Arc && seg.points.size() >= 3) {
            for (int i = 0; i <= detail; ++i) {
                float t = static_cast<float>(i) / static_cast<float>(detail);
                float u = 1.f - t;
                Vec3 p = seg.points[0] * (u*u) + seg.points[1] * (2*u*t) + seg.points[2] * (t*t);
                pts.push_back(p);
            }
        } else if (seg.type == SketchSegment::Bezier && seg.points.size() >= 4) {
            for (int i = 0; i <= detail; ++i) {
                float t = static_cast<float>(i) / static_cast<float>(detail);
                float u = 1.f - t;
                Vec3 p = seg.points[0]*(u*u*u) + seg.points[1]*(3*u*u*t)
                       + seg.points[2]*(3*u*t*t) + seg.points[3]*(t*t*t);
                pts.push_back(p);
            }
        }
    }
    return pts;
}

Mesh SketchPrimitive::generate_mesh(int detail) const {
    Mesh mesh;
    auto pts = tessellate_profile(detail);
    for (size_t i = 0; i + 1 < pts.size(); ++i) {
        auto base = static_cast<uint32_t>(mesh.vertices.size());
        float w = 0.02f;
        Vec3 a = pts[i], b = pts[i+1];
        Vec3 dir = (b - a).normalized();
        Vec3 up{0, 0, 1};
        Vec3 side = dir.cross(up).normalized() * w;
        mesh.vertices.push_back({a + side, up}); mesh.vertices.push_back({a - side, up});
        mesh.vertices.push_back({b + side, up}); mesh.vertices.push_back({b - side, up});
        mesh.indices.insert(mesh.indices.end(), {base, base+1, base+2, base+1, base+3, base+2});
    }
    return mesh;
}

AABB SketchPrimitive::local_bounds() const {
    AABB bb;
    for (auto& seg : segments)
        for (auto& p : seg.points) bb.expand(p);
    return bb;
}
std::unique_ptr<Primitive> SketchPrimitive::clone() const { return std::make_unique<SketchPrimitive>(*this); }
void SketchPrimitive::to_json(nlohmann::json& j) const {
    j["type"] = "sketch";
    auto& segs = j["segments"]; segs = nlohmann::json::array();
    for (auto& s : segments) {
        nlohmann::json sj;
        sj["seg_type"] = static_cast<int>(s.type);
        auto& pts = sj["points"]; pts = nlohmann::json::array();
        for (auto& p : s.points) pts.push_back({p.x, p.y, p.z});
        segs.push_back(sj);
    }
}

// ── Extrude ────────────────────────────────────────────────────

Mesh ExtrudePrimitive::generate_mesh(int detail) const {
    Mesh mesh;
    auto profile = sketch.tessellate_profile(detail);
    if (profile.size() < 2) return mesh;

    Vec3 d = direction.normalized() * depth;
    for (size_t i = 0; i + 1 < profile.size(); ++i) {
        Vec3 a = profile[i], b = profile[i + 1];
        Vec3 c = b + d, e = a + d;
        Vec3 edge = (b - a);
        Vec3 n = edge.cross(d).normalized();
        auto base = static_cast<uint32_t>(mesh.vertices.size());
        mesh.vertices.push_back({a, n}); mesh.vertices.push_back({b, n});
        mesh.vertices.push_back({c, n}); mesh.vertices.push_back({e, n});
        mesh.indices.insert(mesh.indices.end(), {base, base+1, base+2, base, base+2, base+3});
    }
    return mesh;
}

AABB ExtrudePrimitive::local_bounds() const {
    AABB bb = sketch.local_bounds();
    Vec3 d = direction.normalized() * depth;
    bb.expand(bb.min_pt + d); bb.expand(bb.max_pt + d);
    return bb;
}
std::unique_ptr<Primitive> ExtrudePrimitive::clone() const { return std::make_unique<ExtrudePrimitive>(*this); }
void ExtrudePrimitive::to_json(nlohmann::json& j) const {
    j["type"] = "extrude";
    nlohmann::json sj; sketch.to_json(sj); j["sketch"] = sj;
    j["direction"] = {direction.x, direction.y, direction.z}; j["depth"] = depth;
}

// ── Revolve ────────────────────────────────────────────────────

Mesh RevolvePrimitive::generate_mesh(int detail) const {
    Mesh mesh;
    auto profile = sketch.tessellate_profile(detail);
    if (profile.size() < 2) return mesh;

    int steps = detail;
    float angle_rad = angle * kDegToRad;
    Vec3 ax = axis.normalized();

    auto rotate_point = [&](Vec3 p, float a) -> Vec3 {
        float c = std::cos(a), s = std::sin(a);
        float d = p.dot(ax);
        Vec3 proj = ax * d;
        Vec3 perp = p - proj;
        Vec3 cross = ax.cross(perp);
        return proj + perp * c + cross * s;
    };

    for (int j = 0; j <= steps; ++j) {
        float a = angle_rad * static_cast<float>(j) / static_cast<float>(steps);
        for (auto& pt : profile)
            mesh.vertices.push_back({rotate_point(pt, a), rotate_point(pt.normalized(), a)});
    }

    int n = static_cast<int>(profile.size());
    for (int j = 0; j < steps; ++j)
        for (int i = 0; i < n - 1; ++i) {
            uint32_t a = static_cast<uint32_t>(j * n + i);
            uint32_t b = a + static_cast<uint32_t>(n);
            mesh.indices.insert(mesh.indices.end(), {a, b, a+1, a+1, b, b+1});
        }
    return mesh;
}

AABB RevolvePrimitive::local_bounds() const {
    AABB bb = sketch.local_bounds();
    float r = std::max({std::abs(bb.min_pt.x), std::abs(bb.max_pt.x),
                        std::abs(bb.min_pt.z), std::abs(bb.max_pt.z)});
    return {{-r, bb.min_pt.y, -r}, {r, bb.max_pt.y, r}};
}
std::unique_ptr<Primitive> RevolvePrimitive::clone() const { return std::make_unique<RevolvePrimitive>(*this); }
void RevolvePrimitive::to_json(nlohmann::json& j) const {
    j["type"] = "revolve";
    nlohmann::json sj; sketch.to_json(sj); j["sketch"] = sj;
    j["axis"] = {axis.x, axis.y, axis.z}; j["angle"] = angle;
}

// ── DSP (displacement map surface) ─────────────────────────────

DSPPrimitive::DSPPrimitive() {
    heightmap.resize(static_cast<size_t>(width * height_dim), 0.f);
    for (int y = 0; y < height_dim; ++y)
        for (int x = 0; x < width; ++x)
            heightmap[static_cast<size_t>(y * width + x)] =
                0.5f * std::sin(static_cast<float>(x) * 0.8f) *
                std::cos(static_cast<float>(y) * 0.8f);
}

Mesh DSPPrimitive::generate_mesh(int) const {
    Mesh mesh;
    for (int y = 0; y < height_dim; ++y)
        for (int x = 0; x < width; ++x) {
            float h = heightmap[static_cast<size_t>(y * width + x)] * height_scale;
            Vec3 pos{static_cast<float>(x) * cell_size, h, static_cast<float>(y) * cell_size};
            Vec3 n{0, 1, 0};
            if (x > 0 && x < width - 1 && y > 0 && y < height_dim - 1) {
                float hL = heightmap[static_cast<size_t>(y * width + x - 1)] * height_scale;
                float hR = heightmap[static_cast<size_t>(y * width + x + 1)] * height_scale;
                float hD = heightmap[static_cast<size_t>((y - 1) * width + x)] * height_scale;
                float hU = heightmap[static_cast<size_t>((y + 1) * width + x)] * height_scale;
                n = Vec3{hL - hR, 2.f * cell_size, hD - hU}.normalized();
            }
            mesh.vertices.push_back({pos, n});
        }
    for (int y = 0; y < height_dim - 1; ++y)
        for (int x = 0; x < width - 1; ++x) {
            uint32_t a = static_cast<uint32_t>(y * width + x);
            uint32_t b = a + static_cast<uint32_t>(width);
            mesh.indices.insert(mesh.indices.end(), {a, b, a+1, a+1, b, b+1});
        }
    return mesh;
}

AABB DSPPrimitive::local_bounds() const {
    float w = static_cast<float>(width - 1) * cell_size;
    float d = static_cast<float>(height_dim - 1) * cell_size;
    float hmin = *std::min_element(heightmap.begin(), heightmap.end()) * height_scale;
    float hmax = *std::max_element(heightmap.begin(), heightmap.end()) * height_scale;
    return {{0, hmin, 0}, {w, hmax, d}};
}
std::unique_ptr<Primitive> DSPPrimitive::clone() const { return std::make_unique<DSPPrimitive>(*this); }
void DSPPrimitive::to_json(nlohmann::json& j) const {
    j["type"] = "dsp"; j["width"] = width; j["height_dim"] = height_dim;
    j["cell_size"] = cell_size; j["height_scale"] = height_scale;
    j["heightmap"] = heightmap;
}

// ── Metaball ───────────────────────────────────────────────────

MetaballPrimitive::MetaballPrimitive() {
    controls = {{{0,0,0}, 1.f, 1.f}, {{0.8f,0,0}, 0.8f, 0.8f}};
}

Mesh MetaballPrimitive::generate_mesh(int detail) const {
    Mesh mesh;
    if (controls.empty()) return mesh;

    AABB bb;
    for (auto& c : controls) {
        bb.expand(c.position + Vec3{c.radius, c.radius, c.radius});
        bb.expand(c.position - Vec3{c.radius, c.radius, c.radius});
    }

    int grid = std::max(8, detail / 2);
    Vec3 step = {(bb.max_pt.x - bb.min_pt.x) / grid,
                 (bb.max_pt.y - bb.min_pt.y) / grid,
                 (bb.max_pt.z - bb.min_pt.z) / grid};

    auto field = [&](Vec3 p) -> float {
        float sum = 0.f;
        for (auto& c : controls) {
            Vec3 d = p - c.position;
            float r2 = d.dot(d);
            if (r2 < 1e-8f) r2 = 1e-8f;
            sum += c.strength * c.radius * c.radius / r2;
        }
        return sum;
    };

    for (int z = 0; z < grid; ++z)
        for (int y = 0; y < grid; ++y)
            for (int x = 0; x < grid; ++x) {
                Vec3 p{bb.min_pt.x + (static_cast<float>(x) + 0.5f) * step.x,
                       bb.min_pt.y + (static_cast<float>(y) + 0.5f) * step.y,
                       bb.min_pt.z + (static_cast<float>(z) + 0.5f) * step.z};
                if (field(p) >= threshold) {
                    Vec3 gx{field(p + Vec3{step.x*0.1f,0,0}) - field(p - Vec3{step.x*0.1f,0,0}), 0, 0};
                    Vec3 gy{0, field(p + Vec3{0,step.y*0.1f,0}) - field(p - Vec3{0,step.y*0.1f,0}), 0};
                    Vec3 gz{0, 0, field(p + Vec3{0,0,step.z*0.1f}) - field(p - Vec3{0,0,step.z*0.1f})};
                    Vec3 n = (gx + gy + gz).normalized();
                    auto base = static_cast<uint32_t>(mesh.vertices.size());
                    float s = step.x * 0.5f;
                    struct F { Vec3 fn; Vec3 c[4]; };
                    F faces[6] = {
                        {{0,0,1}, {{s,s,s},{-s,s,s},{-s,-s,s},{s,-s,s}}},
                        {{0,0,-1}, {{-s,s,-s},{s,s,-s},{s,-s,-s},{-s,-s,-s}}},
                        {{0,1,0}, {{s,s,-s},{-s,s,-s},{-s,s,s},{s,s,s}}},
                        {{0,-1,0}, {{s,-s,s},{-s,-s,s},{-s,-s,-s},{s,-s,-s}}},
                        {{1,0,0}, {{s,s,-s},{s,s,s},{s,-s,s},{s,-s,-s}}},
                        {{-1,0,0}, {{-s,s,s},{-s,s,-s},{-s,-s,-s},{-s,-s,s}}},
                    };
                    for (auto& f : faces) {
                        auto fb = static_cast<uint32_t>(mesh.vertices.size());
                        for (auto& c : f.c) mesh.vertices.push_back({p + c, n});
                        mesh.indices.insert(mesh.indices.end(), {fb, fb+1, fb+2, fb, fb+2, fb+3});
                    }
                }
            }
    return mesh;
}

AABB MetaballPrimitive::local_bounds() const {
    AABB bb;
    for (auto& c : controls) {
        bb.expand(c.position + Vec3{c.radius, c.radius, c.radius});
        bb.expand(c.position - Vec3{c.radius, c.radius, c.radius});
    }
    return bb;
}
std::unique_ptr<Primitive> MetaballPrimitive::clone() const { return std::make_unique<MetaballPrimitive>(*this); }
void MetaballPrimitive::to_json(nlohmann::json& j) const {
    j["type"] = "metaball"; j["threshold"] = threshold;
    auto& arr = j["controls"]; arr = nlohmann::json::array();
    for (auto& c : controls)
        arr.push_back({{"position", {c.position.x, c.position.y, c.position.z}},
                       {"strength", c.strength}, {"radius", c.radius}});
}

// ── Heart ──────────────────────────────────────────────────────

Mesh HeartPrimitive::generate_mesh(int detail) const {
    Mesh mesh;
    int stacks = detail, slices = detail * 2;
    for (int i = 0; i <= stacks; ++i) {
        float v = static_cast<float>(i) / static_cast<float>(stacks);
        float phi = kPi * v;
        for (int j = 0; j <= slices; ++j) {
            float u = static_cast<float>(j) / static_cast<float>(slices);
            float theta = kTwoPi * u;
            float ct = std::cos(theta), st = std::sin(theta);
            float sp = std::sin(phi), cp = std::cos(phi);
            float r = scale * (1.f - 0.4f * cp * cp) * sp;
            float x = r * st;
            float y = scale * (cp + 0.3f * sp * sp - 0.3f);
            float z = r * ct;
            mesh.vertices.push_back({{x, y, z}, Vec3{x, y, z}.normalized()});
        }
    }
    for (int i = 0; i < stacks; ++i)
        for (int j = 0; j < slices; ++j) {
            uint32_t a = static_cast<uint32_t>(i * (slices + 1) + j);
            uint32_t b = a + static_cast<uint32_t>(slices + 1);
            mesh.indices.insert(mesh.indices.end(), {a, b, a+1, a+1, b, b+1});
        }
    return mesh;
}

AABB HeartPrimitive::local_bounds() const {
    return {{-scale, -scale, -scale}, {scale, scale, scale}};
}
std::unique_ptr<Primitive> HeartPrimitive::clone() const { return std::make_unique<HeartPrimitive>(*this); }
void HeartPrimitive::to_json(nlohmann::json& j) const {
    j["type"] = "heart"; j["scale"] = scale;
}

// ── PointCloud ─────────────────────────────────────────────────

PointCloudPrimitive::PointCloudPrimitive() {
    for (int i = 0; i < 50; ++i)
        points.push_back({static_cast<float>(i % 5) * 0.4f - 1.f,
                           static_cast<float>((i / 5) % 5) * 0.4f - 1.f,
                           static_cast<float>(i / 25) * 0.4f - 0.4f});
}

Mesh PointCloudPrimitive::generate_mesh(int) const {
    Mesh mesh;
    for (auto& p : points) {
        auto base = static_cast<uint32_t>(mesh.vertices.size());
        float s = point_size;
        Vec3 verts[8] = {
            {p.x-s,p.y-s,p.z-s},{p.x+s,p.y-s,p.z-s},{p.x+s,p.y+s,p.z-s},{p.x-s,p.y+s,p.z-s},
            {p.x-s,p.y-s,p.z+s},{p.x+s,p.y-s,p.z+s},{p.x+s,p.y+s,p.z+s},{p.x-s,p.y+s,p.z+s},
        };
        struct F { int i[4]; Vec3 n; };
        F faces[6] = {
            {{0,3,2,1},{0,0,-1}}, {{4,5,6,7},{0,0,1}},
            {{0,1,5,4},{0,-1,0}}, {{2,3,7,6},{0,1,0}},
            {{1,2,6,5},{1,0,0}},  {{0,4,7,3},{-1,0,0}},
        };
        for (auto& f : faces) {
            auto fb = static_cast<uint32_t>(mesh.vertices.size());
            for (auto idx : f.i) mesh.vertices.push_back({verts[idx], f.n});
            mesh.indices.insert(mesh.indices.end(), {fb, fb+1, fb+2, fb, fb+2, fb+3});
        }
    }
    return mesh;
}

AABB PointCloudPrimitive::local_bounds() const {
    AABB bb;
    for (auto& p : points) bb.expand(p);
    return bb;
}
std::unique_ptr<Primitive> PointCloudPrimitive::clone() const { return std::make_unique<PointCloudPrimitive>(*this); }
void PointCloudPrimitive::to_json(nlohmann::json& j) const {
    j["type"] = "pointcloud"; j["point_size"] = point_size;
    auto& arr = j["points"]; arr = nlohmann::json::array();
    for (auto& p : points) arr.push_back({p.x, p.y, p.z});
}

// ── Annotation ─────────────────────────────────────────────────

Mesh AnnotationPrimitive::generate_mesh(int) const {
    Mesh mesh;
    Vec3 end = anchor + offset;
    Vec3 dir = offset.normalized();
    Vec3 side = (std::abs(dir.y) < 0.9f ? dir.cross({0,1,0}) : dir.cross({1,0,0})).normalized() * 0.02f;

    auto base = static_cast<uint32_t>(mesh.vertices.size());
    mesh.vertices.push_back({anchor + side, dir});
    mesh.vertices.push_back({anchor - side, dir});
    mesh.vertices.push_back({end + side, dir});
    mesh.vertices.push_back({end - side, dir});
    mesh.indices.insert(mesh.indices.end(), {base, base+1, base+2, base+1, base+3, base+2});
    return mesh;
}

AABB AnnotationPrimitive::local_bounds() const {
    AABB bb;
    bb.expand(anchor); bb.expand(anchor + offset);
    return bb;
}
std::unique_ptr<Primitive> AnnotationPrimitive::clone() const { return std::make_unique<AnnotationPrimitive>(*this); }
void AnnotationPrimitive::to_json(nlohmann::json& j) const {
    j["type"] = "annotation"; j["text"] = text;
    j["anchor"] = {anchor.x, anchor.y, anchor.z};
    j["offset"] = {offset.x, offset.y, offset.z};
}

}  // namespace smidr
