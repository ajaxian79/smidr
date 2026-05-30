#include "core/Scene.h"

#include <algorithm>
#include <stdexcept>

#include "io/json.hpp"

namespace smidr {

const char* boolean_op_name(BooleanOp op) {
    switch (op) {
        case BooleanOp::Union:        return "union";
        case BooleanOp::Intersection: return "intersection";
        case BooleanOp::Difference:   return "difference";
    }
    return "union";
}

BooleanOp boolean_op_from_name(const std::string& name) {
    if (name == "intersection") return BooleanOp::Intersection;
    if (name == "difference")   return BooleanOp::Difference;
    return BooleanOp::Union;
}

Mat4 SceneNode::local_transform() const {
    return Mat4::translate(position)
         * Mat4::rotate_z(rotation.z * kDegToRad)
         * Mat4::rotate_y(rotation.y * kDegToRad)
         * Mat4::rotate_x(rotation.x * kDegToRad)
         * Mat4::scale(scale_vec);
}

Mat4 SceneNode::world_transform(const Scene& scene) const {
    Mat4 local = local_transform();
    if (parent != kInvalidNode) {
        if (auto* p = scene.find(parent))
            return p->world_transform(scene) * local;
    }
    return local;
}

void SceneNode::to_json(nlohmann::json& j) const {
    j["id"]   = id;
    j["name"] = name;
    j["position"] = {position.x, position.y, position.z};
    j["rotation"] = {rotation.x, rotation.y, rotation.z};
    j["scale"]    = {scale_vec.x, scale_vec.y, scale_vec.z};
    j["color"]    = {color.x, color.y, color.z};
    j["opacity"]  = opacity;
    j["visible"]  = visible;

    if (primitive) {
        nlohmann::json pj;
        primitive->to_json(pj);
        j["primitive"] = pj;
    }

    if (!children.empty()) {
        j["boolean_op"] = boolean_op_name(boolean_op);
        j["children"] = children;
    }

    if (parent != kInvalidNode) j["parent"] = parent;
}

SceneNode SceneNode::from_json(const nlohmann::json& j, NodeId assigned_id) {
    SceneNode n;
    n.id   = assigned_id;
    n.name = j.value("name", "node");

    if (j.contains("position")) {
        auto& p = j["position"];
        n.position = {p[0].get<float>(), p[1].get<float>(), p[2].get<float>()};
    }
    if (j.contains("rotation")) {
        auto& r = j["rotation"];
        n.rotation = {r[0].get<float>(), r[1].get<float>(), r[2].get<float>()};
    }
    if (j.contains("scale")) {
        auto& s = j["scale"];
        n.scale_vec = {s[0].get<float>(), s[1].get<float>(), s[2].get<float>()};
    }
    if (j.contains("color")) {
        auto& c = j["color"];
        n.color = {c[0].get<float>(), c[1].get<float>(), c[2].get<float>()};
    }

    n.opacity = j.value("opacity", 1.f);
    n.visible = j.value("visible", true);

    if (j.contains("primitive"))
        n.primitive = Primitive::from_json(j["primitive"]);

    if (j.contains("boolean_op"))
        n.boolean_op = boolean_op_from_name(j["boolean_op"].get<std::string>());

    if (j.contains("children"))
        n.children = j["children"].get<std::vector<NodeId>>();

    n.parent = j.value("parent", kInvalidNode);

    return n;
}

// ── Scene ──────────────────────────────────────────────────────

Scene::Scene() = default;

NodeId Scene::add_primitive(const std::string& name,
                            std::unique_ptr<Primitive> prim,
                            NodeId parent) {
    SceneNode node;
    node.id = next_id_++;
    node.name = name;
    node.primitive = std::move(prim);
    node.parent = parent;

    NodeId id = node.id;
    nodes_.push_back(std::move(node));

    if (parent != kInvalidNode) {
        if (auto* p = find(parent))
            p->children.push_back(id);
    } else {
        root_ids_.push_back(id);
    }
    return id;
}

NodeId Scene::add_group(const std::string& name, NodeId parent) {
    SceneNode node;
    node.id = next_id_++;
    node.name = name;
    node.parent = parent;

    NodeId id = node.id;
    nodes_.push_back(std::move(node));

    if (parent != kInvalidNode) {
        if (auto* p = find(parent))
            p->children.push_back(id);
    } else {
        root_ids_.push_back(id);
    }
    return id;
}

NodeId Scene::add_boolean(const std::string& name,
                          BooleanOp op,
                          NodeId left, NodeId right) {
    SceneNode node;
    node.id = next_id_++;
    node.name = name;
    node.boolean_op = op;
    node.children = {left, right};

    auto* l = find(left);
    auto* r = find(right);
    if (l) l->parent = node.id;
    if (r) r->parent = node.id;

    auto remove_from_roots = [&](NodeId rid) {
        root_ids_.erase(
            std::remove(root_ids_.begin(), root_ids_.end(), rid),
            root_ids_.end());
    };
    remove_from_roots(left);
    remove_from_roots(right);

    NodeId id = node.id;
    nodes_.push_back(std::move(node));
    root_ids_.push_back(id);
    return id;
}

void Scene::remove_node(NodeId id) {
    auto* node = find(id);
    if (!node) return;

    if (node->parent != kInvalidNode) {
        if (auto* p = find(node->parent)) {
            auto& ch = p->children;
            ch.erase(std::remove(ch.begin(), ch.end(), id), ch.end());
        }
    }

    root_ids_.erase(
        std::remove(root_ids_.begin(), root_ids_.end(), id),
        root_ids_.end());

    for (auto child_id : node->children) {
        if (auto* c = find(child_id)) {
            c->parent = kInvalidNode;
            root_ids_.push_back(child_id);
        }
    }

    nodes_.erase(
        std::remove_if(nodes_.begin(), nodes_.end(),
                        [id](const SceneNode& n) { return n.id == id; }),
        nodes_.end());
}

SceneNode* Scene::find(NodeId id) {
    for (auto& n : nodes_)
        if (n.id == id) return &n;
    return nullptr;
}

const SceneNode* Scene::find(NodeId id) const {
    for (auto& n : nodes_)
        if (n.id == id) return &n;
    return nullptr;
}

void Scene::select(NodeId id) {
    for (auto& n : nodes_) n.selected = (n.id == id);
}

void Scene::clear_selection() {
    for (auto& n : nodes_) n.selected = false;
}

std::optional<NodeId> Scene::selected_id() const {
    for (auto& n : nodes_)
        if (n.selected) return n.id;
    return std::nullopt;
}

void Scene::add_to_selection(NodeId id) {
    auto* n = find(id);
    if (n) n->selected = true;
}

void Scene::toggle_selection(NodeId id) {
    auto* n = find(id);
    if (n) n->selected = !n->selected;
}

std::vector<NodeId> Scene::selected_ids() const {
    std::vector<NodeId> out;
    for (auto& n : nodes_) if (n.selected) out.push_back(n.id);
    return out;
}

int Scene::selection_count() const {
    int c = 0;
    for (auto& n : nodes_) if (n.selected) ++c;
    return c;
}

void Scene::for_each(std::function<void(const SceneNode&)> fn) const {
    for (auto& n : nodes_) fn(n);
}

void Scene::to_json(nlohmann::json& j) const {
    j["root_ids"] = root_ids_;
    j["next_id"] = next_id_;
    auto& arr = j["nodes"];
    arr = nlohmann::json::array();
    for (auto& n : nodes_) {
        nlohmann::json nj;
        n.to_json(nj);
        arr.push_back(nj);
    }
}

void Scene::from_json(const nlohmann::json& j) {
    clear();
    root_ids_ = j.at("root_ids").get<std::vector<NodeId>>();
    next_id_  = j.value("next_id", static_cast<NodeId>(1));
    for (auto& nj : j.at("nodes")) {
        NodeId id = nj.at("id").get<NodeId>();
        nodes_.push_back(SceneNode::from_json(nj, id));
        if (id >= next_id_) next_id_ = id + 1;
    }
}

void Scene::clear() {
    nodes_.clear();
    root_ids_.clear();
    next_id_ = 1;
}

}  // namespace smidr
