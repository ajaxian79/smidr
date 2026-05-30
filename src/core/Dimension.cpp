#include "core/Dimension.h"

#include <cmath>
#include <cstdio>

namespace smidr {

float Dimension::measure() const {
    switch (type) {
        case DimensionType::Linear:
            return (point_b - point_a).length();
        case DimensionType::Radial:
        case DimensionType::Diameter:
            return (point_b - point_a).length() * (type == DimensionType::Diameter ? 2.f : 1.f);
        case DimensionType::Angular: {
            Vec3 va = (point_a - point_b).normalized();
            Vec3 vc = (point_c - point_b).normalized();
            float d = va.dot(vc);
            d = std::max(-1.f, std::min(1.f, d));
            return std::acos(d) * 180.f / kPi;
        }
    }
    return 0.f;
}

std::string Dimension::format_value(int decimals) const {
    char buf[64];
    const char* fmt = type == DimensionType::Angular ? "%.*f°" : "%.*f";
    std::snprintf(buf, sizeof buf, fmt, decimals, measure());
    return buf;
}

int DimensionSet::add_linear(Vec3 a, Vec3 b, float offset) {
    Dimension d;
    d.type = DimensionType::Linear;
    d.point_a = a; d.point_b = b; d.offset = offset;
    d.label = d.format_value();
    dimensions.push_back(d);
    return static_cast<int>(dimensions.size()) - 1;
}

int DimensionSet::add_radial(Vec3 center, Vec3 on_circle, float offset) {
    Dimension d;
    d.type = DimensionType::Radial;
    d.point_a = center; d.point_b = on_circle; d.offset = offset;
    d.label = "R" + d.format_value();
    dimensions.push_back(d);
    return static_cast<int>(dimensions.size()) - 1;
}

int DimensionSet::add_angular(Vec3 a, Vec3 vertex, Vec3 c, float offset) {
    Dimension d;
    d.type = DimensionType::Angular;
    d.point_a = a; d.point_b = vertex; d.point_c = c; d.offset = offset;
    d.label = d.format_value();
    dimensions.push_back(d);
    return static_cast<int>(dimensions.size()) - 1;
}

}  // namespace smidr
