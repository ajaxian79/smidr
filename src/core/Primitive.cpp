#include "core/Primitive.h"
#include "core/PrimitivesAdvanced.h"
#include "core/PrimitivesExtra.h"

#include <cmath>
#include <stdexcept>

#include "io/json.hpp"

namespace smidr {

AABB Mesh::bounds() const {
    AABB bb;
    for (auto& v : vertices) bb.expand(v.position);
    return bb;
}

const char* primitive_type_name(PrimitiveType t) {
    switch (t) {
        case PrimitiveType::Sphere:    return "sphere";
        case PrimitiveType::Box:       return "box";
        case PrimitiveType::Cylinder:  return "cylinder";
        case PrimitiveType::Cone:      return "cone";
        case PrimitiveType::Torus:     return "torus";
        case PrimitiveType::Ellipsoid: return "ellipsoid";
        case PrimitiveType::Halfspace: return "halfspace";
        case PrimitiveType::Pipe:      return "pipe";
        case PrimitiveType::Wedge:     return "wedge";
        case PrimitiveType::Arb8:           return "arb8";
        case PrimitiveType::Superellipsoid: return "superellipsoid";
        case PrimitiveType::Particle:       return "particle";
        case PrimitiveType::Arbn:           return "arbn";
        case PrimitiveType::RPC:            return "rpc";
        case PrimitiveType::RHC:            return "rhc";
        case PrimitiveType::EPA:            return "epa";
        case PrimitiveType::EHY:            return "ehy";
        case PrimitiveType::ETO:            return "eto";
        case PrimitiveType::Hyperboloid:    return "hyperboloid";
        case PrimitiveType::Bot:            return "bot";
        case PrimitiveType::Sketch:         return "sketch";
        case PrimitiveType::Extrude:        return "extrude";
        case PrimitiveType::Revolve:        return "revolve";
        case PrimitiveType::DSP:            return "dsp";
        case PrimitiveType::Metaball:       return "metaball";
        case PrimitiveType::Heart:          return "heart";
        case PrimitiveType::PointCloud:     return "pointcloud";
        case PrimitiveType::Annotation:     return "annotation";
        case PrimitiveType::CLine:          return "cline";
        case PrimitiveType::Joint:          return "joint";
        case PrimitiveType::Grip:           return "grip";
        case PrimitiveType::Datum:          return "datum";
        case PrimitiveType::Submodel:       return "submodel";
        case PrimitiveType::Script:         return "script";
        case PrimitiveType::EBM:            return "ebm";
        case PrimitiveType::VOL:            return "vol";
        case PrimitiveType::HF:             return "hf";
        case PrimitiveType::ARS:            return "ars";
    }
    return "unknown";
}

PrimitiveType primitive_type_from_name(const std::string& name) {
    if (name == "sphere")         return PrimitiveType::Sphere;
    if (name == "box")            return PrimitiveType::Box;
    if (name == "cylinder")       return PrimitiveType::Cylinder;
    if (name == "cone")           return PrimitiveType::Cone;
    if (name == "torus")          return PrimitiveType::Torus;
    if (name == "ellipsoid")      return PrimitiveType::Ellipsoid;
    if (name == "halfspace")      return PrimitiveType::Halfspace;
    if (name == "pipe")           return PrimitiveType::Pipe;
    if (name == "wedge")          return PrimitiveType::Wedge;
    if (name == "arb8")           return PrimitiveType::Arb8;
    if (name == "superellipsoid") return PrimitiveType::Superellipsoid;
    if (name == "particle")       return PrimitiveType::Particle;
    if (name == "arbn")           return PrimitiveType::Arbn;
    if (name == "rpc")            return PrimitiveType::RPC;
    if (name == "rhc")            return PrimitiveType::RHC;
    if (name == "epa")            return PrimitiveType::EPA;
    if (name == "ehy")            return PrimitiveType::EHY;
    if (name == "eto")            return PrimitiveType::ETO;
    if (name == "hyperboloid")    return PrimitiveType::Hyperboloid;
    if (name == "bot")            return PrimitiveType::Bot;
    if (name == "sketch")         return PrimitiveType::Sketch;
    if (name == "extrude")        return PrimitiveType::Extrude;
    if (name == "revolve")        return PrimitiveType::Revolve;
    if (name == "dsp")            return PrimitiveType::DSP;
    if (name == "metaball")       return PrimitiveType::Metaball;
    if (name == "heart")          return PrimitiveType::Heart;
    if (name == "pointcloud")     return PrimitiveType::PointCloud;
    if (name == "annotation")     return PrimitiveType::Annotation;
    if (name == "cline")          return PrimitiveType::CLine;
    if (name == "joint")          return PrimitiveType::Joint;
    if (name == "grip")           return PrimitiveType::Grip;
    if (name == "datum")          return PrimitiveType::Datum;
    if (name == "submodel")       return PrimitiveType::Submodel;
    if (name == "script")         return PrimitiveType::Script;
    if (name == "ebm")            return PrimitiveType::EBM;
    if (name == "vol")            return PrimitiveType::VOL;
    if (name == "hf")             return PrimitiveType::HF;
    if (name == "ars")            return PrimitiveType::ARS;
    throw std::runtime_error("Unknown primitive type: " + name);
}

std::unique_ptr<Primitive> Primitive::from_json(const nlohmann::json& j) {
    auto type = primitive_type_from_name(j.at("type").get<std::string>());
    switch (type) {
        case PrimitiveType::Sphere: {
            auto p = std::make_unique<Sphere>();
            p->radius = j.value("radius", 1.f);
            return p;
        }
        case PrimitiveType::Box: {
            auto p = std::make_unique<Box>();
            auto& he = j.at("half_extents");
            p->half_extents = {he[0].get<float>(), he[1].get<float>(), he[2].get<float>()};
            return p;
        }
        case PrimitiveType::Cylinder: {
            auto p = std::make_unique<Cylinder>();
            p->radius = j.value("radius", 0.5f);
            p->height = j.value("height", 2.f);
            return p;
        }
        case PrimitiveType::Cone: {
            auto p = std::make_unique<Cone>();
            p->radius = j.value("radius", 0.5f);
            p->height = j.value("height", 2.f);
            return p;
        }
        case PrimitiveType::Torus: {
            auto p = std::make_unique<Torus>();
            p->major_radius = j.value("major_radius", 1.f);
            p->minor_radius = j.value("minor_radius", 0.3f);
            return p;
        }
        case PrimitiveType::Ellipsoid: {
            auto p = std::make_unique<Ellipsoid>();
            if (j.contains("radii")) {
                auto& r = j["radii"];
                p->radii = {r[0].get<float>(), r[1].get<float>(), r[2].get<float>()};
            }
            return p;
        }
        case PrimitiveType::Halfspace: {
            auto p = std::make_unique<Halfspace>();
            if (j.contains("normal")) {
                auto& n = j["normal"];
                p->normal = Vec3{n[0].get<float>(), n[1].get<float>(), n[2].get<float>()}.normalized();
            }
            p->offset = j.value("offset", 0.f);
            return p;
        }
        case PrimitiveType::Pipe: {
            auto p = std::make_unique<Pipe>();
            p->inner_radius = j.value("inner_radius", 0.3f);
            p->outer_radius = j.value("outer_radius", 0.5f);
            p->height = j.value("height", 2.f);
            return p;
        }
        case PrimitiveType::Wedge: {
            auto p = std::make_unique<Wedge>();
            if (j.contains("size")) {
                auto& s = j["size"];
                p->size = {s[0].get<float>(), s[1].get<float>(), s[2].get<float>()};
            }
            p->top_width = j.value("top_width", 0.f);
            return p;
        }
        case PrimitiveType::Arb8: {
            auto p = std::make_unique<Arb8>();
            if (j.contains("verts")) {
                auto& v = j["verts"];
                for (int i = 0; i < 8 && i < static_cast<int>(v.size()); ++i)
                    p->verts[i] = {v[i][0].get<float>(), v[i][1].get<float>(), v[i][2].get<float>()};
            }
            return p;
        }
        case PrimitiveType::Superellipsoid: return std::make_unique<Superellipsoid>();
        case PrimitiveType::Particle:       return std::make_unique<Particle>();
        case PrimitiveType::Arbn:           return std::make_unique<Arbn>();
        case PrimitiveType::RPC:            return std::make_unique<RPC>();
        case PrimitiveType::RHC:            return std::make_unique<RHC>();
        case PrimitiveType::EPA:            return std::make_unique<EPA>();
        case PrimitiveType::EHY:            return std::make_unique<EHY>();
        case PrimitiveType::ETO:            return std::make_unique<ETO>();
        case PrimitiveType::Hyperboloid:    return std::make_unique<Hyperboloid>();
        case PrimitiveType::Bot:            return std::make_unique<Bot>();
        case PrimitiveType::Sketch:         return std::make_unique<SketchPrimitive>();
        case PrimitiveType::Extrude:        return std::make_unique<ExtrudePrimitive>();
        case PrimitiveType::Revolve:        return std::make_unique<RevolvePrimitive>();
        case PrimitiveType::DSP:            return std::make_unique<DSPPrimitive>();
        case PrimitiveType::Metaball:       return std::make_unique<MetaballPrimitive>();
        case PrimitiveType::Heart:          return std::make_unique<HeartPrimitive>();
        case PrimitiveType::PointCloud:     return std::make_unique<PointCloudPrimitive>();
        case PrimitiveType::Annotation:     return std::make_unique<AnnotationPrimitive>();
        case PrimitiveType::CLine:          return std::make_unique<CLinePrimitive>();
        case PrimitiveType::Joint:          return std::make_unique<JointPrimitive>();
        case PrimitiveType::Grip:           return std::make_unique<GripPrimitive>();
        case PrimitiveType::Datum:          return std::make_unique<DatumPrimitive>();
        case PrimitiveType::Submodel:       return std::make_unique<SubmodelPrimitive>();
        case PrimitiveType::Script:         return std::make_unique<ScriptPrimitive>();
        case PrimitiveType::EBM:            return std::make_unique<EBMPrimitive>();
        case PrimitiveType::VOL:            return std::make_unique<VOLPrimitive>();
        case PrimitiveType::HF:             return std::make_unique<HFPrimitive>();
        case PrimitiveType::ARS:            return std::make_unique<ARSPrimitive>();
    }
    return nullptr;
}

// ── Sphere ─────────────────────────────────────────────────────

Mesh Sphere::generate_mesh(int detail) const {
    Mesh mesh;
    int stacks = detail, slices = detail * 2;

    for (int i = 0; i <= stacks; ++i) {
        float phi = kPi * static_cast<float>(i) / static_cast<float>(stacks);
        float sp = std::sin(phi), cp = std::cos(phi);

        for (int j = 0; j <= slices; ++j) {
            float theta = kTwoPi * static_cast<float>(j) / static_cast<float>(slices);
            float st = std::sin(theta), ct = std::cos(theta);

            Vec3 n{sp * ct, cp, sp * st};
            mesh.vertices.push_back({n * radius, n});
        }
    }

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            uint32_t a = static_cast<uint32_t>(i * (slices + 1) + j);
            uint32_t b = a + static_cast<uint32_t>(slices + 1);
            mesh.indices.insert(mesh.indices.end(), {a, b, a + 1, a + 1, b, b + 1});
        }
    }
    return mesh;
}

