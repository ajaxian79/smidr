#pragma once

#include "core/Math.h"

#include <string>
#include <unordered_map>
#include <cstdint>

namespace smidr {

using MaterialId = uint32_t;
constexpr MaterialId kDefaultMaterial = 0;

struct Material {
    std::string name = "Default";
    Vec3  base_color{0.6f, 0.6f, 0.7f};
    float roughness  = 0.5f;
    float metallic   = 0.0f;
    float opacity     = 1.0f;
};

class MaterialLibrary {
public:
    MaterialLibrary();

    MaterialId add(const Material& mat);
    void       remove(MaterialId id);
    Material*       find(MaterialId id);
    const Material* find(MaterialId id) const;

    const std::unordered_map<MaterialId, Material>& all() const { return mats_; }

    void clear();

private:
    std::unordered_map<MaterialId, Material> mats_;
    MaterialId next_id_ = 1;
};

}  // namespace smidr
