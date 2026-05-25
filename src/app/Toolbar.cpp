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

}  // namespace smidr
