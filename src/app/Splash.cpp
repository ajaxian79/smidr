#include "app/Splash.h"
#include "app/Chrome.h"
#include "gui/ui.h"

#include <algorithm>
#include <imgui.h>

namespace smidr {

SplashAction draw_splash_screen(const ui::Fonts& fonts) {
    auto& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##splash", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);
    ImGui::PopStyleVar();

    static const ui::SplashAction actions[] = {
        {"New document",  "Ctrl+N"},
        {"Open file...",  "Ctrl+O"},
        {"Documentation", "F1"},
    };

    auto choice = ui::Splash("smidr", "craft - model - build",
                              {}, std::span<const ui::SplashAction>(actions, 3),
                              0, ImVec2(0, 0), fonts.hero);

    ImGui::End();

    if (choice.kind == ui::SplashChoice::Kind::Action) {
        switch (choice.index) {
            case 0: return SplashAction::NewDocument;
            case 1: return SplashAction::OpenFile;
            default: break;
        }
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Escape) ||
        (ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
         choice.kind == ui::SplashChoice::Kind::None))
        return SplashAction::NewDocument;

    bool ctrl = io.KeyCtrl || io.KeySuper;
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_N, false))
        return SplashAction::NewDocument;

    return SplashAction::None;
}

void transition_splash_to_workspace(GLFWwindow* win) {
    glfwSetWindowAttrib(win, GLFW_RESIZABLE, GLFW_TRUE);
    glfwSetWindowSizeLimits(win, kMinWindowWidth, kMinWindowHeight,
                            GLFW_DONT_CARE, GLFW_DONT_CARE);

    GLFWmonitor* mon = glfwGetPrimaryMonitor();
    if (mon) {
        int wx, wy, ww, wh;
        glfwGetMonitorWorkarea(mon, &wx, &wy, &ww, &wh);
        int margin_x = std::min(50, static_cast<int>(ww * 0.05));
        int margin_top = std::min(50, static_cast<int>(wh * 0.05));
        int margin_bot = std::min(150, static_cast<int>(wh * 0.15));
        int new_w = std::max(kMinWindowWidth, ww - margin_x * 2);
        int new_h = std::max(kMinWindowHeight, wh - margin_top - margin_bot);
        glfwSetWindowSize(win, new_w, new_h);
        glfwSetWindowPos(win, wx + margin_x, wy + margin_top);
    } else {
        glfwSetWindowSize(win, 1400, 900);
        center_window_on_monitor(win);
    }
}

}  // namespace smidr
