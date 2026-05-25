#include <cstdio>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "gui/ui.h"

#include "app/App.h"
#include "app/Viewport.h"
#include "app/SceneTreePanel.h"
#include "app/PropertiesPanel.h"
#include "app/Toolbar.h"

static void glfw_error(int code, const char* desc) {
    std::fprintf(stderr, "glfw error %d: %s\n", code, desc);
}

static void draw_titlebar(const smidr::ui::Fonts& fonts, smidr::App& app) {
    const float h = 44.0f;
    const ImVec2 vp = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(vp.x, h));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##titlebar", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);
    ImGui::PopStyleVar();

    auto* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilledMultiColor(
        ImVec2(0, 0), ImVec2(vp.x, h),
        smidr::ui::tokens::surface::deep, smidr::ui::tokens::surface::deep,
        smidr::ui::tokens::surface::window, smidr::ui::tokens::surface::window);

    if (fonts.sans_md) ImGui::PushFont(fonts.sans_md);
    dl->AddText(ImVec2(20, 14), smidr::ui::tokens::ink::primary, "smidr");
    if (fonts.sans_md) ImGui::PopFont();

    const char* docname = app.document().name().c_str();
    char title[256];
    std::snprintf(title, sizeof title, "/ %s%s", docname,
                  app.document().dirty() ? " *" : "");
    dl->AddText(ImVec2(80, 16), smidr::ui::tokens::ink::muted, title);

    const float right = vp.x - 24.f;
    ImGui::SetCursorScreenPos(ImVec2(right - 120.f, 10.f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
        smidr::ui::tokens::to_vec4(smidr::ui::tokens::surface::control));

    if (ImGui::Button(smidr::ui::icons::kSave, ImVec2(24, 24))) {
        auto& doc = app.document();
        if (doc.path().empty()) {
            doc.save("untitled.smidr");
            app.log(smidr::LogEntry::Info, "Saved untitled.smidr");
        } else {
            doc.save(doc.path());
            app.log(smidr::LogEntry::Info, "Saved " + doc.path().string());
        }
    }
    ImGui::SameLine();
    ImGui::Button(smidr::ui::icons::kSearch, ImVec2(24, 24));
    ImGui::SameLine();
    ImGui::Button(smidr::ui::icons::kCog, ImVec2(24, 24));

    ImGui::PopStyleColor(2);
    ImGui::End();
}

static void draw_workspace_strip(int* workspace, int* document) {
    const float top = 44.f;
    const ImVec2 vp = ImGui::GetIO().DisplaySize;
    const float h = smidr::ui::tokens::sizes::wsbar_h + smidr::ui::tokens::sizes::doctab_h;

    ImGui::SetNextWindowPos(ImVec2(0, top));
    ImGui::SetNextWindowSize(ImVec2(vp.x, h));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##wsstrip", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);
    ImGui::PopStyleVar();

    static const smidr::ui::WorkspaceTab ws[] = {
        {"model",   "Model",   smidr::ui::icons::kLayers},
        {"inspect", "Inspect", smidr::ui::icons::kEye},
        {"render",  "Render",  smidr::ui::icons::kRender},
    };
    smidr::ui::WorkspaceTabs(std::span<const smidr::ui::WorkspaceTab>(ws, 3), workspace);

    static smidr::ui::DocumentTab docs[] = {
        {"Untitled", false},
    };
    int closed = -1;
    smidr::ui::DocumentTabs(std::span<const smidr::ui::DocumentTab>(docs, 1), document, &closed);

    ImGui::End();
}

