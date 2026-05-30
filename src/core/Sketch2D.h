#pragma once

#include "core/Math.h"
#include "core/Constraints.h"

#include <string>
#include <vector>

namespace smidr {

enum class Sketch2DPrimitive { Point, Line, Arc, Circle, Spline };

struct SketchEntity {
    Sketch2DPrimitive kind = Sketch2DPrimitive::Line;
    std::vector<ParamId> point_refs;
    std::vector<float>   numeric_params;
    std::string label;
};

class Sketch2D {
public:
    Vec3 plane_origin{0, 0, 0};
    Vec3 plane_u{1, 0, 0};
    Vec3 plane_v{0, 1, 0};

    SketchConstraints constraints;
    std::vector<SketchEntity> entities;

    ParamId add_2d_point(float u, float v);
    void    add_line(ParamId p1, ParamId p2);
    void    add_circle(ParamId center, float radius);
    void    add_horizontal_distance(ParamId p1, ParamId p2, float d);

    bool solve();

    Vec3 world_from_uv(float u, float v) const;
    std::vector<Vec3> tessellate(int segments = 32) const;
};

}  // namespace smidr
