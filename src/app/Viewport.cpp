#include "app/Viewport.h"
#include "app/App.h"
#include "core/RayCast.h"

#include <imgui.h>

namespace smidr {

void draw_viewport(App& app) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::BeginChild("##viewport", ImVec2(0, 0), ImGuiChildFlags(0),
                      ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleVar();

    ImVec2 avail = ImGui::GetContentRegionAvail();
    int vw = static_cast<int>(avail.x);
    int vh = static_cast<int>(avail.y);
    if (vw < 1) vw = 1;
    if (vh < 1) vh = 1;

    auto& renderer = app.renderer();
    if (vw != renderer.width() || vh != renderer.height())
        renderer.resize(vw, vh);

    renderer.render(app.camera(), app.mesh_gen());

    ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
    ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(renderer.texture())),
                 avail, ImVec2(0, 1), ImVec2(1, 0));

    if (ImGui::IsItemHovered()) {
        auto& io = ImGui::GetIO();
        auto& cam = app.camera();

        if (io.MouseWheel != 0.f)
            cam.zoom(io.MouseWheel);

        if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
            if (io.KeyShift)
                cam.pan(io.MouseDelta.x, io.MouseDelta.y);
            else
                cam.orbit(io.MouseDelta.x, io.MouseDelta.y);
        }

        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
            cam.pan(io.MouseDelta.x, io.MouseDelta.y);

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            !ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            float mx = io.MousePos.x - cursor_pos.x;
            float my = io.MousePos.y - cursor_pos.y;
            float aspect = static_cast<float>(vw) / static_cast<float>(vh);
            Mat4 view = cam.view_matrix();
            Mat4 proj = cam.projection_matrix(aspect);

            Ray ray = screen_to_ray(mx, my,
                                     static_cast<float>(vw),
                                     static_cast<float>(vh),
                                     view, proj);

            auto hit = ray_cast(ray, app.mesh_gen());
            if (hit) {
                app.document().scene().select(hit->node_id);
                app.document().mark_dirty();
            } else {
                app.document().scene().clear_selection();
            }
        }
    }

    ImGui::EndChild();
}

}  // namespace smidr
