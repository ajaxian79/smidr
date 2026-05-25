#include <cstdio>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "gui/ui.h"
#include "app/App.h"
#include "app/Chrome.h"
#include "app/TitleBar.h"
#include "app/MenuBar.h"
#include "app/StatusBar.h"
#include "app/Splash.h"
#include "app/Viewport.h"
#include "app/SceneTreePanel.h"
#include "app/PropertiesPanel.h"
#include "app/Toolbar.h"

static void glfw_error(int code, const char* desc) {
    std::fprintf(stderr, "glfw error %d: %s\n", code, desc);
}

static void draw_workspace(smidr::App& app, const smidr::ui::Fonts& fonts,
                           GLFWwindow* win) {
    using namespace smidr;
    const auto& io = ImGui::GetIO();
    const float content_top = kTopChromeHeight + kMenuBarHeight;
    const float content_h = io.DisplaySize.y - content_top - kStatusBarHeight;

    draw_titlebar(app, fonts, win, [&]{ draw_toolbar(app); });
    draw_menubar(app);

    ImGui::SetNextWindowPos(ImVec2(0, content_top));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, content_h));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##body", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);
    ImGui::PopStyleVar();

    const float side_w = 240.f, insp_w = 280.f;
    const float body_h = ImGui::GetContentRegionAvail().y;

    ImGui::PushStyleColor(ImGuiCol_ChildBg,
        ui::tokens::to_vec4(ui::tokens::surface::panel));
    ImGui::BeginChild("##tree", ImVec2(side_w, body_h));
    ImGui::Indent(14); ImGui::Dummy(ImVec2(0, 8));
    draw_scene_tree(app);
    ImGui::Unindent(14);
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::SameLine(0, 1);
    ImGui::BeginChild("##vp", ImVec2(io.DisplaySize.x - side_w - insp_w - 2, body_h));
    draw_viewport(app);
    ImGui::EndChild();

    ImGui::SameLine(0, 1);
    ImGui::PushStyleColor(ImGuiCol_ChildBg,
        ui::tokens::to_vec4(ui::tokens::surface::panel));
    ImGui::BeginChild("##props", ImVec2(insp_w, body_h));
    ImGui::Indent(14); ImGui::Dummy(ImVec2(0, 8));
    draw_properties(app);
    ImGui::Unindent(14);
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::End();

    draw_statusbar(app);
}

int main(int, char**) {
    glfwSetErrorCallback(glfw_error);
    if (!glfwInit()) return 1;

    auto cursors = smidr::create_chrome_cursors();
    GLFWwindow* win = smidr::create_undecorated_window(
        smidr::kSplashWidth, smidr::kSplashHeight, "smidr",
        smidr::kSplashWidth, smidr::kSplashHeight, false, false);
    if (!win) { glfwTerminate(); return 1; }

    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    smidr::ui::ApplyDefaults(smidr::ui::tokens::accents::cyan);
    auto fonts = smidr::ui::LoadFonts(io, SMIDR_FONT_DIR, 14.f, 13.f, 80.f);

    ImGui_ImplGlfw_InitForOpenGL(win, true);
    ImGui_ImplOpenGL3_Init("#version 150");

    smidr::App app;
    app.init(win, fonts);
    smidr::ChromeState chrome{};

    glfwShowWindow(win);

    bool in_splash = true;
    while (!glfwWindowShouldClose(win) && !app.wants_quit()) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        smidr::handle_custom_chrome(chrome, win, cursors, !in_splash);

        if (in_splash) {
            auto action = smidr::draw_splash_screen(fonts);
            if (action != smidr::SplashAction::None) {
                smidr::transition_splash_to_workspace(win);
                in_splash = false;
            }
        } else {
            auto& io = ImGui::GetIO();
            bool ctrl = io.KeyCtrl || io.KeySuper;
            if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) app.undo();
            if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) app.redo();
            if (ImGui::IsKeyPressed(ImGuiKey_Delete, false))    app.delete_selected();

            app.update();
            draw_workspace(app, fonts, win);
        }

        ImGui::Render();
        int dw, dh;
        glfwGetFramebufferSize(win, &dw, &dh);
        glViewport(0, 0, dw, dh);
        constexpr ImU32 bg = smidr::ui::tokens::surface::deep;
        glClearColor(float((bg >> IM_COL32_R_SHIFT) & 0xff) / 255.f,
                     float((bg >> IM_COL32_G_SHIFT) & 0xff) / 255.f,
                     float((bg >> IM_COL32_B_SHIFT) & 0xff) / 255.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(win);
    }

    app.renderer().cleanup();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    smidr::destroy_chrome_cursors(cursors);
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
