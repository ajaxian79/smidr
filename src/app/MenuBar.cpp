#include "app/MenuBar.h"
#include "app/App.h"
#include "app/Chrome.h"
#include "io/MeshExport.h"
#include "gui/ui.h"

#include <imgui.h>

namespace smidr {

static constexpr ImGuiWindowFlags kChromeFlags =
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
    ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

void draw_menubar(App& app) {
    const ImVec2 vp = ImGui::GetIO().DisplaySize;

    ImGui::SetNextWindowPos(ImVec2(0, kTopChromeHeight));
    ImGui::SetNextWindowSize(ImVec2(vp.x, kMenuBarHeight));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 2));
    ImGui::Begin("##menubar", nullptr, kChromeFlags);
    ImGui::PopStyleVar();

    auto* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(ImVec2(0, kTopChromeHeight),
                      ImVec2(vp.x, kTopChromeHeight + kMenuBarHeight),
                      ui::tokens::surface::panel);
    dl->AddLine(ImVec2(0, kTopChromeHeight + kMenuBarHeight - 1),
                ImVec2(vp.x, kTopChromeHeight + kMenuBarHeight - 1),
                ui::tokens::border::separator, 1.f);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New", "Ctrl+N"))
                app.document().new_document();
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                auto& doc = app.document();
                if (doc.path().empty())
                    doc.save("untitled.smidr");
                else
                    doc.save(doc.path());
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Export STL...")) {
                app.mesh_gen().rebuild(app.document().scene());
                if (export_stl("export.stl", app.mesh_gen()))
                    app.log(LogEntry::Info, "Exported export.stl");
                else
                    app.log(LogEntry::Error, "STL export failed");
            }
            if (ImGui::MenuItem("Export OBJ...")) {
                app.mesh_gen().rebuild(app.document().scene());
                if (export_obj("export.obj", app.mesh_gen()))
                    app.log(LogEntry::Info, "Exported export.obj");
                else
                    app.log(LogEntry::Error, "OBJ export failed");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit"))
                app.request_quit();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, app.history().can_undo()))
                app.undo();
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, app.history().can_redo()))
                app.redo();
            ImGui::Separator();
            if (ImGui::MenuItem("Delete", "Del"))
                app.delete_selected();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Create")) {
            if (ImGui::MenuItem("Sphere"))   app.add_primitive(PrimitiveType::Sphere);
            if (ImGui::MenuItem("Box"))      app.add_primitive(PrimitiveType::Box);
            if (ImGui::MenuItem("Cylinder")) app.add_primitive(PrimitiveType::Cylinder);
            if (ImGui::MenuItem("Cone"))     app.add_primitive(PrimitiveType::Cone);
            if (ImGui::MenuItem("Torus"))    app.add_primitive(PrimitiveType::Torus);
            ImGui::Separator();
            if (ImGui::MenuItem("Ellipsoid")) app.add_primitive(PrimitiveType::Ellipsoid);
            if (ImGui::MenuItem("Pipe"))      app.add_primitive(PrimitiveType::Pipe);
            if (ImGui::MenuItem("Wedge"))     app.add_primitive(PrimitiveType::Wedge);
            if (ImGui::MenuItem("Arb8"))      app.add_primitive(PrimitiveType::Arb8);
            if (ImGui::MenuItem("Halfspace")) app.add_primitive(PrimitiveType::Halfspace);
            ImGui::Separator();
            if (ImGui::MenuItem("Group"))
                app.document().scene().add_group("Group");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Modify")) {
            if (ImGui::MenuItem("Union"))        app.boolean_selected(BooleanOp::Union);
            if (ImGui::MenuItem("Intersection")) app.boolean_selected(BooleanOp::Intersection);
            if (ImGui::MenuItem("Difference"))   app.boolean_selected(BooleanOp::Difference);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Wireframe", nullptr, &app.renderer().wireframe);
            if (ImGui::MenuItem("Frame All", "F")) {
                AABB bb;
                app.document().scene().for_each([&](const SceneNode& n) {
                    if (n.primitive) bb.merge(n.primitive->local_bounds());
                });
                app.camera().frame(bb);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            ImGui::MenuItem("About Smidr");
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::End();
}

}  // namespace smidr
