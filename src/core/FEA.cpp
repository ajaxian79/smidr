#include "core/FEA.h"
#include "core/Analysis.h"

#include <algorithm>
#include <cmath>

namespace smidr {

float Tetrahedron::volume() const {
    Vec3 a = v[1] - v[0];
    Vec3 b = v[2] - v[0];
    Vec3 c = v[3] - v[0];
    return std::abs(a.dot(b.cross(c))) / 6.f;
}

Vec3 Tetrahedron::centroid() const {
    return (v[0] + v[1] + v[2] + v[3]) * 0.25f;
}

float Tetrahedron::aspect_ratio() const {
    float max_edge = 0.f;
    for (int i = 0; i < 4; ++i)
        for (int j = i + 1; j < 4; ++j) {
            float len = (v[j] - v[i]).length();
            max_edge = std::max(max_edge, len);
        }
    float vol = volume();
    if (vol < 1e-10f) return 1e10f;
    float ideal = std::pow(vol * 6.f * std::sqrt(2.f), 1.f/3.f);
    return max_edge / std::max(ideal, 1e-6f);
}

Tetrahedron TetMesh::get_tet(int i) const {
    Tetrahedron t;
    for (int j = 0; j < 4; ++j) {
        t.v[j] = vertices[indices[static_cast<size_t>(i*4 + j)]];
    }
    return t;
}

TetMesh tetrahedralize_aabb_grid(const Mesh& boundary, int resolution) {
    TetMesh tm;
    AABB bb = boundary.bounds();
    Vec3 size = bb.max_pt - bb.min_pt;
    Vec3 step{size.x / resolution, size.y / resolution, size.z / resolution};

    auto idx = [&](int x, int y, int z) {
        return static_cast<uint32_t>((z*(resolution+1) + y)*(resolution+1) + x);
    };

    for (int z = 0; z <= resolution; ++z)
        for (int y = 0; y <= resolution; ++y)
            for (int x = 0; x <= resolution; ++x) {
                Vec3 p{bb.min_pt.x + step.x * x,
                       bb.min_pt.y + step.y * y,
                       bb.min_pt.z + step.z * z};
                tm.vertices.push_back(p);
            }

    for (int z = 0; z < resolution; ++z)
        for (int y = 0; y < resolution; ++y)
            for (int x = 0; x < resolution; ++x) {
                uint32_t v[8];
                v[0] = idx(x, y, z);
                v[1] = idx(x+1, y, z);
                v[2] = idx(x+1, y+1, z);
                v[3] = idx(x, y+1, z);
                v[4] = idx(x, y, z+1);
                v[5] = idx(x+1, y, z+1);
                v[6] = idx(x+1, y+1, z+1);
                v[7] = idx(x, y+1, z+1);

                Vec3 cube_center{(bb.min_pt.x + step.x * (x + 0.5f)),
                                  (bb.min_pt.y + step.y * (y + 0.5f)),
                                  (bb.min_pt.z + step.z * (z + 0.5f))};
                if (!point_inside_closed_mesh(cube_center, boundary)) continue;

                int t[5][4] = {
                    {0, 1, 3, 4},
                    {1, 2, 3, 6},
                    {3, 4, 6, 7},
                    {1, 4, 5, 6},
                    {1, 3, 4, 6},
                };
                for (auto& tt : t) {
                    for (int i = 0; i < 4; ++i) tm.indices.push_back(v[tt[i]]);
                }
            }

    tm.quality.resize(static_cast<size_t>(tm.tet_count()));
    for (int i = 0; i < tm.tet_count(); ++i) {
        tm.quality[static_cast<size_t>(i)] = tm.get_tet(i).aspect_ratio();
    }
    return tm;
}

FEAQuality compute_fea_quality(const TetMesh& tet) {
    FEAQuality q{};
    q.total_tets = tet.tet_count();
    q.min_volume = q.min_aspect = 1e30f;
    q.max_volume = q.max_aspect = -1e30f;
    double sum_vol = 0, sum_aspect = 0;
    for (int i = 0; i < tet.tet_count(); ++i) {
        auto t = tet.get_tet(i);
        float v = t.volume();
        float a = t.aspect_ratio();
        if (v < 1e-10f) q.degenerate_tets++;
        Vec3 ab = t.v[1] - t.v[0], ac = t.v[2] - t.v[0], ad = t.v[3] - t.v[0];
        if (ab.dot(ac.cross(ad)) < 0) q.inverted_tets++;
        q.min_volume = std::min(q.min_volume, v);
        q.max_volume = std::max(q.max_volume, v);
        q.min_aspect = std::min(q.min_aspect, a);
        q.max_aspect = std::max(q.max_aspect, a);
        sum_vol += v; sum_aspect += a;
    }
    if (q.total_tets > 0) {
        q.avg_volume = static_cast<float>(sum_vol) / static_cast<float>(q.total_tets);
        q.avg_aspect = static_cast<float>(sum_aspect) / static_cast<float>(q.total_tets);
    }
    return q;
}

}  // namespace smidr
