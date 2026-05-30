#include "render/RayTracer.h"
#include "render/Camera.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <future>
#include <mutex>
#include <numeric>
#include <thread>

namespace smidr {

static float fast_rand(uint32_t& s) {
    s = s * 1664525u + 1013904223u;
    return static_cast<float>(s & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
}

void RayTracer::build_scene(const MeshGenerator& meshes, const MaterialLibrary& mats) {
    triangles_.clear();
    bvh_.clear();

    for (auto& entry : meshes.entries()) {
        auto& m = entry.mesh;
        const Material* mat = mats.find(kDefaultMaterial);
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
            tri.emission = 0.f;
            triangles_.push_back(tri);
        }
    }

    if (!triangles_.empty())
        build_bvh();
}

void RayTracer::add_light(const RTLight& light) { lights_.push_back(light); }
void RayTracer::clear_lights() { lights_.clear(); }

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
                best.emission = tri.emission;
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

Vec3 RayTracer::sample_direct_lighting(const RTHit& hit, Vec3 view_dir,
                                         const RayTraceSettings& settings,
                                         uint32_t& rng) const {
    Vec3 result{0, 0, 0};
    Vec3 n = hit.normal;
    if (n.dot(view_dir) > 0) n = -n;

    int samples = settings.shadow_samples;

    for (auto& light : lights_) {
        Vec3 accum{0, 0, 0};

        for (int s = 0; s < samples; ++s) {
            Vec3 light_pos = light.position;
            if (light.shape == LightShape::Area && light.radius > 0.f) {
                light_pos.x += (fast_rand(rng) * 2.f - 1.f) * light.radius;
                light_pos.y += (fast_rand(rng) * 2.f - 1.f) * light.radius;
                light_pos.z += (fast_rand(rng) * 2.f - 1.f) * light.radius;
            }

            Vec3 L;
            float light_dist = 1e30f;
            if (light.shape == LightShape::Directional) {
                L = (light.direction * -1.f).normalized();
            } else {
                Vec3 to_light = light_pos - hit.point;
                light_dist = to_light.length();
                L = to_light * (1.f / light_dist);
            }

            Ray shadow_ray{hit.point + n * 0.001f, L};
            RTHit shadow_hit = trace_ray(shadow_ray);
            if (shadow_hit.valid() && shadow_hit.t < light_dist) continue;

            float diff = std::max(0.f, n.dot(L));
            accum += hit.color * (light.color * light.intensity * diff);

            Vec3 H = (L + view_dir * -1.f).normalized();
            float spec_exp = 2.f / (hit.roughness * hit.roughness + 0.001f);
            float spec = std::pow(std::max(0.f, n.dot(H)), spec_exp);
            Vec3 spec_color = hit.metallic > 0.5f ? hit.color : Vec3{1, 1, 1};
            accum += spec_color * (spec * 0.3f * light.intensity);
        }

        result += accum * (1.f / static_cast<float>(samples));
    }

    return result;
}

Vec3 RayTracer::photon_gather(const RTHit& hit, const RayTraceSettings& settings) const {
    if (photons_.empty()) return {0, 0, 0};

    float r2 = settings.photon_search_r * settings.photon_search_r;
    Vec3 accum{0, 0, 0};
    int found = 0;

    for (auto& p : photons_) {
        Vec3 d = p.position - hit.point;
        float dist2 = d.dot(d);
        if (dist2 < r2) {
            float weight = 1.f - std::sqrt(dist2) / settings.photon_search_r;
            accum += p.power * weight;
            ++found;
            if (found >= settings.photon_search_n) break;
        }
    }

    float area = kPi * r2;
    return accum * (1.f / area);
}

Vec3 RayTracer::shade(const Ray& ray, const RTHit& hit, int depth,
                      const RayTraceSettings& settings, uint32_t& rng) const {
    if (!hit.valid()) return settings.background;

    if (hit.emission > 0.f) return hit.color * hit.emission;

    Vec3 n = hit.normal;
    if (n.dot(ray.direction) > 0) n = -n;

    Vec3 result = hit.color * settings.ambient;
    result += sample_direct_lighting(hit, ray.direction, settings, rng);

    if (settings.enable_photons && !photons_.empty()) {
        result += hit.color * photon_gather(hit, settings);
    }

    if (depth < settings.max_depth && hit.metallic > 0.1f) {
        Vec3 refl_dir = ray.direction - n * (2.f * ray.direction.dot(n));
        Ray refl_ray{hit.point + n * 0.001f, refl_dir.normalized()};
        RTHit refl_hit = trace_ray(refl_ray);
        Vec3 refl_color = shade(refl_ray, refl_hit, depth + 1, settings, rng);
        float fresnel = hit.metallic * 0.5f;
        result = result * (1.f - fresnel) + refl_color * fresnel;
    }

    if (depth < settings.max_depth && hit.opacity < 0.99f) {
        float eta = 1.f / 1.5f;
        float cos_i = -n.dot(ray.direction);
        float k = 1.f - eta * eta * (1.f - cos_i * cos_i);
        if (k > 0.f) {
            Vec3 refr_dir = ray.direction * eta + n * (eta * cos_i - std::sqrt(k));
            Ray refr_ray{hit.point - n * 0.001f, refr_dir.normalized()};
            RTHit refr_hit = trace_ray(refr_ray);
            Vec3 refr_color = shade(refr_ray, refr_hit, depth + 1, settings, rng);
            result = result * hit.opacity + refr_color * (1.f - hit.opacity);
        }
    }

    return result;
}

void RayTracer::emit_photons(int count) {
    photons_.clear();
    if (lights_.empty() || triangles_.empty()) return;

    uint32_t rng = 12345u;
    int per_light = std::max(1, count / static_cast<int>(lights_.size()));

    for (auto& light : lights_) {
        Vec3 power = light.color * (light.intensity / static_cast<float>(per_light));

        for (int i = 0; i < per_light; ++i) {
            Vec3 dir;
            float z = 1.f - 2.f * fast_rand(rng);
            float r = std::sqrt(std::max(0.f, 1.f - z * z));
            float phi = kTwoPi * fast_rand(rng);
            dir = {r * std::cos(phi), z, r * std::sin(phi)};

            Ray ray{light.position, dir};
            for (int bounce = 0; bounce < 4; ++bounce) {
                RTHit hit = trace_ray(ray);
                if (!hit.valid()) break;
                if (bounce > 0) {
                    photons_.push_back({hit.point, ray.direction, power * hit.color});
                }
                Vec3 n = hit.normal;
                if (n.dot(ray.direction) > 0) n = -n;
                Vec3 refl = ray.direction - n * (2.f * ray.direction.dot(n));
                Vec3 random_off{(fast_rand(rng) - 0.5f) * 0.3f,
                                 (fast_rand(rng) - 0.5f) * 0.3f,
                                 (fast_rand(rng) - 0.5f) * 0.3f};
                ray.origin = hit.point + n * 0.001f;
                ray.direction = (refl + random_off).normalized();
                power = power * hit.color * 0.7f;
                if (power.x + power.y + power.z < 0.01f) break;
            }
        }
    }
}

void RayTracer::render_tile(int x0, int y0, int x1, int y1,
                             const Camera& cam, const RayTraceSettings& settings,
                             std::vector<uint8_t>& pixels) const {
    int w = settings.width, h = settings.height;
    float aspect = static_cast<float>(w) / static_cast<float>(h);
    Mat4 view = cam.view_matrix();
    Mat4 proj = cam.projection_matrix(aspect);

    uint32_t rng = static_cast<uint32_t>(y0 * w + x0) * 2654435761u + 1u;

    for (int y = y0; y < y1; ++y) {
        for (int x = x0; x < x1; ++x) {
            Vec3 accum{0, 0, 0};
            int spp = settings.samples_per_pixel;

            for (int s = 0; s < spp; ++s) {
                float jx = spp > 1 ? fast_rand(rng) : 0.5f;
                float jy = spp > 1 ? fast_rand(rng) : 0.5f;
                float sx = static_cast<float>(x) + jx;
                float sy = static_cast<float>(y) + jy;

                Ray ray = screen_to_ray(sx, sy, static_cast<float>(w),
                                         static_cast<float>(h), view, proj);
                RTHit hit = trace_ray(ray);
                accum += shade(ray, hit, 0, settings, rng);
            }
            accum = accum * (1.f / static_cast<float>(spp));

            if (settings.gamma_correct) {
                accum.x = std::pow(std::max(0.f, accum.x), 1.f / 2.2f);
                accum.y = std::pow(std::max(0.f, accum.y), 1.f / 2.2f);
                accum.z = std::pow(std::max(0.f, accum.z), 1.f / 2.2f);
            }
            accum.x = std::min(accum.x, 1.f);
            accum.y = std::min(accum.y, 1.f);
            accum.z = std::min(accum.z, 1.f);

            size_t idx = static_cast<size_t>((y * w + x) * 3);
            pixels[idx]     = static_cast<uint8_t>(accum.x * 255.f);
            pixels[idx + 1] = static_cast<uint8_t>(accum.y * 255.f);
            pixels[idx + 2] = static_cast<uint8_t>(accum.z * 255.f);
        }
    }
}

std::vector<uint8_t> RayTracer::render(const Camera& cam,
                                        const RayTraceSettings& settings) {
    int w = settings.width, h = settings.height;
    std::vector<uint8_t> pixels(static_cast<size_t>(w * h * 3));

    if (lights_.empty()) {
        lights_.push_back({LightShape::Area, {5, 8, 6}, {0, -1, 0}, {1, 1, 1}, 0.8f, 0.5f});
        lights_.push_back({LightShape::Point, {-3, 4, -2}, {0, -1, 0}, {0.6f, 0.7f, 0.8f}, 0.4f, 0.f});
    }

    if (settings.enable_photons && photons_.empty()) {
        emit_photons(settings.photon_count);
    }

    int num_threads = settings.num_threads > 0 ? settings.num_threads
                                                : static_cast<int>(std::thread::hardware_concurrency());
    if (num_threads < 1) num_threads = 1;

    struct Tile { int x0, y0, x1, y1; };
    std::vector<Tile> tiles;
    int ts = settings.tile_size;
    for (int y = 0; y < h; y += ts)
        for (int x = 0; x < w; x += ts)
            tiles.push_back({x, y, std::min(x + ts, w), std::min(y + ts, h)});

    std::atomic<size_t> next_tile{0};
    std::vector<std::thread> workers;

    for (int t = 0; t < num_threads; ++t) {
        workers.emplace_back([&]() {
            while (true) {
                size_t idx = next_tile.fetch_add(1);
                if (idx >= tiles.size()) break;
                auto& tile = tiles[idx];
                render_tile(tile.x0, tile.y0, tile.x1, tile.y1, cam, settings, pixels);
            }
        });
    }
    for (auto& w_th : workers) w_th.join();

    return pixels;
}

}  // namespace smidr
