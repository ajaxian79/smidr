#pragma once

#include "core/CommandHistory.h"
#include "core/Primitive.h"
#include "core/Scene.h"

#include <memory>

namespace smidr {

class AddPrimitiveCommand : public Command {
public:
    AddPrimitiveCommand(Scene& scene, const std::string& name,
                        std::unique_ptr<Primitive> prim, NodeId parent = kInvalidNode)
        : scene_(scene), name_(name), prim_(std::move(prim)), parent_(parent) {}

    void execute() override {
        id_ = scene_.add_primitive(name_, prim_->clone(), parent_);
        scene_.select(id_);
    }

    void undo() override {
        scene_.remove_node(id_);
    }

    std::string description() const override { return "Add " + name_; }

private:
    Scene& scene_;
    std::string name_;
    std::unique_ptr<Primitive> prim_;
    NodeId parent_;
    NodeId id_ = kInvalidNode;
};

class DeleteNodeCommand : public Command {
public:
    DeleteNodeCommand(Scene& scene, NodeId id)
        : scene_(scene), target_id_(id) {}

    void execute() override {
        auto* node = scene_.find(target_id_);
        if (!node) return;
        saved_name_ = node->name;
        saved_position_ = node->position;
        saved_rotation_ = node->rotation;
        saved_scale_ = node->scale_vec;
        saved_color_ = node->color;
        saved_opacity_ = node->opacity;
        saved_visible_ = node->visible;
        if (node->primitive) saved_prim_ = node->primitive->clone();
        scene_.remove_node(target_id_);
    }

    void undo() override {
        if (!saved_prim_) return;
        NodeId id = scene_.add_primitive(saved_name_, saved_prim_->clone());
        auto* node = scene_.find(id);
        if (node) {
            node->position = saved_position_;
            node->rotation = saved_rotation_;
            node->scale_vec = saved_scale_;
            node->color = saved_color_;
            node->opacity = saved_opacity_;
            node->visible = saved_visible_;
        }
        target_id_ = id;
        scene_.select(id);
    }

    std::string description() const override { return "Delete " + saved_name_; }

private:
    Scene& scene_;
    NodeId target_id_;
    std::string saved_name_;
    Vec3 saved_position_, saved_rotation_, saved_scale_, saved_color_;
    float saved_opacity_ = 1.f;
    bool saved_visible_ = true;
    std::unique_ptr<Primitive> saved_prim_;
};

class ColorCommand : public Command {
public:
    ColorCommand(Scene& scene, NodeId id, Vec3 new_color)
        : scene_(scene), id_(id), new_color_(new_color) {
        auto* n = scene_.find(id_);
        if (n) old_color_ = n->color;
    }
    void execute() override {
        auto* n = scene_.find(id_); if (n) n->color = new_color_;
    }
    void undo() override {
        auto* n = scene_.find(id_); if (n) n->color = old_color_;
    }
    std::string description() const override { return "Color"; }
private:
    Scene& scene_; NodeId id_;
    Vec3 new_color_, old_color_;
};

class RenameCommand : public Command {
public:
    RenameCommand(Scene& scene, NodeId id, std::string new_name)
        : scene_(scene), id_(id), new_name_(std::move(new_name)) {
        auto* n = scene_.find(id_);
        if (n) old_name_ = n->name;
    }
    void execute() override {
        auto* n = scene_.find(id_); if (n) n->name = new_name_;
    }
    void undo() override {
        auto* n = scene_.find(id_); if (n) n->name = old_name_;
    }
    std::string description() const override { return "Rename"; }
private:
    Scene& scene_; NodeId id_;
    std::string new_name_, old_name_;
};

class VisibilityCommand : public Command {
public:
    VisibilityCommand(Scene& scene, NodeId id, bool visible)
        : scene_(scene), id_(id), new_vis_(visible) {
        auto* n = scene_.find(id_);
        if (n) old_vis_ = n->visible;
    }
    void execute() override {
        auto* n = scene_.find(id_); if (n) n->visible = new_vis_;
    }
    void undo() override {
        auto* n = scene_.find(id_); if (n) n->visible = old_vis_;
    }
    std::string description() const override { return "Toggle Visibility"; }
private:
    Scene& scene_; NodeId id_;
    bool new_vis_, old_vis_ = true;
};

class ReparentCommand : public Command {
public:
    ReparentCommand(Scene& scene, NodeId child, NodeId new_parent)
        : scene_(scene), child_(child), new_parent_(new_parent) {
        auto* n = scene_.find(child_);
        if (n) old_parent_ = n->parent;
    }
    void execute() override { scene_.reparent(child_, new_parent_); }
    void undo() override { scene_.reparent(child_, old_parent_); }
    std::string description() const override { return "Reparent"; }
private:
    Scene& scene_;
    NodeId child_, new_parent_, old_parent_ = kInvalidNode;
};

class MacroCommand : public Command {
public:
    std::vector<std::unique_ptr<Command>> commands;
    std::string macro_name = "Macro";
    void execute() override { for (auto& c : commands) c->execute(); }
    void undo() override { for (auto it = commands.rbegin(); it != commands.rend(); ++it) (*it)->undo(); }
    std::string description() const override { return macro_name; }
};

class TransformCommand : public Command {
public:
    TransformCommand(Scene& scene, NodeId id,
                     Vec3 new_pos, Vec3 new_rot, Vec3 new_scale)
        : scene_(scene), id_(id),
          new_pos_(new_pos), new_rot_(new_rot), new_scale_(new_scale) {
        auto* n = scene_.find(id_);
        if (n) {
            old_pos_ = n->position;
            old_rot_ = n->rotation;
            old_scale_ = n->scale_vec;
        }
    }

    void execute() override {
        auto* n = scene_.find(id_);
        if (!n) return;
        n->position = new_pos_;
        n->rotation = new_rot_;
        n->scale_vec = new_scale_;
    }

    void undo() override {
        auto* n = scene_.find(id_);
        if (!n) return;
        n->position = old_pos_;
        n->rotation = old_rot_;
        n->scale_vec = old_scale_;
    }

    std::string description() const override { return "Transform"; }

private:
    Scene& scene_;
    NodeId id_;
    Vec3 new_pos_, new_rot_, new_scale_;
    Vec3 old_pos_, old_rot_, old_scale_;
};

}  // namespace smidr
