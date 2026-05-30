#include "render/PostFX.h"

#include <algorithm>
#include <cmath>

namespace smidr {

Vec3 tonemap_aces(Vec3 c) {
    const float a = 2.51f, b = 0.03f, c_ = 2.43f, d = 0.59f, e = 0.14f;
    auto t = [&](float x) {
        return std::max(0.f, std::min(1.f, (x * (a * x + b)) / (x * (c_ * x + d) + e)));
    };
    return {t(c.x), t(c.y), t(c.z)};
}

Vec3 tonemap_reinhard(Vec3 c) {
    return {c.x / (1.f + c.x), c.y / (1.f + c.y), c.z / (1.f + c.z)};
}

Vec3 tonemap_filmic(Vec3 c) {
    auto t = [](float x) {
        x = std::max(0.f, x - 0.004f);
        return (x * (6.2f * x + 0.5f)) / (x * (6.2f * x + 1.7f) + 0.06f);
    };
    return {t(c.x), t(c.y), t(c.z)};
}

void apply_tonemap(std::vector<uint8_t>& pixels, int w, int h, Vec3 (*func)(Vec3)) {
    for (int i = 0; i < w * h; ++i) {
        size_t idx = static_cast<size_t>(i * 3);
        Vec3 hdr{pixels[idx] / 255.f, pixels[idx+1] / 255.f, pixels[idx+2] / 255.f};
        Vec3 ldr = func(hdr * 1.5f);
        pixels[idx]   = static_cast<uint8_t>(std::min(255.f, ldr.x * 255.f));
        pixels[idx+1] = static_cast<uint8_t>(std::min(255.f, ldr.y * 255.f));
        pixels[idx+2] = static_cast<uint8_t>(std::min(255.f, ldr.z * 255.f));
    }
}

void gaussian_blur(std::vector<uint8_t>& pixels, int w, int h, int radius) {
    if (radius < 1) return;
    std::vector<float> kernel(static_cast<size_t>(radius * 2 + 1));
    float sigma = radius * 0.5f;
    float sum = 0.f;
    for (int i = -radius; i <= radius; ++i) {
        float v = std::exp(-(i * i) / (2 * sigma * sigma));
        kernel[i + radius] = v;
        sum += v;
    }
    for (auto& k : kernel) k /= sum;

    std::vector<uint8_t> tmp(pixels.size());

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float r = 0, g = 0, b = 0;
            for (int k = -radius; k <= radius; ++k) {
                int sx = std::max(0, std::min(w - 1, x + k));
                size_t idx = static_cast<size_t>((y * w + sx) * 3);
                float w_k = kernel[k + radius];
                r += pixels[idx] * w_k;
                g += pixels[idx+1] * w_k;
                b += pixels[idx+2] * w_k;
            }
            size_t out = static_cast<size_t>((y * w + x) * 3);
            tmp[out] = static_cast<uint8_t>(r);
            tmp[out+1] = static_cast<uint8_t>(g);
            tmp[out+2] = static_cast<uint8_t>(b);
        }
    }
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float r = 0, g = 0, b = 0;
            for (int k = -radius; k <= radius; ++k) {
                int sy = std::max(0, std::min(h - 1, y + k));
                size_t idx = static_cast<size_t>((sy * w + x) * 3);
                float w_k = kernel[k + radius];
                r += tmp[idx] * w_k;
                g += tmp[idx+1] * w_k;
                b += tmp[idx+2] * w_k;
            }
            size_t out = static_cast<size_t>((y * w + x) * 3);
            pixels[out] = static_cast<uint8_t>(r);
            pixels[out+1] = static_cast<uint8_t>(g);
            pixels[out+2] = static_cast<uint8_t>(b);
        }
    }
}

void bloom_effect(std::vector<uint8_t>& pixels, int w, int h,
                   float threshold, float intensity, int blur_radius) {
    std::vector<uint8_t> bright(pixels.size());
    for (size_t i = 0; i + 2 < pixels.size(); i += 3) {
        float lum = (pixels[i] + pixels[i+1] + pixels[i+2]) / (3.f * 255.f);
        if (lum > threshold) {
            float t = (lum - threshold) / (1.f - threshold);
            bright[i] = static_cast<uint8_t>(pixels[i] * t);
            bright[i+1] = static_cast<uint8_t>(pixels[i+1] * t);
            bright[i+2] = static_cast<uint8_t>(pixels[i+2] * t);
        }
    }
    gaussian_blur(bright, w, h, blur_radius);
    for (size_t i = 0; i + 2 < pixels.size(); i += 3) {
        int r = pixels[i] + static_cast<int>(bright[i] * intensity);
        int g = pixels[i+1] + static_cast<int>(bright[i+1] * intensity);
        int b = pixels[i+2] + static_cast<int>(bright[i+2] * intensity);
        pixels[i] = static_cast<uint8_t>(std::min(255, r));
        pixels[i+1] = static_cast<uint8_t>(std::min(255, g));
        pixels[i+2] = static_cast<uint8_t>(std::min(255, b));
    }
}

void vignette(std::vector<uint8_t>& pixels, int w, int h, float strength) {
    float cx = w * 0.5f, cy = h * 0.5f;
    float max_dist = std::sqrt(cx * cx + cy * cy);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float dx = (x - cx), dy = (y - cy);
            float d = std::sqrt(dx * dx + dy * dy) / max_dist;
            float v = 1.f - std::min(1.f, d * d * strength);
            size_t idx = static_cast<size_t>((y * w + x) * 3);
            pixels[idx]   = static_cast<uint8_t>(pixels[idx] * v);
            pixels[idx+1] = static_cast<uint8_t>(pixels[idx+1] * v);
            pixels[idx+2] = static_cast<uint8_t>(pixels[idx+2] * v);
        }
    }
}

void chromatic_aberration(std::vector<uint8_t>& pixels, int w, int h, float strength) {
    std::vector<uint8_t> tmp = pixels;
    int offset = static_cast<int>(strength * w);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int rx = std::max(0, std::min(w - 1, x - offset));
            int bx = std::max(0, std::min(w - 1, x + offset));
            size_t out = static_cast<size_t>((y * w + x) * 3);
            pixels[out]   = tmp[static_cast<size_t>((y * w + rx) * 3)];
            pixels[out+1] = tmp[static_cast<size_t>((y * w + x) * 3) + 1];
            pixels[out+2] = tmp[static_cast<size_t>((y * w + bx) * 3) + 2];
        }
    }
}

}  // namespace smidr
