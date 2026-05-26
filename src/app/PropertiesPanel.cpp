#include "app/PropertiesPanel.h"
#include "app/App.h"

#include <cstdio>
#include <imgui.h>
#include "gui/ui.h"

namespace smidr {

void draw_properties(App& app) {
    smidr::ui::SectionHeader("Properties");

    auto sel = app.document().scene().selected_id();
    if (!sel) {
        ImGui::TextColored(
            smidr::ui::tokens::to_vec4(smidr::ui::tokens::ink::dim),
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
    smidr::ui::SectionHeader("Transform");

    bool changed = false;
    ImGui::SetNextItemWidth(-1);
    changed |= ImGui::DragFloat3("Position", &node->position.x, 0.05f);
    ImGui::SetNextItemWidth(-1);
    changed |= ImGui::DragFloat3("Rotation", &node->rotation.x, 0.5f);
    ImGui::SetNextItemWidth(-1);
    changed |= ImGui::DragFloat3("Scale", &node->scale_vec.x, 0.01f, 0.01f, 100.f);

    if (changed) app.document().mark_dirty();

    ImGui::Dummy(ImVec2(0, 8));
    smidr::ui::SectionHeader("Appearance");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::ColorEdit3("Color", &node->color.x, ImGuiColorEditFlags_NoInputs))
        app.document().mark_dirty();
    ImGui::SetNextItemWidth(120);
    if (ImGui::SliderFloat("Opacity", &node->opacity, 0.f, 1.f))
        app.document().mark_dirty();

    if (node->primitive) {
        ImGui::Dummy(ImVec2(0, 8));
        smidr::ui::SectionHeader("Primitive");

        smidr::ui::BadgeChip(primitive_type_name(node->primitive->type()),
                         smidr::ui::BadgeTone::Accent);

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
            case PrimitiveType::Ellipsoid: {
                auto* e = static_cast<Ellipsoid*>(node->primitive.get());
                ImGui::SetNextItemWidth(-1);
                prim_changed |= ImGui::DragFloat3("Radii", &e->radii.x, 0.01f, 0.01f, 100.f);
                break;
            }
            case PrimitiveType::Halfspace: {
                auto* h = static_cast<Halfspace*>(node->primitive.get());
                ImGui::SetNextItemWidth(-1);
                prim_changed |= ImGui::DragFloat3("Normal", &h->normal.x, 0.01f);
                ImGui::SetNextItemWidth(120);
                prim_changed |= ImGui::DragFloat("Offset", &h->offset, 0.05f);
                break;
            }
            case PrimitiveType::Pipe: {
                auto* p = static_cast<Pipe*>(node->primitive.get());
                ImGui::SetNextItemWidth(120);
                prim_changed |= ImGui::DragFloat("Inner R", &p->inner_radius, 0.01f, 0.01f, 100.f);
                ImGui::SetNextItemWidth(120);
                prim_changed |= ImGui::DragFloat("Outer R", &p->outer_radius, 0.01f, 0.01f, 100.f);
                ImGui::SetNextItemWidth(120);
                prim_changed |= ImGui::DragFloat("Height", &p->height, 0.01f, 0.01f, 100.f);
                break;
            }
            case PrimitiveType::Wedge: {
                auto* w = static_cast<Wedge*>(node->primitive.get());
                ImGui::SetNextItemWidth(-1);
                prim_changed |= ImGui::DragFloat3("Size", &w->size.x, 0.01f, 0.01f, 100.f);
                ImGui::SetNextItemWidth(120);
                prim_changed |= ImGui::DragFloat("Top Width", &w->top_width, 0.01f, 0.f, 100.f);
                break;
            }
            case PrimitiveType::Arb8: {
                auto* a = static_cast<Arb8*>(node->primitive.get());
                for (int i = 0; i < 8; ++i) {
                    char label[16];
                    std::snprintf(label, sizeof label, "V%d", i);
                    ImGui::SetNextItemWidth(-1);
                    prim_changed |= ImGui::DragFloat3(label, &a->verts[i].x, 0.01f);
                }
                break;
            }
            default:
                ImGui::TextColored(
                    smidr::ui::tokens::to_vec4(smidr::ui::tokens::ink::dim),
                    "(%s — use console for params)",
                    primitive_type_name(node->primitive->type()));
                break;
        }

        if (prim_changed) app.document().mark_dirty();
    }

    if (node->is_boolean()) {
        ImGui::Dummy(ImVec2(0, 8));
        smidr::ui::SectionHeader("Boolean");
        smidr::ui::BadgeChip(boolean_op_name(node->boolean_op), smidr::ui::BadgeTone::Info);

        ImGui::TextColored(
            smidr::ui::tokens::to_vec4(smidr::ui::tokens::ink::muted),
            "%zu children", node->children.size());
    }

    ImGui::Dummy(ImVec2(0, 8));
    if (ImGui::Button("Visible")) {
        node->visible = !node->visible;
        app.document().mark_dirty();
    }
    ImGui::SameLine();
    smidr::ui::BadgeChip(node->visible ? "shown" : "hidden",
                     node->visible ? smidr::ui::BadgeTone::Success : smidr::ui::BadgeTone::Muted);
}

}  // namespace smidr
