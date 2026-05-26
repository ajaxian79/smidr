#include "render/RayTracer.h"
#include "render/Camera.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace smidr {

void RayTracer::build_scene(const MeshGenerator& meshes, const MaterialLibrary& mats) {
    triangles_.clear();
    bvh_.clear();

    for (auto& entry : meshes.entries()) {
        auto& m = entry.mesh;
        const Material* mat = mats.find(kDefaultMaterial);
        Vec3 color = mat ? mat->base_color : entry.color;
        float rough = mat ? mat->roughness : 0.5f;
        float metal = mat ? mat->metallic : 0.f;

        for (size_t i = 0; i + 2 < m.indices.size(); i += 3) {
            RTTriangle tri;
            tri.v0 = entry.transform.transform_point(m.vertices[m.indices[i]].position);
            tri.v1 = entry.transform.transform_point(m.vertices[m.indices[i+1]].position);
            tri.v2 = entry.transform.transform_point(m.vertices[m.indices[i+2]].position);
            tri.normal = (tri.v1 - tri.v0).cross(tri.v2 - tri.v0).normalized();
            tri.color = entry.color;
            tri.roughness = rough;
            tri.metallic = metal;
            tri.opacity = entry.opacity;
            triangles_.push_back(tri);
        }
    }

    if (!triangles_.empty())
        build_bvh();
}

void RayTracer::add_light(const RTLight& light) {
    lights_.push_back(light);
}

static bool ray_aabb(const Ray& ray, const AABB& box, float& tmin) {
    float t1, t2;
    tmin = 0.f;
    float tmax = 1e30f;

    auto slab = [&](float origin, float dir, float bmin, float bmax) {
        if (std::abs(dir) < 1e-8f) {
            return origin >= bmin && origin <= bmax;
        }
        t1 = (bmin - origin) / dir;
        t2 = (bmax - origin) / dir;
        if (t1 > t2) std::swap(t1, t2);
        tmin = std::max(tmin, t1);
        tmax = std::min(tmax, t2);
        return tmin <= tmax;
    };

    if (!slab(ray.origin.x, ray.direction.x, box.min_pt.x, box.max_pt.x)) return false;
    if (!slab(ray.origin.y, ray.direction.y, box.min_pt.y, box.max_pt.y)) return false;
    if (!slab(ray.origin.z, ray.direction.z, box.min_pt.z, box.max_pt.z)) return false;
    return true;
}

void RayTracer::build_bvh() {
    std::vector<int> indices(triangles_.size());
    std::iota(indices.begin(), indices.end(), 0);

    struct BuildTask { int node; int start; int end; };
    std::vector<BuildTask> stack;

    bvh_.clear();
    bvh_.push_back({});
    bvh_root_ = 0;
    stack.push_back({0, 0, static_cast<int>(indices.size())});

    while (!stack.empty()) {
        auto [node_idx, start, end] = stack.back();
        stack.pop_back();
        auto& node = bvh_[node_idx];

        AABB bounds;
        for (int i = start; i < end; ++i) {
            auto& t = triangles_[indices[i]];
            bounds.expand(t.v0); bounds.expand(t.v1); bounds.expand(t.v2);
        }
        node.bounds = bounds;

        int count = end - start;
        if (count <= 4) {
            node.tri_start = start;
            node.tri_count = count;
            continue;
        }

        Vec3 ext = bounds.extent();
        int axis = (ext.x >= ext.y && ext.x >= ext.z) ? 0 : (ext.y >= ext.z ? 1 : 2);
        float mid_val = (axis == 0) ? bounds.center().x : (axis == 1 ? bounds.center().y : bounds.center().z);

        auto centroid_val = [&](int idx) -> float {
            auto& t = triangles_[idx];
            Vec3 c = (t.v0 + t.v1 + t.v2) * (1.f / 3.f);
            return axis == 0 ? c.x : (axis == 1 ? c.y : c.z);
        };

        auto it = std::partition(indices.begin() + start, indices.begin() + end,
            [&](int idx) { return centroid_val(idx) < mid_val; });
        int split = static_cast<int>(it - indices.begin());

        if (split == start || split == end) split = (start + end) / 2;

        int left_idx = static_cast<int>(bvh_.size());
        bvh_.push_back({});
        int right_idx = static_cast<int>(bvh_.size());
        bvh_.push_back({});

        node.left = left_idx;
        node.right = right_idx;

        stack.push_back({left_idx, start, split});
        stack.push_back({right_idx, split, end});
    }

    std::vector<RTTriangle> sorted(triangles_.size());
    for (size_t i = 0; i < indices.size(); ++i)
        sorted[i] = triangles_[indices[i]];
    triangles_ = std::move(sorted);
}

