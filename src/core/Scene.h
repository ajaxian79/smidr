#pragma once

#include "core/Math.h"
#include "core/Primitive.h"
#include "core/Material.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "io/json_fwd.hpp"

namespace smidr {

enum class BooleanOp { Union, Intersection, Difference };

const char* boolean_op_name(BooleanOp op);
BooleanOp   boolean_op_from_name(const std::string& name);

using NodeId = uint32_t;
constexpr NodeId kInvalidNode = 0;

struct SceneNode {
    NodeId      id = kInvalidNode;
    std::string name;

    Vec3 position{0, 0, 0};
    Vec3 rotation{0, 0, 0};
    Vec3 scale_vec{1, 1, 1};

    std::unique_ptr<Primitive> primitive;

    BooleanOp              boolean_op = BooleanOp::Union;
    std::vector<NodeId>    children;
    NodeId                 parent = kInvalidNode;

    bool visible  = true;
    bool selected = false;

    Vec3 color{0.6f, 0.6f, 0.7f};
    float opacity = 1.f;
    MaterialId material = kDefaultMaterial;

    Mat4 local_transform() const;
    Mat4 world_transform(const class Scene& scene) const;

    bool is_group() const { return !primitive && !children.empty(); }
    bool is_boolean() const { return !primitive && children.size() >= 2; }

    void to_json(nlohmann::json& j) const;
    static SceneNode from_json(const nlohmann::json& j, NodeId assigned_id);
};

class Scene {
public:
    Scene();

    NodeId add_primitive(const std::string& name,
                         std::unique_ptr<Primitive> prim,
                         NodeId parent = kInvalidNode);

    NodeId add_group(const std::string& name,
                     NodeId parent = kInvalidNode);

    NodeId add_boolean(const std::string& name,
                       BooleanOp op,
                       NodeId left, NodeId right);

    void remove_node(NodeId id);

    SceneNode*       find(NodeId id);
    const SceneNode* find(NodeId id) const;

    void select(NodeId id);
    void add_to_selection(NodeId id);
    void toggle_selection(NodeId id);
    void clear_selection();
    std::optional<NodeId> selected_id() const;
    std::vector<NodeId> selected_ids() const;
    int selection_count() const;

    const std::vector<NodeId>& root_ids() const { return root_ids_; }
    const std::vector<SceneNode>& nodes() const { return nodes_; }

    void for_each(std::function<void(const SceneNode&)> fn) const;

    void to_json(nlohmann::json& j) const;
    void from_json(const nlohmann::json& j);

    void clear();

private:
    std::vector<SceneNode> nodes_;
    std::vector<NodeId>    root_ids_;
    NodeId                 next_id_ = 1;
};

}  // namespace smidr
