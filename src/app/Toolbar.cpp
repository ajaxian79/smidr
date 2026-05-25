#include "app/Toolbar.h"
#include "app/App.h"

#include <imgui.h>
#include "gui/ui.h"

namespace smidr {

void draw_toolbar(App& app) {
    smidr::ui::BeginToolbar();

    if (smidr::ui::IconButton(smidr::ui::icons::kPlus, "Add Sphere"))
        app.add_primitive(PrimitiveType::Sphere);
    ImGui::SameLine();

    if (ImGui::Button("Box"))
        app.add_primitive(PrimitiveType::Box);
    ImGui::SameLine();

    if (ImGui::Button("Cyl"))
        app.add_primitive(PrimitiveType::Cylinder);
    ImGui::SameLine();

    if (ImGui::Button("Cone"))
        app.add_primitive(PrimitiveType::Cone);
    ImGui::SameLine();

    if (ImGui::Button("Torus"))
        app.add_primitive(PrimitiveType::Torus);

    ImGui::SameLine(0, 20);
    smidr::ui::MutedSeparator(0, 0);
    ImGui::SameLine(0, 20);

    if (smidr::ui::IconButton(smidr::ui::icons::kTrash, "Delete"))
        app.delete_selected();

    ImGui::SameLine(0, 20);
    smidr::ui::MutedSeparator(0, 0);
    ImGui::SameLine(0, 20);

    if (ImGui::Button("Union"))
        app.boolean_selected(BooleanOp::Union);
    ImGui::SameLine();
    if (ImGui::Button("Intersect"))
        app.boolean_selected(BooleanOp::Intersection);
    ImGui::SameLine();
    if (ImGui::Button("Subtract"))
        app.boolean_selected(BooleanOp::Difference);

    ImGui::SameLine(0, 20);
    smidr::ui::MutedSeparator(0, 0);
    ImGui::SameLine(0, 20);

    smidr::ui::PillToggle("Wire", &app.renderer().wireframe);

    smidr::ui::EndToolbar();
}

void draw_console(App& app) {
    smidr::ui::SectionHeader("Console");

    ImGui::BeginChild("##console_scroll", ImVec2(0, 0), ImGuiChildFlags(0),
                      ImGuiWindowFlags_HorizontalScrollbar);

    for (auto& entry : app.log_entries()) {
        ImU32 col;
        switch (entry.level) {
            case LogEntry::Warning: col = smidr::ui::tokens::status::warning; break;
            case LogEntry::Error:   col = smidr::ui::tokens::status::destructive; break;
            default:                col = smidr::ui::tokens::ink::muted; break;
        }
        ImGui::TextColored(smidr::ui::tokens::to_vec4(col), "%s", entry.message.c_str());
    }

    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.f);

    ImGui::EndChild();
}

}  // namespace smidr
