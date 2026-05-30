#include "render/ShadersExtra.h"

#include <cmath>

namespace smidr {

static float fbm_helper(Vec3 p, int octaves, float scale) {
    float val = 0.f, amp = 1.f, freq = scale;
    for (int i = 0; i < octaves; ++i) {
        val += amp * perlin_noise_3d(p.x * freq, p.y * freq, p.z * freq);
        amp *= 0.5f; freq *= 2.f;
    }
    return val;
}

ShadeResult MarbleShader::evaluate(const ShadeContext& ctx) const {
    float n = fbm_helper(ctx.point, 4, frequency);
    float v = std::abs(std::sin(ctx.point.x * 2.f + n * 4.f));
    Vec3 c = base_color * (1.f - v) + vein_color * v;
    return {c, 0.2f, 0.f, 1.f, 0.f};
}

ShadeResult GraniteShader::evaluate(const ShadeContext& ctx) const {
    float n = (perlin_noise_3d(ctx.point.x * scale, ctx.point.y * scale, ctx.point.z * scale) + 1.f) * 0.5f;
    float spot = n > 0.6f ? 1.f : 0.f;
    Vec3 c = base_color * (1.f - spot) + spot_color * spot;
    return {c, 0.5f, 0.f, 1.f, 0.f};
}

ShadeResult BrickShader::evaluate(const ShadeContext& ctx) const {
    float u = ctx.point.x, v = ctx.point.y;
    float row = std::floor(v / brick_h);
    float u_offset = (static_cast<int>(row) & 1) ? brick_w * 0.5f : 0.f;
    float local_u = std::fmod(u + u_offset, brick_w);
    float local_v = std::fmod(v, brick_h);
    bool in_mortar = local_u < mortar_w || local_u > brick_w - mortar_w
                     || local_v < mortar_w || local_v > brick_h - mortar_w;
    return {in_mortar ? mortar_color : brick_color, 0.8f, 0.f, 1.f, 0.f};
}

ShadeResult FireShader::evaluate(const ShadeContext& ctx) const {
    float n = fbm_helper(ctx.point, 4, scale);
    float t = std::max(0.f, std::min(1.f, (n + 1.f) * 0.5f));
    Vec3 c = cool_color * (1.f - t) + hot_color * t;
    return {c, 0.9f, 0.f, 1.f, t * 1.5f};
}

ShadeResult WaterShader::evaluate(const ShadeContext& ctx) const {
    float ripple = fbm_helper(ctx.point, 3, 8.f) * 0.1f;
    Vec3 c = water_color + Vec3{ripple, ripple, ripple};
    return {c, roughness, 0.f, 0.7f, 0.f};
}

ShadeResult FabricShader::evaluate(const ShadeContext& ctx) const {
    float warp = std::sin(ctx.point.x * weave_scale) * 0.5f + 0.5f;
    float weft = std::sin(ctx.point.z * weave_scale) * 0.5f + 0.5f;
    float w = (warp + weft) * 0.5f;
    return {color * (0.7f + 0.3f * w), 0.8f, 0.f, 1.f, 0.f};
}

ShadeResult LeatherShader::evaluate(const ShadeContext& ctx) const {
    float n = fbm_helper(ctx.point, 5, grain);
    float v = (n + 1.f) * 0.5f;
    return {color * (0.7f + 0.3f * v), 0.7f, 0.f, 1.f, 0.f};
}

ShadeResult BumpShader::evaluate(const ShadeContext& ctx) const {
    float n = fbm_helper(ctx.point, 4, scale);
    Vec3 perturb{n * strength, 0, 0};
    Vec3 nn = (ctx.normal + perturb).normalized();
    float shade = std::max(0.1f, nn.dot(Vec3{0.4f, 0.8f, 0.6f}.normalized()));
    return {color * shade, 0.5f, 0.f, 1.f, 0.f};
}

ShadeResult DotsShader::evaluate(const ShadeContext& ctx) const {
    float u = std::fmod(std::abs(ctx.point.x), spacing) - spacing * 0.5f;
    float v = std::fmod(std::abs(ctx.point.y), spacing) - spacing * 0.5f;
    float d = std::sqrt(u*u + v*v);
    return {d < radius ? dot_color : bg_color, 0.5f, 0.f, 1.f, 0.f};
}

ShadeResult StripesShader::evaluate(const ShadeContext& ctx) const {
    float s = std::fmod(std::abs(ctx.point.x), width * 2.f);
    return {s < width ? color_a : color_b, 0.6f, 0.f, 1.f, 0.f};
}

ShadeResult VoronoiShader::evaluate(const ShadeContext& ctx) const {
    Vec3 p = ctx.point * scale;
    Vec3 ip{std::floor(p.x), std::floor(p.y), std::floor(p.z)};
    float min_d = 1e30f, second_d = 1e30f;
    for (int dz = -1; dz <= 1; ++dz)
    for (int dy = -1; dy <= 1; ++dy)
    for (int dx = -1; dx <= 1; ++dx) {
        Vec3 cell = ip + Vec3{static_cast<float>(dx), static_cast<float>(dy), static_cast<float>(dz)};
        Vec3 seed{cell.x + 0.5f + 0.5f * perlin_noise_3d(cell.x*3.1f, cell.y*7.7f, cell.z*5.3f),
                   cell.y + 0.5f + 0.5f * perlin_noise_3d(cell.x*5.5f, cell.y*2.2f, cell.z*9.1f),
                   cell.z + 0.5f + 0.5f * perlin_noise_3d(cell.x*8.7f, cell.y*6.1f, cell.z*4.4f)};
        Vec3 d = seed - p;
        float dist = d.length();
        if (dist < min_d) { second_d = min_d; min_d = dist; }
        else if (dist < second_d) second_d = dist;
    }
    float edge = second_d - min_d;
    bool is_edge = edge < 0.05f;
    return {is_edge ? edge_color : cell_color, 0.6f, 0.f, 1.f, 0.f};
}

ShadeResult PlasmaShader::evaluate(const ShadeContext& ctx) const {
    float n = fbm_helper(ctx.point, 6, scale);
    float t = std::sin(n * 4.f) * 0.5f + 0.5f;
    Vec3 c = cold * (1.f - t) + hot * t;
    return {c, 0.3f, 0.f, 1.f, t * 0.5f};
}

ShadeResult FresnelShader::evaluate(const ShadeContext& ctx) const {
    float f = 1.f - std::abs(ctx.normal.dot(ctx.view_dir));
    f = std::pow(std::max(0.f, std::min(1.f, f)), power);
    Vec3 c = facing_color * (1.f - f) + edge_color * f;
    return {c, 0.2f, 0.5f, 1.f, 0.f};
}

ShadeResult HatchingShader::evaluate(const ShadeContext& ctx) const {
    float light = std::max(0.f, ctx.normal.dot(Vec3{0.4f, 0.8f, 0.6f}.normalized()));
    float hatch = std::sin(ctx.point.x * density + ctx.point.y * density * 0.7f) * 0.5f + 0.5f;
    bool drawn = hatch < (1.f - light) * 0.8f;
    return {drawn ? line_color : paper_color, 1.f, 0.f, 1.f, 0.f};
}

ShadeResult RustShader::evaluate(const ShadeContext& ctx) const {
    float n = (fbm_helper(ctx.point, 4, 2.f) + 1.f) * 0.5f;
    float t = n < amount ? n / amount : 1.f;
    Vec3 c = metal * (1.f - t) + rust * t;
    return {c, 0.4f + 0.5f * t, 0.8f * (1.f - t), 1.f, 0.f};
}

ShadeResult GoldShader::evaluate(const ShadeContext&) const {
    return {color, 0.15f, 1.f, 1.f, 0.f};
}

ShadeResult SilverShader::evaluate(const ShadeContext&) const {
    return {color, 0.08f, 1.f, 1.f, 0.f};
}

void register_extra_shaders(ShaderLibrary& lib) {
    lib.register_shader(std::make_unique<MarbleShader>());
    lib.register_shader(std::make_unique<GraniteShader>());
    lib.register_shader(std::make_unique<BrickShader>());
    lib.register_shader(std::make_unique<FireShader>());
    lib.register_shader(std::make_unique<WaterShader>());
    lib.register_shader(std::make_unique<FabricShader>());
    lib.register_shader(std::make_unique<LeatherShader>());
    lib.register_shader(std::make_unique<BumpShader>());
    lib.register_shader(std::make_unique<DotsShader>());
    lib.register_shader(std::make_unique<StripesShader>());
    lib.register_shader(std::make_unique<VoronoiShader>());
    lib.register_shader(std::make_unique<PlasmaShader>());
    lib.register_shader(std::make_unique<FresnelShader>());
    lib.register_shader(std::make_unique<HatchingShader>());
    lib.register_shader(std::make_unique<RustShader>());
    lib.register_shader(std::make_unique<GoldShader>());
    lib.register_shader(std::make_unique<SilverShader>());
}

}  // namespace smidr
