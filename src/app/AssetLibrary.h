#pragma once

#include "core/Primitive.h"
#include "core/Material.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace smidr {

class App;

struct PrimitivePreset {
    std::string name;
    std::string category;
    std::function<std::unique_ptr<Primitive>()> factory;
    std::string description;
};

struct MaterialPreset {
    std::string name;
    std::string category;
    Material material;
};

struct ScenePreset {
    std::string name;
    std::string description;
    std::function<void(App&)> install;
};

class AssetLibrary {
public:
    AssetLibrary();

    const std::vector<PrimitivePreset>& primitives() const { return primitives_; }
    const std::vector<MaterialPreset>& materials() const { return materials_; }
    const std::vector<ScenePreset>& scenes() const { return scenes_; }

    void add_primitive(PrimitivePreset p);
    void add_material(MaterialPreset m);
    void add_scene(ScenePreset s);

    std::vector<PrimitivePreset> by_category(const std::string& cat) const;

    static AssetLibrary& instance();

private:
    std::vector<PrimitivePreset> primitives_;
    std::vector<MaterialPreset>  materials_;
    std::vector<ScenePreset>     scenes_;
    void install_defaults();
};

}  // namespace smidr
