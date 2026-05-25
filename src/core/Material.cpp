#include "core/Material.h"

namespace smidr {

MaterialLibrary::MaterialLibrary() {
    Material def;
    def.name = "Default";
    mats_[kDefaultMaterial] = def;
}

MaterialId MaterialLibrary::add(const Material& mat) {
    MaterialId id = next_id_++;
    mats_[id] = mat;
    return id;
}

void MaterialLibrary::remove(MaterialId id) {
    if (id != kDefaultMaterial)
        mats_.erase(id);
}

Material* MaterialLibrary::find(MaterialId id) {
    auto it = mats_.find(id);
    return it != mats_.end() ? &it->second : nullptr;
}

const Material* MaterialLibrary::find(MaterialId id) const {
    auto it = mats_.find(id);
    return it != mats_.end() ? &it->second : nullptr;
}

void MaterialLibrary::clear() {
    mats_.clear();
    Material def;
    def.name = "Default";
    mats_[kDefaultMaterial] = def;
    next_id_ = 1;
}

}  // namespace smidr