AABB Sphere::local_bounds() const {
    return {{-radius, -radius, -radius}, {radius, radius, radius}};
}

std::unique_ptr<Primitive> Sphere::clone() const {
    return std::make_unique<Sphere>(*this);
}

void Sphere::to_json(nlohmann::json& j) const {
    j["type"] = "sphere";
    j["radius"] = radius;
}

// ── Box ────────────────────────────────────────────────────────

Mesh Box::generate_mesh(int) const {
    Mesh mesh;
    float hx = half_extents.x, hy = half_extents.y, hz = half_extents.z;

    struct Face { Vec3 n; Vec3 corners[4]; };
    Face faces[6] = {
        {{ 0, 0, 1}, {{ hx, hy, hz}, {-hx, hy, hz}, {-hx,-hy, hz}, { hx,-hy, hz}}},
        {{ 0, 0,-1}, {{-hx, hy,-hz}, { hx, hy,-hz}, { hx,-hy,-hz}, {-hx,-hy,-hz}}},
        {{ 0, 1, 0}, {{ hx, hy,-hz}, {-hx, hy,-hz}, {-hx, hy, hz}, { hx, hy, hz}}},
        {{ 0,-1, 0}, {{ hx,-hy, hz}, {-hx,-hy, hz}, {-hx,-hy,-hz}, { hx,-hy,-hz}}},
        {{ 1, 0, 0}, {{ hx, hy,-hz}, { hx, hy, hz}, { hx,-hy, hz}, { hx,-hy,-hz}}},
        {{-1, 0, 0}, {{-hx, hy, hz}, {-hx, hy,-hz}, {-hx,-hy,-hz}, {-hx,-hy, hz}}},
    };

    for (auto& f : faces) {
        auto base = static_cast<uint32_t>(mesh.vertices.size());
        for (auto& c : f.corners) mesh.vertices.push_back({c, f.n});
        mesh.indices.insert(mesh.indices.end(),
            {base, base + 1, base + 2, base, base + 2, base + 3});
    }
    return mesh;
}

