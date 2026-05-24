#include "app/PropertiesPanel.h"
#include "app/App.h"

#include <cstdio>
#include <imgui.h>
#include <skald/skald.h>

namespace smidr {

void draw_properties(App& app) {
    skald::SectionHeader("Properties");

    auto sel = app.document().scene().selected_id();
    if (!sel) {
        ImGui::TextColored(
            skald::tokens::to_vec4(skald::tokens::ink::dim),
            "No selection");
        return;
    }

    auto* node = app.document().scene().find(*sel);
    if (!node) return;

    char name_buf[128];
    std::snprintf(name_buf, sizeof name_buf, "%s", node->name.c_str());
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputText("##name", name_buf, sizeof name_buf)) {
        node->name = name_buf;
        app.document().mark_dirty();
    }

    ImGui::Dummy(ImVec2(0, 8));
    skald::SectionHeader("Transform");

    bool changed = false;
    ImGui::SetNextItemWidth(-1);
    changed |= ImGui::DragFloat3("Position", &node->position.x, 0.05f);
    ImGui::SetNextItemWidth(-1);
    changed |= ImGui::DragFloat3("Rotation", &node->rotation.x, 0.5f);
    ImGui::SetNextItemWidth(-1);
    changed |= ImGui::DragFloat3("Scale", &node->scale_vec.x, 0.01f, 0.01f, 100.f);

    if (changed) app.document().mark_dirty();

    ImGui::Dummy(ImVec2(0, 8));
    skald::SectionHeader("Appearance");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::ColorEdit3("Color", &node->color.x, ImGuiColorEditFlags_NoInputs))
        app.document().mark_dirty();
    ImGui::SetNextItemWidth(120);
    if (ImGui::SliderFloat("Opacity", &node->opacity, 0.f, 1.f))
        app.document().mark_dirty();

    if (node->primitive) {
        ImGui::Dummy(ImVec2(0, 8));
        skald::SectionHeader("Primitive");

        skald::BadgeChip(primitive_type_name(node->primitive->type()),
                         skald::BadgeTone::Accent);

        bool prim_changed = false;

        switch (node->primitive->type()) {
            case PrimitiveType::Sphere: {
                auto* s = static_cast<Sphere*>(node->primitive.get());
                ImGui::SetNextItemWidth(120);
                prim_changed |= ImGui::DragFloat("Radius", &s->radius, 0.01f, 0.01f, 100.f);
                break;
            }
            case PrimitiveType::Box: {
                auto* b = static_cast<Box*>(node->primitive.get());
                ImGui::SetNextItemWidth(-1);
                prim_changed |= ImGui::DragFloat3("Half Extents", &b->half_extents.x, 0.01f, 0.01f, 100.f);
                break;
            }
            case PrimitiveType::Cylinder: {
                auto* c = static_cast<Cylinder*>(node->primitive.get());
                ImGui::SetNextItemWidth(120);
                prim_changed |= ImGui::DragFloat("Radius", &c->radius, 0.01f, 0.01f, 100.f);
                ImGui::SetNextItemWidth(120);
                prim_changed |= ImGui::DragFloat("Height", &c->height, 0.01f, 0.01f, 100.f);
                break;
            }
            case PrimitiveType::Cone: {
                auto* c = static_cast<Cone*>(node->primitive.get());
                ImGui::SetNextItemWidth(120);
                prim_changed |= ImGui::DragFloat("Radius", &c->radius, 0.01f, 0.01f, 100.f);
                ImGui::SetNextItemWidth(120);
                prim_changed |= ImGui::DragFloat("Height", &c->height, 0.01f, 0.01f, 100.f);
                break;
            }
            case PrimitiveType::Torus: {
                auto* t = static_cast<Torus*>(node->primitive.get());
                ImGui::SetNextItemWidth(120);
                prim_changed |= ImGui::DragFloat("Major R", &t->major_radius, 0.01f, 0.01f, 100.f);
                ImGui::SetNextItemWidth(120);
                prim_changed |= ImGui::DragFloat("Minor R", &t->minor_radius, 0.01f, 0.01f, 50.f);
                break;
            }
        }

        if (prim_changed) app.document().mark_dirty();
    }

    if (node->is_boolean()) {
        ImGui::Dummy(ImVec2(0, 8));
        skald::SectionHeader("Boolean");
        skald::BadgeChip(boolean_op_name(node->boolean_op), skald::BadgeTone::Info);

        ImGui::TextColored(
            skald::tokens::to_vec4(skald::tokens::ink::muted),
            "%zu children", node->children.size());
    }

    ImGui::Dummy(ImVec2(0, 8));
    if (ImGui::Button("Visible")) {
        node->visible = !node->visible;
        app.document().mark_dirty();
    }
    ImGui::SameLine();
    skald::BadgeChip(node->visible ? "shown" : "hidden",
                     node->visible ? skald::BadgeTone::Success : skald::BadgeTone::Muted);
}

}  // namespace smidr
