#pragma once

#include "core/MeshGenerator.h"

#include <filesystem>
#include <string>

namespace smidr {

bool export_stl(const std::filesystem::path& path, const MeshGenerator& meshes);
bool export_obj(const std::filesystem::path& path, const MeshGenerator& meshes);
bool export_ply(const std::filesystem::path& path, const MeshGenerator& meshes);
bool export_off(const std::filesystem::path& path, const MeshGenerator& meshes);
bool export_dxf(const std::filesystem::path& path, const MeshGenerator& meshes);
bool export_vrml(const std::filesystem::path& path, const MeshGenerator& meshes);
bool export_x3d(const std::filesystem::path& path, const MeshGenerator& meshes);
bool export_gltf(const std::filesystem::path& path, const MeshGenerator& meshes);
bool export_iges(const std::filesystem::path& path, const MeshGenerator& meshes);
bool export_collada(const std::filesystem::path& path, const MeshGenerator& meshes);

}  // namespace smidr
