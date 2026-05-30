#pragma once

#include <filesystem>
#include <vector>
#include <cstdint>

namespace smidr {

bool save_png(const std::filesystem::path& path,
              const std::vector<uint8_t>& pixels,
              int width, int height, int channels = 3);

bool capture_framebuffer(const std::filesystem::path& path,
                          int width, int height);

}  // namespace smidr