AABB Box::local_bounds() const {
    return {-half_extents, half_extents};
}

std::unique_ptr<Primitive> Box::clone() const {
    return std::make_unique<Box>(*this);
}

void Box::to_json(nlohmann::json& j) const {
    j["type"] = "box";
    j["half_extents"] = {half_extents.x, half_extents.y, half_extents.z};
}

// ── Cylinder ───────────────────────────────────────────────────

Mesh Cylinder::generate_mesh(int detail) const {
    Mesh mesh;
    float half_h = height * 0.5f;

    for (int i = 0; i <= detail; ++i) {
        float theta = kTwoPi * static_cast<float>(i) / static_cast<float>(detail);
        float ct = std::cos(theta), st = std::sin(theta);
        Vec3 n{ct, 0, st};
        mesh.vertices.push_back({{ct * radius, half_h, st * radius}, n});
        mesh.vertices.push_back({{ct * radius, -half_h, st * radius}, n});
    }
    for (int i = 0; i < detail; ++i) {
        uint32_t a = static_cast<uint32_t>(i * 2);
        mesh.indices.insert(mesh.indices.end(),
            {a, a + 1, a + 2, a + 2, a + 1, a + 3});
    }

    auto cap = [&](float y, Vec3 normal) {
        auto center = static_cast<uint32_t>(mesh.vertices.size());
        mesh.vertices.push_back({{0, y, 0}, normal});
        for (int i = 0; i <= detail; ++i) {
            float theta = kTwoPi * static_cast<float>(i) / static_cast<float>(detail);
            mesh.vertices.push_back(
                {{std::cos(theta) * radius, y, std::sin(theta) * radius}, normal});
        }
        for (int i = 0; i < detail; ++i) {
            if (y > 0)
                mesh.indices.insert(mesh.indices.end(),
                    {center, center + 1 + static_cast<uint32_t>(i),
                     center + 2 + static_cast<uint32_t>(i)});
            else
                mesh.indices.insert(mesh.indices.end(),
                    {center, center + 2 + static_cast<uint32_t>(i),
                     center + 1 + static_cast<uint32_t>(i)});
        }
    };
    cap(half_h, {0, 1, 0});
    cap(-half_h, {0, -1, 0});

    return mesh;
}

