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
};

struct RTHit {
    float t = -1.f;
    Vec3 point;
    Vec3 normal;
    Vec3 color;
    float roughness;
    float metallic;
    float opacity;
    bool valid() const { return t > 0.f; }
};

struct RTLight {
    Vec3 position;
    Vec3 color;
    float intensity;
};

struct RayTraceSettings {
    int width = 640;
    int height = 480;
    int max_depth = 4;
    int samples_per_pixel = 1;
    Vec3 background{0.05f, 0.05f, 0.07f};
    float ambient = 0.1f;
};

class RayTracer {
public:
    void build_scene(const MeshGenerator& meshes, const MaterialLibrary& mats);

    void add_light(const RTLight& light);

    std::vector<uint8_t> render(const Camera& cam, const RayTraceSettings& settings);

    RTHit trace_ray(const Ray& ray) const;

private:
    Vec3 shade(const Ray& ray, const RTHit& hit, int depth,
               const RayTraceSettings& settings) const;

    void build_bvh();
    RTHit intersect_bvh(const Ray& ray, int node_idx) const;

    std::vector<RTTriangle> triangles_;
    std::vector<BVHNode> bvh_;
    std::vector<RTLight> lights_;
    int bvh_root_ = -1;
};

}  // namespace smidr
