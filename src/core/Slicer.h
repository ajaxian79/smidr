#pragma once

#include "core/Math.h"
#include "core/Primitive.h"

#include <filesystem>
#include <vector>

namespace smidr {

struct Polyline {
    std::vector<Vec3> points;
    bool closed = false;
};

std::vector<Polyline> slice_mesh(const Mesh& mesh, Vec3 plane_normal, float plane_offset);

std::vector<std::vector<Polyline>> slice_z_stack(const Mesh& mesh, float z_min, float z_max, float step);

bool export_slices_dxf(const std::filesystem::path& path,
                       const std::vector<std::vector<Polyline>>& slices,
                       float z_min, float step);

}  // namespace smidr
