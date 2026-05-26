#include "core/Analysis.h"
#include "core/RayCast.h"

#include <cmath>

namespace smidr {

static Vec3 tri_v(const Mesh& m, size_t idx) {
    return m.vertices[m.indices[idx]].position;
}

float compute_volume(const Mesh& mesh) {
    float vol = 0.f;
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        Vec3 a = tri_v(mesh, i), b = tri_v(mesh, i+1), c = tri_v(mesh, i+2);
        vol += a.dot(b.cross(c));
    }
    return std::abs(vol) / 6.f;
}

float compute_surface_area(const Mesh& mesh) {
    float area = 0.f;
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        Vec3 a = tri_v(mesh, i), b = tri_v(mesh, i+1), c = tri_v(mesh, i+2);
        area += (b - a).cross(c - a).length();
    }
    return area * 0.5f;
}

Vec3 compute_centroid(const Mesh& mesh) {
    Vec3 centroid{0, 0, 0};
    float total_vol = 0.f;
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        Vec3 a = tri_v(mesh, i), b = tri_v(mesh, i+1), c = tri_v(mesh, i+2);
        float tet_vol = a.dot(b.cross(c)) / 6.f;
        centroid += (a + b + c) * (0.25f * tet_vol);
        total_vol += tet_vol;
    }
    if (std::abs(total_vol) > 1e-10f)
        centroid = centroid * (1.f / total_vol);
    return centroid;
}

MomentsOfInertia compute_moments(const Mesh& mesh, Vec3 cm, float density) {
    MomentsOfInertia moi{};
    float total_vol = 0.f;

    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        Vec3 a = tri_v(mesh, i) - cm;
        Vec3 b = tri_v(mesh, i+1) - cm;
        Vec3 c = tri_v(mesh, i+2) - cm;

        float det = a.dot(b.cross(c));
        total_vol += det;

        auto sq = [](float x) { return x * x; };

        moi.Ixx += det * (sq(a.y) + a.y*b.y + sq(b.y) + a.y*c.y + b.y*c.y + sq(c.y)
                        + sq(a.z) + a.z*b.z + sq(b.z) + a.z*c.z + b.z*c.z + sq(c.z));
        moi.Iyy += det * (sq(a.x) + a.x*b.x + sq(b.x) + a.x*c.x + b.x*c.x + sq(c.x)
                        + sq(a.z) + a.z*b.z + sq(b.z) + a.z*c.z + b.z*c.z + sq(c.z));
        moi.Izz += det * (sq(a.x) + a.x*b.x + sq(b.x) + a.x*c.x + b.x*c.x + sq(c.x)
                        + sq(a.y) + a.y*b.y + sq(b.y) + a.y*c.y + b.y*c.y + sq(c.y));

        moi.Ixy -= det * (2*a.x*a.y + b.x*a.y + c.x*a.y + a.x*b.y + 2*b.x*b.y
                         + c.x*b.y + a.x*c.y + b.x*c.y + 2*c.x*c.y);
        moi.Ixz -= det * (2*a.x*a.z + b.x*a.z + c.x*a.z + a.x*b.z + 2*b.x*b.z
                         + c.x*b.z + a.x*c.z + b.x*c.z + 2*c.x*c.z);
        moi.Iyz -= det * (2*a.y*a.z + b.y*a.z + c.y*a.z + a.y*b.z + 2*b.y*b.z
                         + c.y*b.z + a.y*c.z + b.y*c.z + 2*c.y*c.z);
    }

    float factor = density / 60.f;
    moi.Ixx *= factor; moi.Iyy *= factor; moi.Izz *= factor;
    moi.Ixy *= factor / 2.f; moi.Ixz *= factor / 2.f; moi.Iyz *= factor / 2.f;

    return moi;
}

bool point_inside_closed_mesh(Vec3 p, const Mesh& mesh) {
    Ray ray{p, {0.3713f, 0.7427f, 0.5570f}};
    int hits = 0;
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        Vec3 v0 = mesh.vertices[mesh.indices[i]].position;
        Vec3 v1 = mesh.vertices[mesh.indices[i+1]].position;
        Vec3 v2 = mesh.vertices[mesh.indices[i+2]].position;
        if (ray_triangle(ray, v0, v1, v2) > 0.f) ++hits;
    }
    return (hits % 2) == 1;
}

AABB compute_tight_bounds(const Mesh& mesh) {
    return mesh.bounds();
}

AnalysisResult analyze_mesh(const Mesh& mesh, float density) {
    AnalysisResult r;
    if (mesh.indices.empty()) return r;

    r.volume = compute_volume(mesh);
    r.surface_area = compute_surface_area(mesh);
    r.centroid = compute_centroid(mesh);
    r.mass = r.volume * density;

    auto moi = compute_moments(mesh, r.centroid, density);
    r.Ixx = moi.Ixx; r.Iyy = moi.Iyy; r.Izz = moi.Izz;
    r.Ixy = moi.Ixy; r.Ixz = moi.Ixz; r.Iyz = moi.Iyz;

    r.triangle_count = static_cast<int>(mesh.indices.size() / 3);
    r.vertex_count = static_cast<int>(mesh.vertices.size());
    r.valid = true;

    return r;
}

}  // namespace smidr
