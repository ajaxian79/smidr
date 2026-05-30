#pragma once

#include "core/Math.h"

namespace smidr {

class App;

enum class GizmoMode {
    None,
    Translate,
    Rotate,
    Scale,
};

enum class GizmoAxis {
    None, X, Y, Z, XY, XZ, YZ, XYZ,
};

struct GizmoState {
    GizmoMode mode = GizmoMode::Translate;
    GizmoAxis active_axis = GizmoAxis::None;
    bool dragging = false;
    Vec3 drag_start_pos;
    Vec3 drag_start_rotation;
    Vec3 drag_start_scale;
    Vec3 hit_point;
    float drag_start_screen_x = 0;
    float drag_start_screen_y = 0;
};

GizmoState& app_gizmo_state();

void draw_gizmo(App& app, GizmoState& state,
                float viewport_x, float viewport_y,
                float viewport_w, float viewport_h);

bool handle_gizmo_interaction(App& app, GizmoState& state,
                               float mouse_x, float mouse_y,
                               float viewport_w, float viewport_h,
                               bool mouse_down, bool mouse_clicked);

}  // namespace smidr
