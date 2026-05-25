#include "app/TitleBar.h"
#include "app/App.h"
#include "app/Chrome.h"
#include "gui/ui.h"

#include <cstdio>
#include <imgui.h>
#include <GLFW/glfw3.h>

namespace smidr {

static constexpr ImGuiWindowFlags kChromeFlags =
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
    ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

static bool window_control(const char* label, bool destructive = false) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImU32 hover_col = destructive
        ? IM_COL32(0xc0, 0x40, 0x40, 0xff)
        : ui::tokens::surface::control;
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
        ui::tokens::to_vec4(hover_col));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
        ui::tokens::to_vec4(ui::tokens::surface::control_act));
    bool clicked = ImGui::Button(label, ImVec2(kWindowControlSize, kWindowControlSize));
    ImGui::PopStyleColor(3);
    return clicked;
}

void draw_titlebar(App& app, const ui::Fonts& fonts, GLFWwindow* win,
                   std::function<void()> toolbar_fn) {
    const ImVec2 vp = ImGui::GetIO().DisplaySize;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(vp.x, kTopChromeHeight));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ui::tokens::pad::window);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, ui::tokens::radii::none);
    ImGui::Begin("##titlebar", nullptr, kChromeFlags);
    ImGui::PopStyleVar(2);

    auto* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(ImVec2(0, 0), ImVec2(vp.x, kTopChromeHeight),
                      ui::tokens::surface::deep);
    dl->AddLine(ImVec2(0, kTopChromeHeight - 1), ImVec2(vp.x, kTopChromeHeight - 1),
                ui::tokens::border::separator, 1.f);

    ImGui::SetCursorPos(ImVec2(16, 10));
    if (fonts.sans_md) ImGui::PushFont(fonts.sans_md);
    ImGui::TextUnformatted("smidr");
    if (fonts.sans_md) ImGui::PopFont();

    ImGui::SameLine(72);
    char tag[256];
    std::snprintf(tag, sizeof tag, "/ %s%s",
                  app.document().name().c_str(),
                  app.document().dirty() ? " *" : "");
    ImGui::TextColored(ui::tokens::to_vec4(ui::tokens::ink::muted), "%s", tag);

    if (toolbar_fn) {
        ImGui::SameLine(0, 24);
        toolbar_fn();
    }

    float ctrl_x = vp.x - (kWindowControlSize * 3 + 8 * 2 + 12);
    ImGui::SetCursorPos(ImVec2(ctrl_x, 8));
    if (window_control("_")) glfwIconifyWindow(win);
    ImGui::SameLine(0, 4);
    if (window_control("[]")) {
        if (glfwGetWindowAttrib(win, GLFW_MAXIMIZED))
            glfwRestoreWindow(win);
        else
            glfwMaximizeWindow(win);
    }
    ImGui::SameLine(0, 4);
    if (window_control("x", true))
        glfwSetWindowShouldClose(win, GLFW_TRUE);

    ImGui::End();
}

}  // namespace smidr