AABB Cylinder::local_bounds() const {
    float h = height * 0.5f;
    return {{-radius, -h, -radius}, {radius, h, radius}};
}

std::unique_ptr<Primitive> Cylinder::clone() const {
    return std::make_unique<Cylinder>(*this);
}

void Cylinder::to_json(nlohmann::json& j) const {
    j["type"] = "cylinder";
    j["radius"] = radius;
    j["height"] = height;
}

// ── Cone ───────────────────────────────────────────────────────

Mesh Cone::generate_mesh(int detail) const {
    Mesh mesh;
    float half_h = height * 0.5f;
    float slope = radius / height;

    auto tip = static_cast<uint32_t>(mesh.vertices.size());
    mesh.vertices.push_back({{0, half_h, 0}, {0, 1, 0}});

    for (int i = 0; i <= detail; ++i) {
        float theta = kTwoPi * static_cast<float>(i) / static_cast<float>(detail);
        float ct = std::cos(theta), st = std::sin(theta);
        Vec3 n = Vec3{ct, slope, st}.normalized();
        mesh.vertices.push_back({{ct * radius, -half_h, st * radius}, n});
    }
    for (int i = 0; i < detail; ++i) {
        mesh.indices.insert(mesh.indices.end(),
            {tip, tip + 1 + static_cast<uint32_t>(i),
             tip + 2 + static_cast<uint32_t>(i)});
    }

    auto base_center = static_cast<uint32_t>(mesh.vertices.size());
    mesh.vertices.push_back({{0, -half_h, 0}, {0, -1, 0}});
    for (int i = 0; i <= detail; ++i) {
        float theta = kTwoPi * static_cast<float>(i) / static_cast<float>(detail);
        mesh.vertices.push_back(
            {{std::cos(theta) * radius, -half_h, std::sin(theta) * radius}, {0, -1, 0}});
    }
    for (int i = 0; i < detail; ++i) {
        mesh.indices.insert(mesh.indices.end(),
            {base_center, base_center + 2 + static_cast<uint32_t>(i),
             base_center + 1 + static_cast<uint32_t>(i)});
    }

    return mesh;
}

