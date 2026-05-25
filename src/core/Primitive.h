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
    Torus
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

}  // namespace smidr
