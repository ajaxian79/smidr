#include "core/Primitive.h"

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
        case PrimitiveType::Sphere:   return "sphere";
        case PrimitiveType::Box:      return "box";
        case PrimitiveType::Cylinder: return "cylinder";
        case PrimitiveType::Cone:     return "cone";
        case PrimitiveType::Torus:    return "torus";
    }
    return "unknown";
}

PrimitiveType primitive_type_from_name(const std::string& name) {
    if (name == "sphere")   return PrimitiveType::Sphere;
    if (name == "box")      return PrimitiveType::Box;
    if (name == "cylinder") return PrimitiveType::Cylinder;
    if (name == "cone")     return PrimitiveType::Cone;
    if (name == "torus")    return PrimitiveType::Torus;
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

}  // namespace smidr