AABB Cone::local_bounds() const {
    float h = height * 0.5f;
    return {{-radius, -h, -radius}, {radius, h, radius}};
}

std::unique_ptr<Primitive> Cone::clone() const {
    return std::make_unique<Cone>(*this);
}

void Cone::to_json(nlohmann::json& j) const {
    j["type"] = "cone";
    j["radius"] = radius;
    j["height"] = height;
}

// ── Torus ──────────────────────────────────────────────────────

Mesh Torus::generate_mesh(int detail) const {
    Mesh mesh;
    int rings = detail, sides = detail;

    for (int i = 0; i <= rings; ++i) {
        float u = kTwoPi * static_cast<float>(i) / static_cast<float>(rings);
        float cu = std::cos(u), su = std::sin(u);

        for (int j = 0; j <= sides; ++j) {
            float v = kTwoPi * static_cast<float>(j) / static_cast<float>(sides);
            float cv = std::cos(v), sv = std::sin(v);

            float x = (major_radius + minor_radius * cv) * cu;
            float y = minor_radius * sv;
            float z = (major_radius + minor_radius * cv) * su;

            Vec3 n{cv * cu, sv, cv * su};
            mesh.vertices.push_back({{x, y, z}, n});
        }
    }

    for (int i = 0; i < rings; ++i) {
        for (int j = 0; j < sides; ++j) {
            uint32_t a = static_cast<uint32_t>(i * (sides + 1) + j);
            uint32_t b = a + static_cast<uint32_t>(sides + 1);
            mesh.indices.insert(mesh.indices.end(), {a, b, a + 1, a + 1, b, b + 1});
        }
    }
    return mesh;
}