RTHit RayTracer::intersect_bvh(const Ray& ray, int node_idx) const {
    if (node_idx < 0 || node_idx >= static_cast<int>(bvh_.size())) return {};

    const auto& node = bvh_[node_idx];
    float tmin;
    if (!ray_aabb(ray, node.bounds, tmin)) return {};

    if (node.is_leaf()) {
        RTHit best;
        for (int i = node.tri_start; i < node.tri_start + node.tri_count; ++i) {
            auto& tri = triangles_[i];
            float t = ray_triangle(ray, tri.v0, tri.v1, tri.v2);
            if (t > 0.f && (!best.valid() || t < best.t)) {
                best.t = t;
                best.point = ray.origin + ray.direction * t;
                best.normal = tri.normal;
                best.color = tri.color;
                best.roughness = tri.roughness;
                best.metallic = tri.metallic;
                best.opacity = tri.opacity;
            }
        }
        return best;
    }

    RTHit left_hit = intersect_bvh(ray, node.left);
    RTHit right_hit = intersect_bvh(ray, node.right);

    if (left_hit.valid() && right_hit.valid())
        return left_hit.t < right_hit.t ? left_hit : right_hit;
    return left_hit.valid() ? left_hit : right_hit;
}

RTHit RayTracer::trace_ray(const Ray& ray) const {
    if (bvh_.empty()) return {};
    return intersect_bvh(ray, bvh_root_);
}

Vec3 RayTracer::shade(const Ray& ray, const RTHit& hit, int depth,
                      const RayTraceSettings& settings) const {
    if (!hit.valid()) return settings.background;

    Vec3 n = hit.normal;
    if (n.dot(ray.direction) > 0) n = -n;

    Vec3 result = hit.color * settings.ambient;

    for (auto& light : lights_) {
        Vec3 to_light = light.position - hit.point;
        float light_dist = to_light.length();
        Vec3 L = to_light * (1.f / light_dist);

        Ray shadow_ray{hit.point + n * 0.001f, L};
        RTHit shadow_hit = trace_ray(shadow_ray);
        if (shadow_hit.valid() && shadow_hit.t < light_dist) continue;

        float diff = std::max(0.f, n.dot(L));
        result += hit.color * (light.color * light.intensity * diff);

        Vec3 H = (L + (ray.direction * -1.f)).normalized();
        float spec_exp = 2.f / (hit.roughness * hit.roughness + 0.001f);
        float spec = std::pow(std::max(0.f, n.dot(H)), spec_exp);
        Vec3 spec_color = hit.metallic > 0.5f ? hit.color : Vec3{1, 1, 1};
        result += spec_color * (spec * 0.3f * light.intensity);
    }

    if (depth < settings.max_depth && hit.metallic > 0.1f) {
        Vec3 refl_dir = ray.direction - n * (2.f * ray.direction.dot(n));
        Ray refl_ray{hit.point + n * 0.001f, refl_dir.normalized()};
        RTHit refl_hit = trace_ray(refl_ray);
        Vec3 refl_color = shade(refl_ray, refl_hit, depth + 1, settings);
        float fresnel = hit.metallic * 0.5f;
        result = result * (1.f - fresnel) + refl_color * fresnel;
    }

    result.x = std::min(result.x, 1.f);
    result.y = std::min(result.y, 1.f);
    result.z = std::min(result.z, 1.f);
    return result;
}

std::vector<uint8_t> RayTracer::render(const Camera& cam,
                                        const RayTraceSettings& settings) {
    int w = settings.width, h = settings.height;
    std::vector<uint8_t> pixels(static_cast<size_t>(w * h * 3));

    if (lights_.empty()) {
        lights_.push_back({{5, 8, 6}, {1, 1, 1}, 0.8f});
        lights_.push_back({{-3, 4, -2}, {0.6f, 0.7f, 0.8f}, 0.4f});
    }

    float aspect = static_cast<float>(w) / static_cast<float>(h);
    Mat4 view = cam.view_matrix();
    Mat4 proj = cam.projection_matrix(aspect);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            Vec3 accum{0, 0, 0};
            int spp = settings.samples_per_pixel;

            for (int s = 0; s < spp; ++s) {
                float sx = static_cast<float>(x) + (spp > 1 ? (static_cast<float>(s % 2) + 0.5f) * 0.5f : 0.5f);
                float sy = static_cast<float>(y) + (spp > 1 ? (static_cast<float>(s / 2) + 0.5f) * 0.5f : 0.5f);

                Ray ray = screen_to_ray(sx, sy, static_cast<float>(w),
                                         static_cast<float>(h), view, proj);
                RTHit hit = trace_ray(ray);
                accum += shade(ray, hit, 0, settings);
            }

            accum = accum * (1.f / static_cast<float>(spp));

            size_t idx = static_cast<size_t>((y * w + x) * 3);
            pixels[idx]     = static_cast<uint8_t>(accum.x * 255.f);
            pixels[idx + 1] = static_cast<uint8_t>(accum.y * 255.f);
            pixels[idx + 2] = static_cast<uint8_t>(accum.z * 255.f);
        }
    }

    return pixels;
}

}  // namespace smidr
