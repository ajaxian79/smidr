#pragma once

#include "core/Primitive.h"

#include <string>
#include <vector>

namespace smidr {

// CLINE — FASTGEN hollow cone/cylinder
class CLinePrimitive : public Primitive {
public:
    Vec3  start{0, 0, 0};
    Vec3  end{0, 1, 0};
    float outer_radius = 0.3f;
    float wall_thickness = 0.05f;

    PrimitiveType type() const override { return PrimitiveType::CLine; }
    Mesh generate_mesh(int detail = 32) const override;
    AABB local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

// Joint — kinematic constraint reference visualization
class JointPrimitive : public Primitive {
public:
    Vec3 position{0, 0, 0};
    Vec3 axis{0, 1, 0};
    float radius = 0.1f;
    int   dof = 1;   // 1=revolute, 2=cylindrical, 3=spherical, 6=free

    PrimitiveType type() const override { return PrimitiveType::Joint; }
    Mesh generate_mesh(int detail = 32) const override;
    AABB local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

// Grip — display/interaction handle (cross shape)
class GripPrimitive : public Primitive {
public:
    Vec3 position{0, 0, 0};
    float size = 0.2f;

    PrimitiveType type() const override { return PrimitiveType::Grip; }
    Mesh generate_mesh(int detail = 32) const override;
    AABB local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

// Datum — ASME Y14.5M datum reference (plane / line / point)
class DatumPrimitive : public Primitive {
public:
    enum Kind { Plane, Line, Point };
    Kind kind = Plane;
    Vec3 origin{0, 0, 0};
    Vec3 normal{0, 1, 0};
    float size = 1.f;
    std::string label = "A";

    PrimitiveType type() const override { return PrimitiveType::Datum; }
    Mesh generate_mesh(int detail = 32) const override;
    AABB local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

// Submodel — placeholder cube box (real impl would reference external .g)
class SubmodelPrimitive : public Primitive {
public:
    std::string path;
    Vec3 size{1, 1, 1};

    PrimitiveType type() const override { return PrimitiveType::Submodel; }
    Mesh generate_mesh(int detail = 32) const override;
    AABB local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

// Script — procedural geometry script result (displayed as wireframe box)
class ScriptPrimitive : public Primitive {
public:
    std::string script_text = "// procedural geometry";
    Vec3 bounds_min{-0.5f, -0.5f, -0.5f};
    Vec3 bounds_max{0.5f, 0.5f, 0.5f};

    PrimitiveType type() const override { return PrimitiveType::Script; }
    Mesh generate_mesh(int detail = 32) const override;
    AABB local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

// EBM — Extruded Bitmap (heightfield by ON/OFF mask + height)
class EBMPrimitive : public Primitive {
public:
    int width = 16, height = 16;
    std::vector<uint8_t> mask;
    float cell_size = 0.1f;
    float extrude_height = 0.5f;

    EBMPrimitive();

    PrimitiveType type() const override { return PrimitiveType::EBM; }
    Mesh generate_mesh(int detail = 32) const override;
    AABB local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

// VOL — voxel volume (visualized as filled cubes on threshold)
class VOLPrimitive : public Primitive {
public:
    int dim_x = 8, dim_y = 8, dim_z = 8;
    std::vector<uint8_t> voxels;
    float cell_size = 0.2f;
    uint8_t threshold = 128;

    VOLPrimitive();

    PrimitiveType type() const override { return PrimitiveType::VOL; }
    Mesh generate_mesh(int detail = 32) const override;
    AABB local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

// HF — Height Field from float grid
class HFPrimitive : public Primitive {
public:
    int width = 16, depth = 16;
    std::vector<float> heights;
    float cell_size = 0.2f;

    HFPrimitive();

    PrimitiveType type() const override { return PrimitiveType::HF; }
    Mesh generate_mesh(int detail = 32) const override;
    AABB local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

// ARS — Arbitrary Ruled Surface (grid of vertices, ruled between rows)
class ARSPrimitive : public Primitive {
public:
    int ncurves = 4;
    int points_per_curve = 8;
    std::vector<Vec3> grid;

    ARSPrimitive();

    PrimitiveType type() const override { return PrimitiveType::ARS; }
    Mesh generate_mesh(int detail = 32) const override;
    AABB local_bounds() const override;
    std::unique_ptr<Primitive> clone() const override;
    void to_json(nlohmann::json& j) const override;
};

}  // namespace smidr
