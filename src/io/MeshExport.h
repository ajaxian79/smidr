#pragma once

#include "core/MeshGenerator.h"

#include <filesystem>
#include <string>

namespace smidr {

bool export_stl(const std::filesystem::path& path, const MeshGenerator& meshes);
bool export_obj(const std::filesystem::path& path, const MeshGenerator& meshes);

}  // namespace smidr
