#include "core/BREPIntersect.h"

#include <algorithm>
#include <cmath>

namespace smidr {

bool aabb_intersect(const AABB& a, const AABB& b) {
    if (a.max_pt.x < b.min_pt.x || b.max_pt.x < a.min_pt.x) return false;
    if (a.max_pt.y < b.min_pt.y || b.max_pt.y < a.min_pt.y) return false;
    if (a.max_pt.z < b.min_pt.z || b.max_pt.z < a.min_pt.z) return false;
    return true;
}

AABB surface_bounds(const NURBSSurface& s) {
    AABB bb;
    for (auto& cp : s.control_points) bb.expand(cp);
    return bb;
}

static AABB surface_patch_bounds(const NURBSSurface& s, float u0, float u1,
                                  float v0, float v1, int samples = 4) {
    AABB bb;
    for (int i = 0; i <= samples; ++i) {
        float u = u0 + (u1 - u0) * static_cast<float>(i) / static_cast<float>(samples);
        for (int j = 0; j <= samples; ++j) {
            float v = v0 + (v1 - v0) * static_cast<float>(j) / static_cast<float>(samples);
            bb.expand(s.evaluate(u, v));
        }
    }
    return bb;
}

std::vector<ParametricPoint> find_seed_points(const NURBSSurface& a,
                                                const NURBSSurface& b,
                                                int subdivision) {
    std::vector<ParametricPoint> seeds;
    if (a.knots_u.empty() || a.knots_v.empty() ||
        b.knots_u.empty() || b.knots_v.empty()) return seeds;

    float au0 = a.knots_u[a.degree_u], au1 = a.knots_u[a.knots_u.size() - a.degree_u - 1];
    float av0 = a.knots_v[a.degree_v], av1 = a.knots_v[a.knots_v.size() - a.degree_v - 1];
    float bu0 = b.knots_u[b.degree_u], bu1 = b.knots_u[b.knots_u.size() - b.degree_u - 1];
    float bv0 = b.knots_v[b.degree_v], bv1 = b.knots_v[b.knots_v.size() - b.degree_v - 1];

    for (int i = 0; i <= subdivision; ++i) {
        float ua = au0 + (au1 - au0) * static_cast<float>(i) / static_cast<float>(subdivision);
        for (int j = 0; j <= subdivision; ++j) {
            float va = av0 + (av1 - av0) * static_cast<float>(j) / static_cast<float>(subdivision);
            Vec3 pa = a.evaluate(ua, va);

            float best_dist = 1e30f;
            ParametricPoint best;
            best.u_a = ua; best.v_a = va; best.position = pa;

            for (int k = 0; k <= subdivision; ++k) {
                float ub = bu0 + (bu1 - bu0) * static_cast<float>(k) / static_cast<float>(subdivision);
                for (int l = 0; l <= subdivision; ++l) {
                    float vb = bv0 + (bv1 - bv0) * static_cast<float>(l) / static_cast<float>(subdivision);
                    Vec3 pb = b.evaluate(ub, vb);
                    Vec3 d = pb - pa;
                    float dist = d.dot(d);
                    if (dist < best_dist) {
                        best_dist = dist;
                        best.u_b = ub; best.v_b = vb;
                    }
                }
            }

            if (best_dist < 0.01f) seeds.push_back(best);
        }
    }
    return seeds;
}

static Vec3 surface_point(const NURBSSurface& s, float u, float v) {
    return s.evaluate(u, v);
}

static void refine_intersection(const NURBSSurface& a, const NURBSSurface& b,
                                  ParametricPoint& p, int iters = 8) {
    for (int i = 0; i < iters; ++i) {
        Vec3 pa = a.evaluate(p.u_a, p.v_a);
        Vec3 pb = b.evaluate(p.u_b, p.v_b);
        Vec3 d = pb - pa;
        if (d.dot(d) < 1e-12f) break;

        Vec3 na = a.normal(p.u_a, p.v_a);
        Vec3 nb = b.normal(p.u_b, p.v_b);

        float t_a = d.dot(na) * 0.1f;
        float t_b = -d.dot(nb) * 0.1f;
        p.u_a += t_a * 0.5f;
        p.v_a += t_a * 0.5f;
        p.u_b += t_b * 0.5f;
        p.v_b += t_b * 0.5f;
    }
    p.position = (a.evaluate(p.u_a, p.v_a) + b.evaluate(p.u_b, p.v_b)) * 0.5f;
}

SurfaceIntersection march_intersection(const NURBSSurface& a, const NURBSSurface& b,
                                         ParametricPoint seed, float step,
                                         int max_steps) {
    SurfaceIntersection result;
    refine_intersection(a, b, seed);
    result.points.push_back(seed.position);

    ParametricPoint current = seed;
    for (int s = 0; s < max_steps; ++s) {
        Vec3 na = a.normal(current.u_a, current.v_a);
        Vec3 nb = b.normal(current.u_b, current.v_b);
        Vec3 march_dir = na.cross(nb).normalized();

        Vec3 next_pos = current.position + march_dir * step;

        ParametricPoint next = current;
        Vec3 delta_a = next_pos - a.evaluate(current.u_a, current.v_a);
        next.u_a += delta_a.x * step;
        next.v_a += delta_a.y * step;
        Vec3 delta_b = next_pos - b.evaluate(current.u_b, current.v_b);
        next.u_b += delta_b.x * step;
        next.v_b += delta_b.y * step;

        refine_intersection(a, b, next);

        Vec3 diff = next.position - seed.position;
        if (s > 4 && diff.dot(diff) < step * step) {
            result.closed = true;
            break;
        }

        if (next.u_a < 0 || next.u_a > 1 || next.v_a < 0 || next.v_a > 1 ||
            next.u_b < 0 || next.u_b > 1 || next.v_b < 0 || next.v_b > 1) {
            break;
        }

        result.points.push_back(next.position);
        current = next;
    }

    return result;
}

std::vector<SurfaceIntersection> intersect_surfaces(const NURBSSurface& a,
                                                      const NURBSSurface& b,
                                                      int subdivision) {
    std::vector<SurfaceIntersection> intersections;

    AABB ba = surface_bounds(a), bb = surface_bounds(b);
    if (!aabb_intersect(ba, bb)) return intersections;

    auto seeds = find_seed_points(a, b, subdivision);
    if (seeds.empty()) return intersections;

    std::vector<bool> used(seeds.size(), false);
    for (size_t i = 0; i < seeds.size(); ++i) {
        if (used[i]) continue;
        used[i] = true;
        auto si = march_intersection(a, b, seeds[i]);
        if (si.points.size() < 2) continue;

        for (size_t j = i + 1; j < seeds.size(); ++j) {
            if (used[j]) continue;
            for (auto& p : si.points) {
                Vec3 d = p - seeds[j].position;
                if (d.dot(d) < 0.01f) { used[j] = true; break; }
            }
        }
        intersections.push_back(si);
    }
    return intersections;
}

}  // namespace smidr
