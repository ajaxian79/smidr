#pragma once

#include "core/Primitive.h"
#include "core/Scene.h"

namespace smidr {

struct SceneMeshEntry {
    NodeId node_id;
    Mesh   mesh;
    Mat4   transform;
    Vec3   color;
    float  opacity;
    bool   selected;
};

class MeshGenerator {
public:
    void rebuild(const Scene& scene, int detail = 32);
    const std::vector<SceneMeshEntry>& entries() const { return entries_; }

private:
    void process_node(const Scene& scene, const SceneNode& node, int detail);
    std::vector<SceneMeshEntry> entries_;
};

}  // namespace smidr
