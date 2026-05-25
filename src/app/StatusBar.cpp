#include "app/StatusBar.h"
#include "app/App.h"
#include "app/Chrome.h"
#include "gui/ui.h"

#include <cstdio>
#include <imgui.h>

namespace smidr {

static constexpr ImGuiWindowFlags kChromeFlags =
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
    ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

void draw_statusbar(App& app) {
    const ImVec2 vp = ImGui::GetIO().DisplaySize;
    const float y = vp.y - kStatusBarHeight;

    ImGui::SetNextWindowPos(ImVec2(0, y));
    ImGui::SetNextWindowSize(ImVec2(vp.x, kStatusBarHeight));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 4));
    ImGui::Begin("##statusbar", nullptr, kChromeFlags);
    ImGui::PopStyleVar();

    auto* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(ImVec2(0, y), ImVec2(vp.x, vp.y),
                      ui::tokens::surface::panel_alt);
    dl->AddLine(ImVec2(0, y), ImVec2(vp.x, y),
                ui::tokens::border::separator, 1.f);

    ui::BadgeChip("Select", ui::BadgeTone::Accent);
    ImGui::SameLine(0, 16);

    int obj_count = 0;
    app.document().scene().for_each([&](const SceneNode&) { ++obj_count; });
    char stats[128];
    std::snprintf(stats, sizeof stats, "Objects: %d", obj_count);
    ImGui::TextColored(ui::tokens::to_vec4(ui::tokens::ink::muted), "%s", stats);

    ImGui::SameLine(0, 32);
    ImGui::TextColored(ui::tokens::to_vec4(ui::tokens::ink::dim), "Ready");

    ImGui::End();
}

}  // namespace smidr
