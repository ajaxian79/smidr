#pragma once

#include "core/Math.h"

#include <cstdint>
#include <vector>

namespace smidr {

Vec3 tonemap_aces(Vec3 hdr);
Vec3 tonemap_reinhard(Vec3 hdr);
Vec3 tonemap_filmic(Vec3 hdr);

void apply_tonemap(std::vector<uint8_t>& pixels, int w, int h,
                    Vec3 (*func)(Vec3) = tonemap_aces);

void gaussian_blur(std::vector<uint8_t>& pixels, int w, int h, int radius);

void bloom_effect(std::vector<uint8_t>& pixels, int w, int h,
                   float threshold = 0.8f, float intensity = 0.5f, int blur_radius = 5);

void vignette(std::vector<uint8_t>& pixels, int w, int h, float strength = 0.4f);

void chromatic_aberration(std::vector<uint8_t>& pixels, int w, int h, float strength = 0.005f);

}  // namespace smidr