AABB Torus::local_bounds() const {
    float R = major_radius + minor_radius;
    return {{-R, -minor_radius, -R}, {R, minor_radius, R}};
}

std::unique_ptr<Primitive> Torus::clone() const {
    return std::make_unique<Torus>(*this);
}

void Torus::to_json(nlohmann::json& j) const {
    j["type"] = "torus";
    j["major_radius"] = major_radius;
    j["minor_radius"] = minor_radius;
}

// ── Ellipsoid ──────────────────────────────────────────────────

Mesh Ellipsoid::generate_mesh(int detail) const {
    Mesh mesh;
    int stacks = detail, slices = detail * 2;

    for (int i = 0; i <= stacks; ++i) {
        float phi = kPi * static_cast<float>(i) / static_cast<float>(stacks);
        float sp = std::sin(phi), cp = std::cos(phi);
        for (int j = 0; j <= slices; ++j) {
            float theta = kTwoPi * static_cast<float>(j) / static_cast<float>(slices);
            float st = std::sin(theta), ct = std::cos(theta);
            Vec3 unit{sp * ct, cp, sp * st};
            Vec3 pos{unit.x * radii.x, unit.y * radii.y, unit.z * radii.z};
            Vec3 n{unit.x / (radii.x * radii.x),
                    unit.y / (radii.y * radii.y),
                    unit.z / (radii.z * radii.z)};
            mesh.vertices.push_back({pos, n.normalized()});
        }
    }
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            uint32_t a = static_cast<uint32_t>(i * (slices + 1) + j);
            uint32_t b = a + static_cast<uint32_t>(slices + 1);
            mesh.indices.insert(mesh.indices.end(), {a, b, a + 1, a + 1, b, b + 1});
        }
    }
    return mesh;
}

AABB Ellipsoid::local_bounds() const {
    return {{-radii.x, -radii.y, -radii.z}, {radii.x, radii.y, radii.z}};
}

std::unique_ptr<Primitive> Ellipsoid::clone() const {
    return std::make_unique<Ellipsoid>(*this);
}

void Ellipsoid::to_json(nlohmann::json& j) const {
    j["type"] = "ellipsoid";
    j["radii"] = {radii.x, radii.y, radii.z};
}

// ── Halfspace ──────────────────────────────────────────────────

Mesh Halfspace::generate_mesh(int) const {
    Mesh mesh;
    Vec3 t1, t2;
    if (std::abs(normal.x) < 0.9f) t1 = normal.cross({1, 0, 0}).normalized();
    else                            t1 = normal.cross({0, 1, 0}).normalized();
    t2 = normal.cross(t1).normalized();

    constexpr float sz = 10.f;
    Vec3 center = normal * offset;
    Vec3 corners[4] = {
        center + t1 * sz + t2 * sz,
        center - t1 * sz + t2 * sz,
        center - t1 * sz - t2 * sz,
        center + t1 * sz - t2 * sz,
    };
    for (auto& c : corners) mesh.vertices.push_back({c, normal});
    mesh.indices = {0, 1, 2, 0, 2, 3};
    return mesh;
}

AABB Halfspace::local_bounds() const {
    return {{-10, -10, -10}, {10, 10, 10}};
}

std::unique_ptr<Primitive> Halfspace::clone() const {
    return std::make_unique<Halfspace>(*this);
}

void Halfspace::to_json(nlohmann::json& j) const {
    j["type"] = "halfspace";
    j["normal"] = {normal.x, normal.y, normal.z};
    j["offset"] = offset;
}

// ── Pipe ───────────────────────────────────────────────────────

