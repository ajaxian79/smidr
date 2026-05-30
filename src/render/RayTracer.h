#pragma once

#include "core/Math.h"
#include "core/RayCast.h"
#include "core/MeshGenerator.h"
#include "core/Material.h"
#include "render/Camera.h"

#include <cstdint>
#include <vector>

namespace smidr {

struct BVHNode {
    AABB bounds;
    int left = -1, right = -1;
    int tri_start = -1, tri_count = 0;
    bool is_leaf() const { return tri_count > 0; }
};

struct RTTriangle {
    Vec3 v0, v1, v2;
    Vec3 normal;
    Vec3 color;
    float roughness;
    float metallic;
    float opacity;
    float emission = 0.f;
};

struct RTHit {
    float t = -1.f;
    Vec3 point;
    Vec3 normal;
    Vec3 color;
    float roughness;
    float metallic;
    float opacity;
    float emission = 0.f;
    bool valid() const { return t > 0.f; }
};

enum class LightShape { Point, Directional, Area };

struct RTLight {
    LightShape shape = LightShape::Point;
    Vec3 position;
    Vec3 direction{0, -1, 0};
    Vec3 color;
    float intensity = 1.f;
    float radius = 0.f;
};

struct Photon {
    Vec3 position;
    Vec3 direction;
    Vec3 power;
};

struct RayTraceSettings {
    int width = 640;
    int height = 480;
    int max_depth = 4;
    int samples_per_pixel = 1;
    int num_threads = 0;          // 0 = use hardware concurrency
    int tile_size = 32;
    int shadow_samples = 1;       // for area lights
    bool enable_photons = false;
    int photon_count = 50000;
    int photon_search_n = 64;
    float photon_search_r = 0.5f;
    Vec3 background{0.05f, 0.05f, 0.07f};
    float ambient = 0.1f;
    bool gamma_correct = true;
};

class RayTracer {
public:
    void build_scene(const MeshGenerator& meshes, const MaterialLibrary& mats);

    void add_light(const RTLight& light);
    void clear_lights();

    void emit_photons(int count);

    std::vector<uint8_t> render(const Camera& cam, const RayTraceSettings& settings);

    RTHit trace_ray(const Ray& ray) const;

private:
    Vec3 shade(const Ray& ray, const RTHit& hit, int depth,
               const RayTraceSettings& settings, uint32_t& rng) const;

    Vec3 sample_direct_lighting(const RTHit& hit, Vec3 view_dir,
                                 const RayTraceSettings& settings,
                                 uint32_t& rng) const;

    Vec3 photon_gather(const RTHit& hit, const RayTraceSettings& settings) const;

    void build_bvh();
    RTHit intersect_bvh(const Ray& ray, int node_idx) const;

    void render_tile(int x0, int y0, int x1, int y1, const Camera& cam,
                     const RayTraceSettings& settings,
                     std::vector<uint8_t>& pixels) const;

    std::vector<RTTriangle> triangles_;
    std::vector<BVHNode> bvh_;
    std::vector<RTLight> lights_;
    std::vector<Photon>  photons_;
    int bvh_root_ = -1;
};

}  // namespace smidr
