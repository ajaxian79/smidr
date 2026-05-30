#include "io/Screenshot.h"
#include "render/glad/glad.h"
#include "io/stb_image_write.h"

#include <algorithm>
#include <vector>

namespace smidr {

bool save_png(const std::filesystem::path& path,
              const std::vector<uint8_t>& pixels,
              int width, int height, int channels) {
    return stbi_write_png(path.string().c_str(), width, height, channels,
                          pixels.data(), width * channels) != 0;
}

bool capture_framebuffer(const std::filesystem::path& path,
                          int width, int height) {
    std::vector<uint8_t> pixels(static_cast<size_t>(width * height * 4));
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    std::vector<uint8_t> flipped(pixels.size());
    int row_bytes = width * 4;
    for (int y = 0; y < height; ++y) {
        std::copy(pixels.begin() + y * row_bytes,
                  pixels.begin() + (y + 1) * row_bytes,
                  flipped.begin() + (height - 1 - y) * row_bytes);
    }

    return save_png(path, flipped, width, height, 4);
}

}  // namespace smidr
