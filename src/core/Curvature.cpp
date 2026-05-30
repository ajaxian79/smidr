#include "core/Curvature.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace smidr {

std::vector<VertexCurvature> compute_curvature(const Mesh& mesh) {
    std::vector<VertexCurvature> result(mesh.vertices.size());

    std::vector<float> area_sum(mesh.vertices.size(), 0.f);
    std::vector<float> angle_sum(mesh.vertices.size(), 0.f);
    std::vector<Vec3>  mean_curv_vec(mesh.vertices.size(), Vec3{0, 0, 0});

    for (size_t t = 0; t + 2 < mesh.indices.size(); t += 3) {
        uint32_t i0 = mesh.indices[t], i1 = mesh.indices[t+1], i2 = mesh.indices[t+2];
        Vec3 p0 = mesh.vertices[i0].position;
        Vec3 p1 = mesh.vertices[i1].position;
        Vec3 p2 = mesh.vertices[i2].position;

        Vec3 e01 = p1 - p0, e12 = p2 - p1, e20 = p0 - p2;
        Vec3 face_normal = e01.cross(p2 - p0);
        float face_area = face_normal.length() * 0.5f;
        if (face_area < 1e-10f) continue;

        auto angle_at = [](Vec3 va, Vec3 vb) {
            float c = va.normalized().dot(vb.normalized());
            c = std::max(-1.f, std::min(1.f, c));
            return std::acos(c);
        };
        float a0 = angle_at(e01, p2 - p0);
        float a1 = angle_at(-e01, e12);
        float a2 = angle_at(-e12, e20);

        angle_sum[i0] += a0;
        angle_sum[i1] += a1;
        angle_sum[i2] += a2;

        float voronoi = face_area / 3.f;
        area_sum[i0] += voronoi;
        area_sum[i1] += voronoi;
        area_sum[i2] += voronoi;

        float cot_a0 = 1.f / std::tan(a0);
        float cot_a1 = 1.f / std::tan(a1);
        float cot_a2 = 1.f / std::tan(a2);

        mean_curv_vec[i0] += (p1 - p0) * cot_a2 + (p2 - p0) * cot_a1;
        mean_curv_vec[i1] += (p2 - p1) * cot_a0 + (p0 - p1) * cot_a2;
        mean_curv_vec[i2] += (p0 - p2) * cot_a1 + (p1 - p2) * cot_a0;
    }

    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        float A = area_sum[i];
        if (A > 1e-10f) {
            result[i].gaussian = (2.f * kPi - angle_sum[i]) / A;
            Vec3 mcv = mean_curv_vec[i] * (1.f / (2.f * A));
            result[i].mean = mcv.length() * 0.5f;
            float disc = result[i].mean * result[i].mean - result[i].gaussian;
            float sq = disc > 0 ? std::sqrt(disc) : 0;
            result[i].k1 = result[i].mean + sq;
            result[i].k2 = result[i].mean - sq;
        }
    }
    return result;
}

static Vec3 hsv_to_rgb(float h, float s, float v) {
    int hi = static_cast<int>(std::floor(h / 60.f)) % 6;
    float f = h / 60.f - std::floor(h / 60.f);
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);
    switch (hi) {
        case 0: return {v, t, p};
        case 1: return {q, v, p};
        case 2: return {p, v, t};
        case 3: return {p, q, v};
        case 4: return {t, p, v};
        default: return {v, p, q};
    }
}

Mesh colorize_curvature(const Mesh& mesh, bool gaussian_not_mean) {
    Mesh out = mesh;
    auto curv = compute_curvature(mesh);
    float min_v = 1e30f, max_v = -1e30f;
    for (auto& c : curv) {
        float v = gaussian_not_mean ? c.gaussian : c.mean;
        min_v = std::min(min_v, v);
        max_v = std::max(max_v, v);
    }
    float range = max_v - min_v;
    if (range < 1e-10f) range = 1.f;
    for (size_t i = 0; i < out.vertices.size(); ++i) {
        float v = gaussian_not_mean ? curv[i].gaussian : curv[i].mean;
        float t = (v - min_v) / range;
        Vec3 color = hsv_to_rgb((1.f - t) * 240.f, 0.8f, 0.95f);
        out.vertices[i].normal = color;
    }
    return out;
}

CurvatureStats curvature_stats(const std::vector<VertexCurvature>& curv) {
    CurvatureStats s{};
    s.min_mean = s.min_gauss = 1e30f;
    s.max_mean = s.max_gauss = -1e30f;
    double sum_mean = 0, sum_gauss = 0;
    for (auto& c : curv) {
        s.min_mean = std::min(s.min_mean, c.mean);
        s.max_mean = std::max(s.max_mean, c.mean);
        s.min_gauss = std::min(s.min_gauss, c.gaussian);
        s.max_gauss = std::max(s.max_gauss, c.gaussian);
        sum_mean += c.mean; sum_gauss += c.gaussian;
        if (std::abs(c.mean) < 1e-3f) s.flat_vertices++;
        else if (c.mean > 0) s.convex_vertices++;
        else s.concave_vertices++;
    }
    s.avg_mean = curv.empty() ? 0 : static_cast<float>(sum_mean) / static_cast<float>(curv.size());
    s.avg_gauss = curv.empty() ? 0 : static_cast<float>(sum_gauss) / static_cast<float>(curv.size());
    return s;
}

}  // namespace smidr
