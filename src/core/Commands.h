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
