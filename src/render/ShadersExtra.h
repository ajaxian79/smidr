#pragma once

#include "render/Shaders.h"

namespace smidr {

class MarbleShader : public SurfaceShader {
public:
    Vec3 vein_color{0.1f, 0.1f, 0.15f};
    Vec3 base_color{0.85f, 0.83f, 0.8f};
    float frequency = 3.f;
    ShadeResult evaluate(const ShadeContext& ctx) const override;
    std::string name() const override { return "marble"; }
    std::unique_ptr<SurfaceShader> clone() const override { return std::make_unique<MarbleShader>(*this); }
};

class GraniteShader : public SurfaceShader {
public:
    Vec3 base_color{0.55f, 0.5f, 0.48f};
    Vec3 spot_color{0.2f, 0.2f, 0.18f};
    float scale = 8.f;
    ShadeResult evaluate(const ShadeContext& ctx) const override;
    std::string name() const override { return "granite"; }
    std::unique_ptr<SurfaceShader> clone() const override { return std::make_unique<GraniteShader>(*this); }
};

class BrickShader : public SurfaceShader {
public:
    Vec3 brick_color{0.65f, 0.3f, 0.2f};
    Vec3 mortar_color{0.85f, 0.85f, 0.82f};
    float brick_w = 0.4f, brick_h = 0.15f, mortar_w = 0.02f;
    ShadeResult evaluate(const ShadeContext& ctx) const override;
    std::string name() const override { return "brick"; }
    std::unique_ptr<SurfaceShader> clone() const override { return std::make_unique<BrickShader>(*this); }
};

class FireShader : public SurfaceShader {
public:
    Vec3 hot_color{1.f, 0.9f, 0.3f};
    Vec3 cool_color{0.6f, 0.1f, 0.05f};
    float scale = 3.f;
    ShadeResult evaluate(const ShadeContext& ctx) const override;
    std::string name() const override { return "fire"; }
    std::unique_ptr<SurfaceShader> clone() const override { return std::make_unique<FireShader>(*this); }
};

class WaterShader : public SurfaceShader {
public:
    Vec3 water_color{0.15f, 0.45f, 0.6f};
    float roughness = 0.05f;
    ShadeResult evaluate(const ShadeContext& ctx) const override;
    std::string name() const override { return "water"; }
    std::unique_ptr<SurfaceShader> clone() const override { return std::make_unique<WaterShader>(*this); }
};

class FabricShader : public SurfaceShader {
public:
    Vec3 color{0.6f, 0.55f, 0.5f};
    float weave_scale = 30.f;
    ShadeResult evaluate(const ShadeContext& ctx) const override;
    std::string name() const override { return "fabric"; }
    std::unique_ptr<SurfaceShader> clone() const override { return std::make_unique<FabricShader>(*this); }
};

class LeatherShader : public SurfaceShader {
public:
    Vec3 color{0.4f, 0.25f, 0.15f};
    float grain = 20.f;
    ShadeResult evaluate(const ShadeContext& ctx) const override;
    std::string name() const override { return "leather"; }
    std::unique_ptr<SurfaceShader> clone() const override { return std::make_unique<LeatherShader>(*this); }
};

class BumpShader : public SurfaceShader {
public:
    Vec3 color{0.6f, 0.6f, 0.65f};
    float strength = 0.5f;
    float scale = 5.f;
    ShadeResult evaluate(const ShadeContext& ctx) const override;
    std::string name() const override { return "bump"; }
    std::unique_ptr<SurfaceShader> clone() const override { return std::make_unique<BumpShader>(*this); }
};

class DotsShader : public SurfaceShader {
public:
    Vec3 dot_color{0.9f, 0.2f, 0.2f};
    Vec3 bg_color{0.95f, 0.95f, 0.95f};
    float spacing = 0.3f;
    float radius = 0.1f;
    ShadeResult evaluate(const ShadeContext& ctx) const override;
    std::string name() const override { return "dots"; }
    std::unique_ptr<SurfaceShader> clone() const override { return std::make_unique<DotsShader>(*this); }
};

class StripesShader : public SurfaceShader {
public:
    Vec3 color_a{0.1f, 0.1f, 0.1f};
    Vec3 color_b{0.95f, 0.95f, 0.95f};
    float width = 0.1f;
    ShadeResult evaluate(const ShadeContext& ctx) const override;
    std::string name() const override { return "stripes"; }
    std::unique_ptr<SurfaceShader> clone() const override { return std::make_unique<StripesShader>(*this); }
};

class VoronoiShader : public SurfaceShader {
public:
    Vec3 cell_color{0.7f, 0.65f, 0.6f};
    Vec3 edge_color{0.1f, 0.1f, 0.1f};
    float scale = 5.f;
    ShadeResult evaluate(const ShadeContext& ctx) const override;
    std::string name() const override { return "voronoi"; }
    std::unique_ptr<SurfaceShader> clone() const override { return std::make_unique<VoronoiShader>(*this); }
};

class PlasmaShader : public SurfaceShader {
public:
    Vec3 hot{1.f, 0.3f, 0.8f};
    Vec3 cold{0.2f, 0.5f, 1.f};
    float scale = 2.f;
    ShadeResult evaluate(const ShadeContext& ctx) const override;
    std::string name() const override { return "plasma"; }
    std::unique_ptr<SurfaceShader> clone() const override { return std::make_unique<PlasmaShader>(*this); }
};

class FresnelShader : public SurfaceShader {
public:
    Vec3 facing_color{0.2f, 0.2f, 0.2f};
    Vec3 edge_color{1.f, 1.f, 1.f};
    float power = 4.f;
    ShadeResult evaluate(const ShadeContext& ctx) const override;
    std::string name() const override { return "fresnel"; }
    std::unique_ptr<SurfaceShader> clone() const override { return std::make_unique<FresnelShader>(*this); }
};

class HatchingShader : public SurfaceShader {
public:
    Vec3 paper_color{0.96f, 0.94f, 0.9f};
    Vec3 line_color{0.1f, 0.1f, 0.1f};
    float density = 30.f;
    ShadeResult evaluate(const ShadeContext& ctx) const override;
    std::string name() const override { return "hatching"; }
    std::unique_ptr<SurfaceShader> clone() const override { return std::make_unique<HatchingShader>(*this); }
};

class RustShader : public SurfaceShader {
public:
    Vec3 metal{0.5f, 0.5f, 0.55f};
    Vec3 rust{0.6f, 0.25f, 0.1f};
    float amount = 0.5f;
    ShadeResult evaluate(const ShadeContext& ctx) const override;
    std::string name() const override { return "rust"; }
    std::unique_ptr<SurfaceShader> clone() const override { return std::make_unique<RustShader>(*this); }
};

class GoldShader : public SurfaceShader {
public:
    Vec3 color{1.0f, 0.85f, 0.4f};
    ShadeResult evaluate(const ShadeContext& ctx) const override;
    std::string name() const override { return "gold"; }
    std::unique_ptr<SurfaceShader> clone() const override { return std::make_unique<GoldShader>(*this); }
};

class SilverShader : public SurfaceShader {
public:
    Vec3 color{0.95f, 0.96f, 0.97f};
    ShadeResult evaluate(const ShadeContext& ctx) const override;
    std::string name() const override { return "silver"; }
    std::unique_ptr<SurfaceShader> clone() const override { return std::make_unique<SilverShader>(*this); }
};

void register_extra_shaders(ShaderLibrary& lib);

}  // namespace smidr
