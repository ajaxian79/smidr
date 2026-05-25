#include "core/MeshGenerator.h"
#include "core/CSGEval.h"

namespace smidr {

void MeshGenerator::rebuild(const Scene& scene, int detail) {
    entries_.clear();
    for (auto rid : scene.root_ids()) {
        if (auto* node = scene.find(rid))
            process_node(scene, *node, detail);
    }
}

void MeshGenerator::process_node(const Scene& scene, const SceneNode& node, int detail) {
    if (!node.visible) return;

    if (node.is_boolean() && node.children.size() >= 2) {
        SceneMeshEntry entry;
        entry.node_id   = node.id;
        entry.mesh      = evaluate_csg_node(scene, node, detail);
        entry.transform = Mat4::identity();
        entry.color     = node.color;
        entry.opacity   = node.opacity;
        entry.selected  = node.selected;
        entries_.push_back(std::move(entry));
        return;
    }

    if (node.primitive) {
        SceneMeshEntry entry;
        entry.node_id   = node.id;
        entry.mesh      = node.primitive->generate_mesh(detail);
        entry.transform = node.world_transform(scene);
        entry.color     = node.color;
        entry.opacity   = node.opacity;
        entry.selected  = node.selected;
        entries_.push_back(std::move(entry));
    }

    for (auto cid : node.children) {
        if (auto* child = scene.find(cid))
            process_node(scene, *child, detail);
    }
}

}  // namespace smidr