static void draw_main_layout(smidr::App& app) {
    auto& io = ImGui::GetIO();
    const float top = 44.f + smidr::ui::tokens::sizes::wsbar_h + smidr::ui::tokens::sizes::doctab_h;

    ImGui::SetNextWindowPos(ImVec2(0, top));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y - top));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##body", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);
    ImGui::PopStyleVar();

    // Toolbar strip
    smidr::draw_toolbar(app);

    const float console_h = 130.f;
    const float side_w    = 240.f;
    const float insp_w    = 300.f;
    const float body_h    = ImGui::GetContentRegionAvail().y - console_h;

    // Left panel: scene tree
    ImGui::PushStyleColor(ImGuiCol_ChildBg,
        smidr::ui::tokens::to_vec4(smidr::ui::tokens::surface::panel));
    ImGui::BeginChild("##scene_tree", ImVec2(side_w, body_h), ImGuiChildFlags(0),
                      ImGuiWindowFlags_NoScrollbar);
    ImGui::Indent(14);
    ImGui::Dummy(ImVec2(0, 8));
    smidr::draw_scene_tree(app);
    ImGui::Unindent(14);
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::SameLine(0, 1);

    // Center: viewport
    float viewport_w = io.DisplaySize.x - side_w - insp_w - 2;
    ImGui::BeginChild("##viewport_area", ImVec2(viewport_w, body_h), ImGuiChildFlags(0));
    smidr::draw_viewport(app);
    ImGui::EndChild();

    ImGui::SameLine(0, 1);

    // Right panel: properties
    ImGui::PushStyleColor(ImGuiCol_ChildBg,
        smidr::ui::tokens::to_vec4(smidr::ui::tokens::surface::panel));
    ImGui::BeginChild("##properties", ImVec2(insp_w, body_h), ImGuiChildFlags(0));
    ImGui::Indent(14);
    ImGui::Dummy(ImVec2(0, 8));
    smidr::draw_properties(app);
    ImGui::Unindent(14);
    ImGui::EndChild();
    ImGui::PopStyleColor();

    // Bottom: console
    ImGui::PushStyleColor(ImGuiCol_ChildBg,
        smidr::ui::tokens::to_vec4(smidr::ui::tokens::surface::panel_alt));
    ImGui::BeginChild("##console", ImVec2(0, console_h), ImGuiChildFlags(0));
    ImGui::Indent(14);
    ImGui::Dummy(ImVec2(0, 4));
    smidr::draw_console(app);
    ImGui::Unindent(14);
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::End();
}

static void draw_splash(const smidr::ui::Fonts& fonts) {
    auto& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##splash_win", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);
    ImGui::PopStyleVar();

    static const smidr::ui::SplashRecent recents[] = {
        {"example_assembly.smidr", "~/projects/smidr/examples", "just now"},
        {"bracket_v2.smidr",       "~/projects/mechanical",     "yesterday"},
        {"housing.smidr",          "~/projects/enclosures",     "last week"},
    };
    static const smidr::ui::SplashAction actions[] = {
        {"New document",   "Ctrl+N"},
        {"Open file...",   "Ctrl+O"},
        {"Documentation",  "F1"},
    };
    smidr::ui::Splash("smidr",
                  "craft - model - build",
                  std::span<const smidr::ui::SplashRecent>(recents, 3),
                  std::span<const smidr::ui::SplashAction>(actions, 3),
                  0, ImVec2(0, 0), fonts.hero);
    ImGui::End();
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    glfwSetErrorCallback(glfw_error);
    if (!glfwInit()) return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    GLFWwindow* win = glfwCreateWindow(1600, 1000, "smidr", nullptr, nullptr);
    if (!win) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    smidr::ui::ApplyDefaults(smidr::ui::tokens::accents::cyan);

    std::string font_dir = SMIDR_FONT_DIR;
    smidr::ui::Fonts fonts = smidr::ui::LoadFonts(io, font_dir.c_str(), 14.f, 13.f, 80.f);

    ImGui_ImplGlfw_InitForOpenGL(win, true);
    ImGui_ImplOpenGL3_Init("#version 150");

    smidr::App app;
    app.init(win, fonts);

    int workspace = 0, document = 0;

    while (!glfwWindowShouldClose(win) && !app.wants_quit()) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (app.show_splash) {
            draw_splash(fonts);
            if (ImGui::IsKeyPressed(ImGuiKey_Escape) ||
                ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                app.show_splash = false;
            }
        } else {
            app.update();
            draw_titlebar(fonts, app);
            draw_workspace_strip(&workspace, &document);
            draw_main_layout(app);
        }

        ImGui::Render();
        int dw, dh;
        glfwGetFramebufferSize(win, &dw, &dh);
        glViewport(0, 0, dw, dh);
        auto bg = smidr::ui::tokens::to_vec4(smidr::ui::tokens::surface::window);
        glClearColor(bg.x, bg.y, bg.z, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(win);
    }

    app.renderer().cleanup();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
