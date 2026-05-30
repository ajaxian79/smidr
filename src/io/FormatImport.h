#pragma once

#include "core/Primitive.h"
#include "core/Scene.h"

#include <filesystem>
#include <string>

namespace smidr {

bool import_stl(const std::filesystem::path& path, Scene& scene);
bool import_obj(const std::filesystem::path& path, Scene& scene);
bool import_ply(const std::filesystem::path& path, Scene& scene);
bool import_off(const std::filesystem::path& path, Scene& scene);
bool import_dxf(const std::filesystem::path& path, Scene& scene);
bool import_vrml(const std::filesystem::path& path, Scene& scene);

std::string detect_format(const std::filesystem::path& path);

}  // namespace smidr
