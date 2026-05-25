#pragma once

#include "core/Math.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "io/json_fwd.hpp"

namespace smidr {

struct Vertex {
    Vec3 position;
    Vec3 normal;
};

struct Mesh {
    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;

    void clear() { vertices.clear(); indices.clear(); }
    AABB bounds() const;
};

enum class PrimitiveType {
    Sphere,
    Box,
    Cylinder,
    Cone,
    Torus,
    Ellipsoid,
    Halfspace,
    Pipe,
    Wedge,
    Arb8
};

const char* primitive_type_name(PrimitiveType t);
PrimitiveType primitive_type_from_name(const std::string& name);

class Primitive {
public:
    virtual ~Primitive() = default;

    virtual PrimitiveType type() const = 0;
    virtual Mesh          generate_mesh(int detail = 32) const = 0;
    virtual AABB          local_bounds() const = 0;
    virtual std::unique_ptr<Primitive> clone() const = 0;

    virtual void to_json(nlohmann::json& j) const = 0;

    static std::unique_ptr<Primitive> from_json(const nlohmann::json& j);
};

class Sphere : public Primitive {
public:
    float radius = 1.f;

    Sphere() = default;
    explicit Sphere(float r) : radius(r) {}

    PrimitiveType type() const override { return PrimitiveType::Sphere; }
    Mesh          generate_mesh(int detail = 32) const override;
    AABB          local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

class Box : public Primitive {
public:
    Vec3 half_extents{0.5f, 0.5f, 0.5f};

    Box() = default;
    explicit Box(Vec3 he) : half_extents(he) {}

    PrimitiveType type() const override { return PrimitiveType::Box; }
    Mesh          generate_mesh(int detail = 32) const override;
    AABB          local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

class Cylinder : public Primitive {
public:
    float radius = 0.5f;
    float height = 2.f;

    Cylinder() = default;
    Cylinder(float r, float h) : radius(r), height(h) {}

    PrimitiveType type() const override { return PrimitiveType::Cylinder; }
    Mesh          generate_mesh(int detail = 32) const override;
    AABB          local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

class Cone : public Primitive {
public:
    float radius = 0.5f;
    float height = 2.f;

    Cone() = default;
    Cone(float r, float h) : radius(r), height(h) {}

    PrimitiveType type() const override { return PrimitiveType::Cone; }
    Mesh          generate_mesh(int detail = 32) const override;
    AABB          local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

class Torus : public Primitive {
public:
    float major_radius = 1.f;
    float minor_radius = 0.3f;

    Torus() = default;
    Torus(float R, float r) : major_radius(R), minor_radius(r) {}

    PrimitiveType type() const override { return PrimitiveType::Torus; }
    Mesh          generate_mesh(int detail = 32) const override;
    AABB          local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

class Ellipsoid : public Primitive {
public:
    Vec3 radii{1.f, 0.7f, 0.5f};

    Ellipsoid() = default;
    explicit Ellipsoid(Vec3 r) : radii(r) {}

    PrimitiveType type() const override { return PrimitiveType::Ellipsoid; }
    Mesh          generate_mesh(int detail = 32) const override;
    AABB          local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

class Halfspace : public Primitive {
public:
    Vec3  normal{0, 1, 0};
    float offset = 0.f;

    Halfspace() = default;
    Halfspace(Vec3 n, float d) : normal(n.normalized()), offset(d) {}

    PrimitiveType type() const override { return PrimitiveType::Halfspace; }
    Mesh          generate_mesh(int detail = 32) const override;
    AABB          local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

class Pipe : public Primitive {
public:
    float inner_radius = 0.3f;
    float outer_radius = 0.5f;
    float height = 2.f;

    Pipe() = default;
    Pipe(float ri, float ro, float h) : inner_radius(ri), outer_radius(ro), height(h) {}

    PrimitiveType type() const override { return PrimitiveType::Pipe; }
    Mesh          generate_mesh(int detail = 32) const override;
    AABB          local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

class Wedge : public Primitive {
public:
    Vec3 size{1.f, 1.f, 1.f};
    float top_width = 0.f;

    Wedge() = default;
    Wedge(Vec3 s, float tw) : size(s), top_width(tw) {}

    PrimitiveType type() const override { return PrimitiveType::Wedge; }
    Mesh          generate_mesh(int detail = 32) const override;
    AABB          local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

class Arb8 : public Primitive {
public:
    Vec3 verts[8] = {
        {-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f},
        { 0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f},
        {-0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f,  0.5f},
        { 0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f},
    };

    Arb8() = default;

    PrimitiveType type() const override { return PrimitiveType::Arb8; }
    Mesh          generate_mesh(int detail = 32) const override;
    AABB          local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

}  // namespace smidr
