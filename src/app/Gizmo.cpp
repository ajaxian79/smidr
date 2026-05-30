#include "app/Gizmo.h"
#include "app/App.h"
#include "core/RayCast.h"

#include <algorithm>
#include <cmath>
#include <imgui.h>

namespace smidr {

GizmoState& app_gizmo_state() {
    static GizmoState state;
    return state;
}

static ImVec2 project(Vec3 world, const Mat4& view, const Mat4& proj,
                       float vp_x, float vp_y, float vp_w, float vp_h) {
    Vec3 v_pos = view.transform_point(world);
    Vec3 c_pos = proj.transform_point(v_pos);
    float sx = vp_x + (c_pos.x * 0.5f + 0.5f) * vp_w;
    float sy = vp_y + (1.f - (c_pos.y * 0.5f + 0.5f)) * vp_h;
    return {sx, sy};
}

void draw_gizmo(App& app, GizmoState& state,
                float vp_x, float vp_y, float vp_w, float vp_h) {
    auto sel = app.document().scene().selected_id();
    if (!sel) return;
    auto* node = app.document().scene().find(*sel);
    if (!node) return;

    Vec3 origin = node->position;
    auto& cam = app.camera();
    float aspect = vp_w / vp_h;
    Mat4 view = cam.view_matrix();
    Mat4 proj = cam.projection_matrix(aspect);

    Vec3 dir_view = (origin - cam.eye()).normalized();
    float scale_factor = (origin - cam.eye()).length() * 0.15f;

    auto* dl = ImGui::GetForegroundDrawList();
    ImVec2 center = project(origin, view, proj, vp_x, vp_y, vp_w, vp_h);

    Vec3 ax_x = {scale_factor, 0, 0};
    Vec3 ax_y = {0, scale_factor, 0};
    Vec3 ax_z = {0, 0, scale_factor};

    ImVec2 x_end = project(origin + ax_x, view, proj, vp_x, vp_y, vp_w, vp_h);
    ImVec2 y_end = project(origin + ax_y, view, proj, vp_x, vp_y, vp_w, vp_h);
    ImVec2 z_end = project(origin + ax_z, view, proj, vp_x, vp_y, vp_w, vp_h);

    ImU32 col_x = state.active_axis == GizmoAxis::X ? IM_COL32(255,255,80,255) : IM_COL32(220,80,80,255);
    ImU32 col_y = state.active_axis == GizmoAxis::Y ? IM_COL32(255,255,80,255) : IM_COL32(80,220,80,255);
    ImU32 col_z = state.active_axis == GizmoAxis::Z ? IM_COL32(255,255,80,255) : IM_COL32(80,120,255,255);

    if (state.mode == GizmoMode::Translate || state.mode == GizmoMode::Scale) {
        dl->AddLine(center, x_end, col_x, 3.f);
        dl->AddLine(center, y_end, col_y, 3.f);
        dl->AddLine(center, z_end, col_z, 3.f);
        if (state.mode == GizmoMode::Translate) {
            dl->AddCircleFilled(x_end, 6.f, col_x);
            dl->AddCircleFilled(y_end, 6.f, col_y);
            dl->AddCircleFilled(z_end, 6.f, col_z);
        } else {
            dl->AddRectFilled({x_end.x-5,x_end.y-5},{x_end.x+5,x_end.y+5}, col_x);
            dl->AddRectFilled({y_end.x-5,y_end.y-5},{y_end.x+5,y_end.y+5}, col_y);
            dl->AddRectFilled({z_end.x-5,z_end.y-5},{z_end.x+5,z_end.y+5}, col_z);
        }
    } else if (state.mode == GizmoMode::Rotate) {
        constexpr int segs = 48;
        auto draw_ring = [&](Vec3 normal, ImU32 col) {
            Vec3 u = (std::abs(normal.y) < 0.9f ? normal.cross({0,1,0}) : normal.cross({1,0,0})).normalized();
            Vec3 v = normal.cross(u).normalized();
            ImVec2 prev{0,0};
            for (int i = 0; i <= segs; ++i) {
                float t = static_cast<float>(i) / static_cast<float>(segs) * kTwoPi;
                Vec3 p = origin + (u * std::cos(t) + v * std::sin(t)) * scale_factor;
                ImVec2 sp = project(p, view, proj, vp_x, vp_y, vp_w, vp_h);
                if (i > 0) dl->AddLine(prev, sp, col, 2.f);
                prev = sp;
            }
        };
        draw_ring({1,0,0}, col_x);
        draw_ring({0,1,0}, col_y);
        draw_ring({0,0,1}, col_z);
    }

    dl->AddCircleFilled(center, 4.f, IM_COL32(255,255,255,200));
    (void)dir_view;
}

static GizmoAxis pick_axis(float mouse_x, float mouse_y, ImVec2 center,
                            ImVec2 x_end, ImVec2 y_end, ImVec2 z_end) {
    auto dist_to_segment = [](float mx, float my, ImVec2 a, ImVec2 b) -> float {
        float dx = b.x - a.x, dy = b.y - a.y;
        float len2 = dx*dx + dy*dy;
        if (len2 < 1e-6f) return std::sqrt((mx-a.x)*(mx-a.x) + (my-a.y)*(my-a.y));
        float t = ((mx-a.x)*dx + (my-a.y)*dy) / len2;
        t = std::max(0.f, std::min(1.f, t));
        float px = a.x + t*dx, py = a.y + t*dy;
        return std::sqrt((mx-px)*(mx-px) + (my-py)*(my-py));
    };

    float d_x = dist_to_segment(mouse_x, mouse_y, center, x_end);
    float d_y = dist_to_segment(mouse_x, mouse_y, center, y_end);
    float d_z = dist_to_segment(mouse_x, mouse_y, center, z_end);
    constexpr float thresh = 8.f;
    float min_d = std::min({d_x, d_y, d_z});
    if (min_d > thresh) return GizmoAxis::None;
    if (min_d == d_x) return GizmoAxis::X;
    if (min_d == d_y) return GizmoAxis::Y;
    return GizmoAxis::Z;
}

bool handle_gizmo_interaction(App& app, GizmoState& state,
                               float mouse_x, float mouse_y,
                               float vp_w, float vp_h,
                               bool mouse_down, bool mouse_clicked) {
    auto sel = app.document().scene().selected_id();
    if (!sel) { state.active_axis = GizmoAxis::None; state.dragging = false; return false; }
    auto* node = app.document().scene().find(*sel);
    if (!node) return false;

    auto& cam = app.camera();
    float aspect = vp_w / vp_h;
    Mat4 view = cam.view_matrix();
    Mat4 proj = cam.projection_matrix(aspect);

    Vec3 origin = node->position;
    float scale_factor = (origin - cam.eye()).length() * 0.15f;

    ImVec2 center = project(origin, view, proj, 0, 0, vp_w, vp_h);
    ImVec2 x_end = project(origin + Vec3{scale_factor,0,0}, view, proj, 0, 0, vp_w, vp_h);
    ImVec2 y_end = project(origin + Vec3{0,scale_factor,0}, view, proj, 0, 0, vp_w, vp_h);
    ImVec2 z_end = project(origin + Vec3{0,0,scale_factor}, view, proj, 0, 0, vp_w, vp_h);

    if (!state.dragging) {
        state.active_axis = pick_axis(mouse_x, mouse_y, center, x_end, y_end, z_end);
    }

    if (state.active_axis == GizmoAxis::None) return false;

    if (mouse_clicked) {
        state.dragging = true;
        state.drag_start_pos = node->position;
        state.drag_start_rotation = node->rotation;
        state.drag_start_scale = node->scale_vec;
        state.drag_start_screen_x = mouse_x;
        state.drag_start_screen_y = mouse_y;
        return true;
    }

    if (state.dragging && mouse_down) {
        float dx = mouse_x - state.drag_start_screen_x;
        float dy = mouse_y - state.drag_start_screen_y;
        float delta = (dx - dy) * 0.01f;

        if (state.mode == GizmoMode::Translate) {
            Vec3 d{0,0,0};
            if (state.active_axis == GizmoAxis::X) d.x = delta;
            else if (state.active_axis == GizmoAxis::Y) d.y = delta;
            else if (state.active_axis == GizmoAxis::Z) d.z = delta;
            node->position = state.drag_start_pos + d * scale_factor * 10.f;
            app.document().mark_dirty();
        } else if (state.mode == GizmoMode::Rotate) {
            Vec3 r = state.drag_start_rotation;
            if (state.active_axis == GizmoAxis::X) r.x += delta * 50.f;
            else if (state.active_axis == GizmoAxis::Y) r.y += delta * 50.f;
            else if (state.active_axis == GizmoAxis::Z) r.z += delta * 50.f;
            node->rotation = r;
            app.document().mark_dirty();
        } else if (state.mode == GizmoMode::Scale) {
            float s = 1.f + delta;
            Vec3 sc = state.drag_start_scale;
            if (state.active_axis == GizmoAxis::X) sc.x *= s;
            else if (state.active_axis == GizmoAxis::Y) sc.y *= s;
            else if (state.active_axis == GizmoAxis::Z) sc.z *= s;
            else sc = state.drag_start_scale * s;
            node->scale_vec = sc;
            app.document().mark_dirty();
        }
        return true;
    }

    if (!mouse_down) state.dragging = false;
    return false;
}

}  // namespace smidr
