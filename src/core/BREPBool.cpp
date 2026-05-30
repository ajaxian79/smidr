#include "core/BREPBool.h"
#include "core/CSGEval.h"
#include "core/RayCast.h"

namespace smidr {

bool point_inside_brep(Vec3 p, const BREPSolid& solid, int tessellation) {
    Mesh m = solid.tessellate(tessellation);
    Ray ray{p, {0.3713f, 0.7427f, 0.5570f}};
    int hits = 0;
    for (size_t i = 0; i + 2 < m.indices.size(); i += 3) {
        Vec3 v0 = m.vertices[m.indices[i]].position;
        Vec3 v1 = m.vertices[m.indices[i+1]].position;
        Vec3 v2 = m.vertices[m.indices[i+2]].position;
        if (ray_triangle(ray, v0, v1, v2) > 0.f) ++hits;
    }
    return (hits % 2) == 1;
}

static BREPSolid combine_faces(const BREPSolid& a, const BREPSolid& b,
                                 bool keep_a_inside, bool keep_a_outside,
                                 bool keep_b_inside, bool keep_b_outside,
                                 bool flip_b_inside) {
    BREPSolid result;
    constexpr int test_samples = 4;

    auto face_inside = [&](const BREPFace& f, const BREPSolid& other) {
        if (f.surface.knots_u.empty()) return false;
        float u_min = f.surface.knots_u[f.surface.degree_u];
        float u_max = f.surface.knots_u[f.surface.knots_u.size() - f.surface.degree_u - 1];
        float v_min = f.surface.knots_v[f.surface.degree_v];
        float v_max = f.surface.knots_v[f.surface.knots_v.size() - f.surface.degree_v - 1];
        int inside_count = 0;
        for (int i = 0; i <= test_samples; ++i) {
            for (int j = 0; j <= test_samples; ++j) {
                float u = u_min + (u_max - u_min) * static_cast<float>(i) / static_cast<float>(test_samples);
                float v = v_min + (v_max - v_min) * static_cast<float>(j) / static_cast<float>(test_samples);
                Vec3 p = f.surface.evaluate(u, v);
                if (point_inside_brep(p, other)) ++inside_count;
            }
        }
        int total = (test_samples + 1) * (test_samples + 1);
        return inside_count * 2 > total;
    };

    for (auto& face : a.faces) {
        bool inside = face_inside(face, b);
        if ((inside && keep_a_inside) || (!inside && keep_a_outside)) {
            result.faces.push_back(face);
        }
    }
    for (auto& face : b.faces) {
        bool inside = face_inside(face, a);
        if ((inside && keep_b_inside) || (!inside && keep_b_outside)) {
            BREPFace f = face;
            if (inside && flip_b_inside) {
                f.reversed = !f.reversed;
            }
            result.faces.push_back(f);
        }
    }
    return result;
}

BREPSolid brep_union(const BREPSolid& a, const BREPSolid& b) {
    return combine_faces(a, b,
        false, true,
        false, true,
        false);
}

BREPSolid brep_intersect(const BREPSolid& a, const BREPSolid& b) {
    return combine_faces(a, b,
        true, false,
        true, false,
        false);
}

BREPSolid brep_subtract(const BREPSolid& a, const BREPSolid& b) {
    return combine_faces(a, b,
        false, true,
        true, false,
        true);
}

}  // namespace smidr
