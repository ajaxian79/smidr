#pragma once

#include "core/Primitive.h"

#include <array>

namespace smidr {

class Superellipsoid : public Primitive {
public:
    Vec3 radii{1.f, 0.7f, 0.5f};
    float n1 = 1.f, n2 = 1.f;

    PrimitiveType type() const override { return PrimitiveType::Superellipsoid; }
    Mesh generate_mesh(int detail = 32) const override;
    AABB local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

class Particle : public Primitive {
public:
    Vec3 base{0, 0, 0};
    Vec3 height_vec{0, 1, 0};
    float base_radius = 0.3f;
    float top_radius  = 0.1f;

    PrimitiveType type() const override { return PrimitiveType::Particle; }
    Mesh generate_mesh(int detail = 32) const override;
    AABB local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

class Arbn : public Primitive {
public:
    struct Plane { Vec3 normal; float dist; };
    std::vector<Plane> planes;

    Arbn();

    PrimitiveType type() const override { return PrimitiveType::Arbn; }
    Mesh generate_mesh(int detail = 32) const override;
    AABB local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

class RPC : public Primitive {
public:
    float height = 2.f;
    float half_width = 0.5f;
    float depth = 1.f;

    PrimitiveType type() const override { return PrimitiveType::RPC; }
    Mesh generate_mesh(int detail = 32) const override;
    AABB local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

class RHC : public Primitive {
public:
    float height = 2.f;
    float half_width = 0.5f;
    float apex_dist = 0.3f;

    PrimitiveType type() const override { return PrimitiveType::RHC; }
    Mesh generate_mesh(int detail = 32) const override;
    AABB local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

class EPA : public Primitive {
public:
    float height = 2.f;
    float semi_major = 1.f;
    float semi_minor = 0.5f;

    PrimitiveType type() const override { return PrimitiveType::EPA; }
    Mesh generate_mesh(int detail = 32) const override;
    AABB local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

class EHY : public Primitive {
public:
    float height = 2.f;
    float semi_major = 1.f;
    float semi_minor = 0.5f;
    float apex_dist = 0.2f;

    PrimitiveType type() const override { return PrimitiveType::EHY; }
    Mesh generate_mesh(int detail = 32) const override;
    AABB local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

class ETO : public Primitive {
public:
    float major_radius = 1.f;
    float semi_major = 0.4f;
    float semi_minor = 0.2f;

    PrimitiveType type() const override { return PrimitiveType::ETO; }
    Mesh generate_mesh(int detail = 32) const override;
    AABB local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

class Hyperboloid : public Primitive {
public:
    float height = 2.f;
    float base_radius = 1.f;
    float neck_radius = 0.3f;

    PrimitiveType type() const override { return PrimitiveType::Hyperboloid; }
    Mesh generate_mesh(int detail = 32) const override;
    AABB local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

class Bot : public Primitive {
public:
    std::vector<Vec3>     bot_vertices;
    std::vector<uint32_t> bot_faces;

    Bot();

    PrimitiveType type() const override { return PrimitiveType::Bot; }
    Mesh generate_mesh(int detail = 32) const override;
    AABB local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

struct SketchSegment {
    enum Type { Line, Arc, Bezier };
    Type type = Line;
    std::vector<Vec3> points;
};

class SketchPrimitive : public Primitive {
public:
    std::vector<SketchSegment> segments;

    SketchPrimitive();

    PrimitiveType type() const override { return PrimitiveType::Sketch; }
    Mesh generate_mesh(int detail = 32) const override;
    AABB local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;

    std::vector<Vec3> tessellate_profile(int detail) const;
};

class ExtrudePrimitive : public Primitive {
public:
    SketchPrimitive sketch;
    Vec3 direction{0, 0, 1};
    float depth = 1.f;

    PrimitiveType type() const override { return PrimitiveType::Extrude; }
    Mesh generate_mesh(int detail = 32) const override;
    AABB local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

class RevolvePrimitive : public Primitive {
public:
    SketchPrimitive sketch;
    Vec3 axis{0, 1, 0};
    float angle = 360.f;

    PrimitiveType type() const override { return PrimitiveType::Revolve; }
    Mesh generate_mesh(int detail = 32) const override;
    AABB local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

class DSPPrimitive : public Primitive {
public:
    int width = 8, height_dim = 8;
    std::vector<float> heightmap;
    float cell_size = 1.f;
    float height_scale = 1.f;

    DSPPrimitive();

    PrimitiveType type() const override { return PrimitiveType::DSP; }
    Mesh generate_mesh(int detail = 32) const override;
    AABB local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

class MetaballPrimitive : public Primitive {
public:
    struct Control { Vec3 position; float strength; float radius; };
    std::vector<Control> controls;
    float threshold = 1.f;

    MetaballPrimitive();

    PrimitiveType type() const override { return PrimitiveType::Metaball; }
    Mesh generate_mesh(int detail = 32) const override;
    AABB local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

class HeartPrimitive : public Primitive {
public:
    float scale = 1.f;

    PrimitiveType type() const override { return PrimitiveType::Heart; }
    Mesh generate_mesh(int detail = 32) const override;
    AABB local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

class PointCloudPrimitive : public Primitive {
public:
    std::vector<Vec3> points;
    float point_size = 0.05f;

    PointCloudPrimitive();

    PrimitiveType type() const override { return PrimitiveType::PointCloud; }
    Mesh generate_mesh(int detail = 32) const override;
    AABB local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

class AnnotationPrimitive : public Primitive {
public:
    std::string text = "Label";
    Vec3 anchor{0, 0, 0};
    Vec3 offset{0, 1, 0};

    PrimitiveType type() const override { return PrimitiveType::Annotation; }
    Mesh generate_mesh(int detail = 32) const override;
    AABB local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

}  // namespace smidr
