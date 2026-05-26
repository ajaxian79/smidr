#include "render/Shaders.h"

#include <cmath>

namespace smidr {

static float fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }
static float lerp_f(float a, float b, float t) { return a + t * (b - a); }

static const int perm[512] = {
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,
    140,36,103,30,69,142,8,99,37,240,21,10,23,190,6,148,
    247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,
    57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,
    74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,
    60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54,
    65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,
    200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,
    52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,
    207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,
    119,248,152,2,44,154,163,70,221,153,101,155,167,43,172,9,
    129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,
    218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241,
    81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,157,
    184,84,204,176,115,121,50,45,127,4,150,254,138,236,205,93,
    222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,
    140,36,103,30,69,142,8,99,37,240,21,10,23,190,6,148,
    247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,
    57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,
    74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,
    60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54,
    65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,
    200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,
    52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,
    207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,
    119,248,152,2,44,154,163,70,221,153,101,155,167,43,172,9,
    129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,
    218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241,
    81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,157,
    184,84,204,176,115,121,50,45,127,4,150,254,138,236,205,93,
    222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
};

static float grad(int hash, float x, float y, float z) {
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

float perlin_noise_3d(float x, float y, float z) {
    int X = static_cast<int>(std::floor(x)) & 255;
    int Y = static_cast<int>(std::floor(y)) & 255;
    int Z = static_cast<int>(std::floor(z)) & 255;
    x -= std::floor(x); y -= std::floor(y); z -= std::floor(z);
    float u = fade(x), v = fade(y), w = fade(z);
    int A = perm[X]+Y, AA = perm[A]+Z, AB = perm[A+1]+Z;
    int B = perm[X+1]+Y, BA = perm[B]+Z, BB = perm[B+1]+Z;
    return lerp_f(lerp_f(lerp_f(grad(perm[AA],x,y,z), grad(perm[BA],x-1,y,z),u),
                         lerp_f(grad(perm[AB],x,y-1,z), grad(perm[BB],x-1,y-1,z),u),v),
                  lerp_f(lerp_f(grad(perm[AA+1],x,y,z-1), grad(perm[BA+1],x-1,y,z-1),u),
                         lerp_f(grad(perm[AB+1],x,y-1,z-1), grad(perm[BB+1],x-1,y-1,z-1),u),v),w);
}

static float fbm(Vec3 p, int octaves, float scale) {
    float val = 0.f, amp = 1.f, freq = scale;
    for (int i = 0; i < octaves; ++i) {
        val += amp * perlin_noise_3d(p.x * freq, p.y * freq, p.z * freq);
        amp *= 0.5f; freq *= 2.f;
    }
    return val;
}

ShadeResult PlasticShader::evaluate(const ShadeContext&) const {
    return {color, 0.5f, 0.f, 1.f, 0.f};
}
std::unique_ptr<SurfaceShader> PlasticShader::clone() const { return std::make_unique<PlasticShader>(*this); }

ShadeResult FlatShader::evaluate(const ShadeContext&) const {
    return {color, 1.f, 0.f, 1.f, 0.f};
}
std::unique_ptr<SurfaceShader> FlatShader::clone() const { return std::make_unique<FlatShader>(*this); }

ShadeResult CookTorranceShader::evaluate(const ShadeContext&) const {
    return {base_color, roughness, metallic, 1.f, 0.f};
}
std::unique_ptr<SurfaceShader> CookTorranceShader::clone() const { return std::make_unique<CookTorranceShader>(*this); }

ShadeResult CheckerShader::evaluate(const ShadeContext& ctx) const {
    float s = scale;
    int ix = static_cast<int>(std::floor(ctx.point.x * s));
    int iy = static_cast<int>(std::floor(ctx.point.y * s));
    int iz = static_cast<int>(std::floor(ctx.point.z * s));
    bool which = ((ix + iy + iz) & 1) == 0;
    return {which ? color_a : color_b, 0.5f, 0.f, 1.f, 0.f};
}
std::unique_ptr<SurfaceShader> CheckerShader::clone() const { return std::make_unique<CheckerShader>(*this); }

ShadeResult NoiseShader::evaluate(const ShadeContext& ctx) const {
    float n = (fbm(ctx.point, octaves, scale) + 1.f) * 0.5f;
    n = std::max(0.f, std::min(n, 1.f));
    Vec3 c = color_a * (1.f - n) + color_b * n;
    return {c, 0.6f, 0.f, 1.f, 0.f};
}
std::unique_ptr<SurfaceShader> NoiseShader::clone() const { return std::make_unique<NoiseShader>(*this); }

ShadeResult WoodShader::evaluate(const ShadeContext& ctx) const {
    float r = std::sqrt(ctx.point.x * ctx.point.x + ctx.point.z * ctx.point.z) * ring_scale;
    float grain = (std::sin(r + fbm(ctx.point, 3, 2.f) * 4.f) + 1.f) * 0.5f;
    Vec3 c = color_light * grain + color_dark * (1.f - grain);
    return {c, 0.7f, 0.f, 1.f, 0.f};
}
std::unique_ptr<SurfaceShader> WoodShader::clone() const { return std::make_unique<WoodShader>(*this); }

ShadeResult CamoShader::evaluate(const ShadeContext& ctx) const {
    float n = fbm(ctx.point, 4, scale);
    Vec3 c;
    if (n < -0.2f) c = color1;
    else if (n < 0.2f) c = color2;
    else c = color3;
    return {c, 0.8f, 0.f, 1.f, 0.f};
}
std::unique_ptr<SurfaceShader> CamoShader::clone() const { return std::make_unique<CamoShader>(*this); }

ShadeResult ToonShader::evaluate(const ShadeContext& ctx) const {
    float ndy = std::max(0.f, ctx.normal.dot(Vec3{0.4f, 0.8f, 0.6f}.normalized()));
    float step = std::floor(ndy * static_cast<float>(levels)) / static_cast<float>(levels);
    return {color * (0.2f + 0.8f * step), 1.f, 0.f, 1.f, 0.f};
}
std::unique_ptr<SurfaceShader> ToonShader::clone() const { return std::make_unique<ToonShader>(*this); }

ShadeResult CloudShader::evaluate(const ShadeContext& ctx) const {
    float n = (fbm(ctx.point, 5, 1.5f) + 1.f) * 0.5f;
    float cloud = std::max(0.f, std::min(n - (1.f - density), 1.f)) / density;
    Vec3 c = sky_color * (1.f - cloud) + cloud_color * cloud;
    return {c, 1.f, 0.f, 1.f, 0.f};
}
std::unique_ptr<SurfaceShader> CloudShader::clone() const { return std::make_unique<CloudShader>(*this); }

ShadeResult MirrorShader::evaluate(const ShadeContext&) const {
    return {tint, 0.02f, 1.f, 1.f, 0.f};
}
std::unique_ptr<SurfaceShader> MirrorShader::clone() const { return std::make_unique<MirrorShader>(*this); }

ShadeResult GlassShader::evaluate(const ShadeContext&) const {
    return {tint, 0.02f, 0.f, 0.2f, 0.f};
}
std::unique_ptr<SurfaceShader> GlassShader::clone() const { return std::make_unique<GlassShader>(*this); }

ShadeResult EmissionShader::evaluate(const ShadeContext&) const {
    return {color, 1.f, 0.f, 1.f, intensity};
}
std::unique_ptr<SurfaceShader> EmissionShader::clone() const { return std::make_unique<EmissionShader>(*this); }

ShaderLibrary::ShaderLibrary() {
    register_shader(std::make_unique<PlasticShader>());
    register_shader(std::make_unique<FlatShader>());
    register_shader(std::make_unique<CookTorranceShader>());
    register_shader(std::make_unique<CheckerShader>());
    register_shader(std::make_unique<NoiseShader>());
    register_shader(std::make_unique<WoodShader>());
    register_shader(std::make_unique<CamoShader>());
    register_shader(std::make_unique<ToonShader>());
    register_shader(std::make_unique<CloudShader>());
    register_shader(std::make_unique<MirrorShader>());
    register_shader(std::make_unique<GlassShader>());
    register_shader(std::make_unique<EmissionShader>());
}

void ShaderLibrary::register_shader(std::unique_ptr<SurfaceShader> shader) {
    std::string n = shader->name();
    shaders_[n] = std::move(shader);
}

const SurfaceShader* ShaderLibrary::get(const std::string& name) const {
    auto it = shaders_.find(name);
    return it != shaders_.end() ? it->second.get() : nullptr;
}

std::vector<std::string> ShaderLibrary::list() const {
    std::vector<std::string> names;
    names.reserve(shaders_.size());
    for (auto& [k, v] : shaders_) names.push_back(k);
    return names;
}

}  // namespace smidr