Mesh Pipe::generate_mesh(int detail) const {
    Mesh mesh;
    float half_h = height * 0.5f;

    auto ring = [&](float y, float r, Vec3 n) {
        for (int i = 0; i <= detail; ++i) {
            float theta = kTwoPi * static_cast<float>(i) / static_cast<float>(detail);
            mesh.vertices.push_back(
                {{std::cos(theta) * r, y, std::sin(theta) * r}, n});
        }
    };

    uint32_t base = 0;
    ring(half_h, outer_radius, {0, 0, 1});
    ring(half_h, outer_radius, {0, 0, 1});
    base = 0;
    mesh.vertices.clear();

    auto tube = [&](float r, float ndir) {
        uint32_t start = static_cast<uint32_t>(mesh.vertices.size());
        for (int i = 0; i <= detail; ++i) {
            float theta = kTwoPi * static_cast<float>(i) / static_cast<float>(detail);
            float ct = std::cos(theta), st = std::sin(theta);
            Vec3 n{ct * ndir, 0, st * ndir};
            mesh.vertices.push_back({{ct * r,  half_h, st * r}, n});
            mesh.vertices.push_back({{ct * r, -half_h, st * r}, n});
        }
        for (int i = 0; i < detail; ++i) {
            uint32_t a = start + static_cast<uint32_t>(i * 2);
            if (ndir > 0)
                mesh.indices.insert(mesh.indices.end(), {a, a+1, a+2, a+2, a+1, a+3});
            else
                mesh.indices.insert(mesh.indices.end(), {a, a+2, a+1, a+1, a+2, a+3});
        }
    };
    tube(outer_radius, 1.f);
    tube(inner_radius, -1.f);

    auto annulus = [&](float y, Vec3 n) {
        uint32_t ci = static_cast<uint32_t>(mesh.vertices.size());
        for (int i = 0; i <= detail; ++i) {
            float theta = kTwoPi * static_cast<float>(i) / static_cast<float>(detail);
            float ct = std::cos(theta), st = std::sin(theta);
            mesh.vertices.push_back({{ct * outer_radius, y, st * outer_radius}, n});
            mesh.vertices.push_back({{ct * inner_radius, y, st * inner_radius}, n});
        }
        for (int i = 0; i < detail; ++i) {
            uint32_t a = ci + static_cast<uint32_t>(i * 2);
            if (y > 0)
                mesh.indices.insert(mesh.indices.end(), {a, a+2, a+1, a+1, a+2, a+3});
            else
                mesh.indices.insert(mesh.indices.end(), {a, a+1, a+2, a+2, a+1, a+3});
        }
    };
    annulus(half_h, {0, 1, 0});
    annulus(-half_h, {0, -1, 0});

    return mesh;
}

AABB Pipe::local_bounds() const {
    float h = height * 0.5f;
    return {{-outer_radius, -h, -outer_radius}, {outer_radius, h, outer_radius}};
}

std::unique_ptr<Primitive> Pipe::clone() const {
    return std::make_unique<Pipe>(*this);
}

void Pipe::to_json(nlohmann::json& j) const {
    j["type"] = "pipe";
    j["inner_radius"] = inner_radius;
    j["outer_radius"] = outer_radius;
    j["height"] = height;
}

// ── Wedge ──────────────────────────────────────────────────────

