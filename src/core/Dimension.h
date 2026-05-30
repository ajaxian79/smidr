#pragma once

#include "core/Math.h"

#include <string>
#include <vector>

namespace smidr {

enum class DimensionType { Linear, Radial, Angular, Diameter };

struct Dimension {
    DimensionType type = DimensionType::Linear;
    Vec3 point_a;
    Vec3 point_b;
    Vec3 point_c;
    float offset = 0.5f;
    std::string label;
    float text_height = 0.05f;

    float measure() const;
    std::string format_value(int decimals = 3) const;
};

class DimensionSet {
public:
    std::vector<Dimension> dimensions;

    int add_linear(Vec3 a, Vec3 b, float offset = 0.5f);
    int add_radial(Vec3 center, Vec3 on_circle, float offset = 0.5f);
    int add_angular(Vec3 a, Vec3 vertex, Vec3 c, float offset = 0.5f);

    void clear() { dimensions.clear(); }
    int  count() const { return static_cast<int>(dimensions.size()); }
};

}  // namespace smidr
