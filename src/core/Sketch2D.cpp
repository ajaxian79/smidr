#include "core/Sketch2D.h"

#include <cmath>

namespace smidr {

ParamId Sketch2D::add_2d_point(float u, float v) {
    return constraints.add_point({u, v, 0});
}

void Sketch2D::add_line(ParamId p1, ParamId p2) {
    SketchEntity e;
    e.kind = Sketch2DPrimitive::Line;
    e.point_refs = {p1, p2};
    entities.push_back(e);
}

void Sketch2D::add_circle(ParamId center, float radius) {
    SketchEntity e;
    e.kind = Sketch2DPrimitive::Circle;
    e.point_refs = {center};
    e.numeric_params = {radius};
    entities.push_back(e);
}

void Sketch2D::add_horizontal_distance(ParamId p1, ParamId p2, float d) {
    constraints.add_distance(p1, p2, d);
}

bool Sketch2D::solve() {
    return constraints.solve();
}

Vec3 Sketch2D::world_from_uv(float u, float v) const {
    return plane_origin + plane_u * u + plane_v * v;
}

std::vector<Vec3> Sketch2D::tessellate(int segments) const {
    std::vector<Vec3> out;
    for (auto& e : entities) {
        switch (e.kind) {
            case Sketch2DPrimitive::Line: {
                if (e.point_refs.size() < 2) break;
                Vec3 p1 = constraints.point(e.point_refs[0]);
                Vec3 p2 = constraints.point(e.point_refs[1]);
                out.push_back(world_from_uv(p1.x, p1.y));
                out.push_back(world_from_uv(p2.x, p2.y));
                break;
            }
            case Sketch2DPrimitive::Circle: {
                if (e.point_refs.empty() || e.numeric_params.empty()) break;
                Vec3 c = constraints.point(e.point_refs[0]);
                float r = e.numeric_params[0];
                for (int i = 0; i <= segments; ++i) {
                    float t = kTwoPi * static_cast<float>(i) / static_cast<float>(segments);
                    out.push_back(world_from_uv(c.x + r * std::cos(t),
                                                 c.y + r * std::sin(t)));
                }
                break;
            }
            default: break;
        }
    }
    return out;
}

}  // namespace smidr
