#pragma once

#include "core/Math.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace smidr {

struct ShadeContext {
    Vec3  point;
    Vec3  normal;
    Vec3  view_dir;
    Vec3  uv;
    float u = 0.f, v = 0.f;
};

struct ShadeResult {
    Vec3  color{0.6f, 0.6f, 0.7f};
    float roughness  = 0.5f;
    float metallic   = 0.f;
    float opacity     = 1.f;
    float emission    = 0.f;
};

class SurfaceShader {
public:
    virtual ~SurfaceShader() = default;
    virtual ShadeResult evaluate(const ShadeContext& ctx) const = 0;
    virtual std::string name() const = 0;
    virtual std::unique_ptr<SurfaceShader> clone() const = 0;
};

class PlasticShader : public SurfaceShader {
public:
    Vec3 color{0.6f, 0.6f, 0.7f};
    float shininess = 64.f;
    ShadeResult evaluate(const ShadeContext&) const override;
    std::string name() const override { return "plastic"; }
    std::unique_ptr<SurfaceShader> clone() const override;
};

class FlatShader : public SurfaceShader {
public:
    Vec3 color{1.f, 1.f, 1.f};
    ShadeResult evaluate(const ShadeContext&) const override;
    std::string name() const override { return "flat"; }
    std::unique_ptr<SurfaceShader> clone() const override;
};

class CookTorranceShader : public SurfaceShader {
public:
    Vec3  base_color{0.8f, 0.5f, 0.3f};
    float roughness = 0.3f;
    float metallic  = 0.8f;
    float ior       = 1.5f;
    ShadeResult evaluate(const ShadeContext&) const override;
    std::string name() const override { return "cook_torrance"; }
    std::unique_ptr<SurfaceShader> clone() const override;
};

class CheckerShader : public SurfaceShader {
public:
    Vec3 color_a{0.9f, 0.9f, 0.9f};
    Vec3 color_b{0.1f, 0.1f, 0.1f};
    float scale = 1.f;
    ShadeResult evaluate(const ShadeContext& ctx) const override;
    std::string name() const override { return "checker"; }
    std::unique_ptr<SurfaceShader> clone() const override;
};

class NoiseShader : public SurfaceShader {
public:
    Vec3 color_a{0.3f, 0.2f, 0.1f};
    Vec3 color_b{0.8f, 0.6f, 0.4f};
    float scale = 2.f;
    int   octaves = 4;
    ShadeResult evaluate(const ShadeContext& ctx) const override;
    std::string name() const override { return "noise"; }
    std::unique_ptr<SurfaceShader> clone() const override;
};

class WoodShader : public SurfaceShader {
public:
    Vec3 color_light{0.8f, 0.6f, 0.3f};
    Vec3 color_dark{0.4f, 0.2f, 0.08f};
    float ring_scale = 8.f;
    ShadeResult evaluate(const ShadeContext& ctx) const override;
    std::string name() const override { return "wood"; }
    std::unique_ptr<SurfaceShader> clone() const override;
};

class CamoShader : public SurfaceShader {
public:
    Vec3 color1{0.2f, 0.3f, 0.15f};
    Vec3 color2{0.35f, 0.25f, 0.1f};
    Vec3 color3{0.15f, 0.15f, 0.1f};
    float scale = 3.f;
    ShadeResult evaluate(const ShadeContext& ctx) const override;
    std::string name() const override { return "camo"; }
    std::unique_ptr<SurfaceShader> clone() const override;
};

class ToonShader : public SurfaceShader {
public:
    Vec3 color{0.6f, 0.3f, 0.3f};
    int levels = 4;
    ShadeResult evaluate(const ShadeContext& ctx) const override;
    std::string name() const override { return "toon"; }
    std::unique_ptr<SurfaceShader> clone() const override;
};

class CloudShader : public SurfaceShader {
public:
    Vec3 sky_color{0.4f, 0.6f, 0.9f};
    Vec3 cloud_color{0.95f, 0.95f, 0.95f};
    float density = 0.5f;
    ShadeResult evaluate(const ShadeContext& ctx) const override;
    std::string name() const override { return "cloud"; }
    std::unique_ptr<SurfaceShader> clone() const override;
};

class MirrorShader : public SurfaceShader {
public:
    Vec3 tint{0.95f, 0.95f, 0.95f};
    ShadeResult evaluate(const ShadeContext&) const override;
    std::string name() const override { return "mirror"; }
    std::unique_ptr<SurfaceShader> clone() const override;
};

class GlassShader : public SurfaceShader {
public:
    Vec3 tint{0.95f, 0.98f, 1.f};
    float ior = 1.5f;
    ShadeResult evaluate(const ShadeContext&) const override;
    std::string name() const override { return "glass"; }
    std::unique_ptr<SurfaceShader> clone() const override;
};

class EmissionShader : public SurfaceShader {
public:
    Vec3 color{1.f, 0.9f, 0.7f};
    float intensity = 2.f;
    ShadeResult evaluate(const ShadeContext&) const override;
    std::string name() const override { return "emission"; }
    std::unique_ptr<SurfaceShader> clone() const override;
};

class ShaderLibrary {
public:
    ShaderLibrary();
    void register_shader(std::unique_ptr<SurfaceShader> shader);
    const SurfaceShader* get(const std::string& name) const;
    std::vector<std::string> list() const;

private:
    std::unordered_map<std::string, std::unique_ptr<SurfaceShader>> shaders_;
};

float perlin_noise_3d(float x, float y, float z);

}  // namespace smidr