Mesh Wedge::generate_mesh(int) const {
    Mesh mesh;
    float hx = size.x * 0.5f, hy = size.y, hz = size.z * 0.5f;
    float tw = top_width * 0.5f;

    Vec3 v[6] = {
        {-hx, 0, -hz}, { hx, 0, -hz}, { hx, 0, hz}, {-hx, 0, hz},
        {-tw, hy, -hz}, { tw, hy, -hz},
    };
    if (tw < 1e-6f) {
        Vec3 apex{0, hy, 0};
        auto tri = [&](Vec3 a, Vec3 b, Vec3 c) {
            Vec3 n = (b - a).cross(c - a).normalized();
            auto base = static_cast<uint32_t>(mesh.vertices.size());
            mesh.vertices.push_back({a, n});
            mesh.vertices.push_back({b, n});
            mesh.vertices.push_back({c, n});
            mesh.indices.insert(mesh.indices.end(), {base, base+1, base+2});
        };
        tri(v[0], v[1], apex);
        tri(v[1], v[2], apex);
        tri(v[2], v[3], apex);
        tri(v[3], v[0], apex);
        Vec3 bn{0, -1, 0};
        auto base = static_cast<uint32_t>(mesh.vertices.size());
        for (auto& c : v) mesh.vertices.push_back({c, bn});
        mesh.indices.insert(mesh.indices.end(), {base, base+2, base+1, base, base+3, base+2});
    } else {
        Vec3 v8[8] = {
            v[0], v[1], v[2], v[3],
            {-tw, hy, -hz}, {tw, hy, -hz}, {tw, hy, hz}, {-tw, hy, hz},
        };
        struct Face { int idx[4]; };
        Face faces[6] = {
            {{0,1,5,4}}, {{2,3,7,6}}, {{1,2,6,5}}, {{3,0,4,7}},
            {{4,5,6,7}}, {{0,3,2,1}},
        };
        for (auto& f : faces) {
            Vec3 a = v8[f.idx[0]], b = v8[f.idx[1]];
            Vec3 c = v8[f.idx[2]], d = v8[f.idx[3]];
            Vec3 n = (b - a).cross(d - a).normalized();
            auto base = static_cast<uint32_t>(mesh.vertices.size());
            mesh.vertices.push_back({a, n});
            mesh.vertices.push_back({b, n});
            mesh.vertices.push_back({c, n});
            mesh.vertices.push_back({d, n});
            mesh.indices.insert(mesh.indices.end(), {base, base+1, base+2, base, base+2, base+3});
        }
    }
    return mesh;
}

AABB Wedge::local_bounds() const {
    float hx = size.x * 0.5f, hz = size.z * 0.5f;
    return {{-hx, 0, -hz}, {hx, size.y, hz}};
}

std::unique_ptr<Primitive> Wedge::clone() const {
    return std::make_unique<Wedge>(*this);
}

void Wedge::to_json(nlohmann::json& j) const {
    j["type"] = "wedge";
    j["size"] = {size.x, size.y, size.z};
    j["top_width"] = top_width;
}

// ── Arb8 ───────────────────────────────────────────────────────

Mesh Arb8::generate_mesh(int) const {
    Mesh mesh;
    struct Face { int idx[4]; };
    Face faces[6] = {
        {{0, 3, 2, 1}},
        {{4, 5, 6, 7}},
        {{0, 1, 5, 4}},
        {{2, 3, 7, 6}},
        {{1, 2, 6, 5}},
        {{0, 4, 7, 3}},
    };
    for (auto& f : faces) {
        Vec3 a = verts[f.idx[0]], b = verts[f.idx[1]];
        Vec3 c = verts[f.idx[2]], d = verts[f.idx[3]];
        Vec3 n = (b - a).cross(d - a).normalized();
        auto base = static_cast<uint32_t>(mesh.vertices.size());
        mesh.vertices.push_back({a, n});
        mesh.vertices.push_back({b, n});
        mesh.vertices.push_back({c, n});
        mesh.vertices.push_back({d, n});
        mesh.indices.insert(mesh.indices.end(), {base, base+1, base+2, base, base+2, base+3});
    }
    return mesh;
}

AABB Arb8::local_bounds() const {
    AABB bb;
    for (auto& v : verts) bb.expand(v);
    return bb;
}

std::unique_ptr<Primitive> Arb8::clone() const {
    auto p = std::make_unique<Arb8>();
    for (int i = 0; i < 8; ++i) p->verts[i] = verts[i];
    return p;
}

void Arb8::to_json(nlohmann::json& j) const {
    j["type"] = "arb8";
    auto& v = j["verts"];
    v = nlohmann::json::array();
    for (int i = 0; i < 8; ++i)
        v.push_back({verts[i].x, verts[i].y, verts[i].z});
}

}  // namespace smidr
