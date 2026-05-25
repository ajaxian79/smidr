#include "app/SceneTreePanel.h"
#include "app/App.h"

#include <imgui.h>
#include "gui/ui.h"

namespace smidr {

static void draw_node(App& app, const SceneNode& node) {
    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_SpanAvailWidth;

    if (node.selected)
        flags |= ImGuiTreeNodeFlags_Selected;

    if (node.children.empty())
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    bool is_open = ImGui::TreeNodeEx(
        reinterpret_cast<void*>(static_cast<uintptr_t>(node.id)),
        flags, "%s", node.name.c_str());

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        app.document().scene().select(node.id);
        app.document().mark_dirty();
    }

    if (is_open && !node.children.empty()) {
        for (auto cid : node.children) {
            if (auto* child = app.document().scene().find(cid))
                draw_node(app, *child);
        }
        ImGui::TreePop();
    }
}

void draw_scene_tree(App& app) {
    smidr::ui::SectionHeader("Scene");

    auto& scene = app.document().scene();
    for (auto rid : scene.root_ids()) {
        if (auto* node = scene.find(rid))
            draw_node(app, *node);
    }

    if (scene.root_ids().empty()) {
        ImGui::TextColored(
            smidr::ui::tokens::to_vec4(smidr::ui::tokens::ink::dim),
            "Empty scene. Add a primitive from the toolbar.");
    }
}

}  // namespace smidr
