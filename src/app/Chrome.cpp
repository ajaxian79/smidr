#include "app/Chrome.h"
#include "platform/Platform.h"

#include <algorithm>
#include <cmath>
#include <imgui.h>

namespace smidr {

ChromeCursors create_chrome_cursors() {
    return {
        glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR),
        glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR),
        glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR),
        glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR),
    };
}

void destroy_chrome_cursors(ChromeCursors& c) {
    if (c.hresize) { glfwDestroyCursor(c.hresize); c.hresize = nullptr; }
    if (c.vresize) { glfwDestroyCursor(c.vresize); c.vresize = nullptr; }
    if (c.nwse)    { glfwDestroyCursor(c.nwse);    c.nwse = nullptr; }
    if (c.nesw)    { glfwDestroyCursor(c.nesw);    c.nesw = nullptr; }
}

GLFWwindow* create_undecorated_window(int w, int h, const char* title,
                                       int min_w, int min_h,
                                       bool resizable, bool visible) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, visible ? GLFW_TRUE : GLFW_FALSE);

    GLFWwindow* win = glfwCreateWindow(w, h, title, nullptr, nullptr);
    if (!win) return nullptr;

    platform::force_undecorated_window(win);
    glfwSetWindowSizeLimits(win,
        resizable ? min_w : w, resizable ? min_h : h,
        resizable ? GLFW_DONT_CARE : w, resizable ? GLFW_DONT_CARE : h);
    center_window_on_monitor(win);
    return win;
}

void center_window_on_monitor(GLFWwindow* win) {
    GLFWmonitor* mon = glfwGetPrimaryMonitor();
    if (!mon) return;
    int wx, wy, ww, wh;
    glfwGetMonitorWorkarea(mon, &wx, &wy, &ww, &wh);
    int winw, winh;
    glfwGetWindowSize(win, &winw, &winh);
    glfwSetWindowPos(win, wx + std::max(0, (ww - winw) / 2),
                          wy + std::max(0, (wh - winh) / 2));
}

static bool is_resize(ChromeAction a) {
    return a != ChromeAction::None && a != ChromeAction::Move;
}

static bool has_left(ChromeAction a) {
    return a == ChromeAction::ResizeLeft || a == ChromeAction::ResizeTopLeft ||
           a == ChromeAction::ResizeBottomLeft;
}
static bool has_right(ChromeAction a) {
    return a == ChromeAction::ResizeRight || a == ChromeAction::ResizeTopRight ||
           a == ChromeAction::ResizeBottomRight;
}
static bool has_top(ChromeAction a) {
    return a == ChromeAction::ResizeTop || a == ChromeAction::ResizeTopLeft ||
           a == ChromeAction::ResizeTopRight;
}
static bool has_bottom(ChromeAction a) {
    return a == ChromeAction::ResizeBottom || a == ChromeAction::ResizeBottomLeft ||
           a == ChromeAction::ResizeBottomRight;
}

static ChromeAction action_at(double cx, double cy, int ww, int wh) {
    const double m = kChromeResizeMargin;
    bool l = cx >= 0 && cx <= m;
    bool r = cx <= ww && cx >= ww - m;
    bool t = cy >= 0 && cy <= m;
    bool b = cy <= wh && cy >= wh - m;
    if (t && l) return ChromeAction::ResizeTopLeft;
    if (t && r) return ChromeAction::ResizeTopRight;
    if (b && l) return ChromeAction::ResizeBottomLeft;
    if (b && r) return ChromeAction::ResizeBottomRight;
    if (t) return ChromeAction::ResizeTop;
    if (b) return ChromeAction::ResizeBottom;
    if (l) return ChromeAction::ResizeLeft;
    if (r) return ChromeAction::ResizeRight;
    return ChromeAction::None;
}

static GLFWcursor* cursor_for(const ChromeCursors& c, ChromeAction a) {
    switch (a) {
        case ChromeAction::ResizeLeft: case ChromeAction::ResizeRight: return c.hresize;
        case ChromeAction::ResizeTop: case ChromeAction::ResizeBottom: return c.vresize;
        case ChromeAction::ResizeTopLeft: case ChromeAction::ResizeBottomRight: return c.nwse;
        case ChromeAction::ResizeTopRight: case ChromeAction::ResizeBottomLeft: return c.nesw;
        default: return nullptr;
    }
}

static void begin_action(ChromeState& s, GLFWwindow* win, ChromeAction a,
                          double cx, double cy) {
    s.action = a;
    int wx, wy;
    glfwGetWindowPos(win, &wx, &wy);
    s.start_global_x = wx + cx;
    s.start_global_y = wy + cy;
    s.start_win_x = wx; s.start_win_y = wy;
    glfwGetWindowSize(win, &s.start_win_w, &s.start_win_h);
}

static void update_action(ChromeState& s, GLFWwindow* win, double cx, double cy) {
    int wx, wy;
    glfwGetWindowPos(win, &wx, &wy);
    double gx = wx + cx, gy = wy + cy;
    int dx = static_cast<int>(std::lround(gx - s.start_global_x));
    int dy = static_cast<int>(std::lround(gy - s.start_global_y));

    if (s.action == ChromeAction::Move) {
        glfwSetWindowPos(win, s.start_win_x + dx, s.start_win_y + dy);
        return;
    }

    int x = s.start_win_x, y = s.start_win_y;
    int w = s.start_win_w, h = s.start_win_h;

    if (has_left(s.action)) {
        int right = s.start_win_x + s.start_win_w;
        w = std::max(kMinWindowWidth, s.start_win_w - dx);
        x = right - w;
    } else if (has_right(s.action)) {
        w = std::max(kMinWindowWidth, s.start_win_w + dx);
    }
    if (has_top(s.action)) {
        int bottom = s.start_win_y + s.start_win_h;
        h = std::max(kMinWindowHeight, s.start_win_h - dy);
        y = bottom - h;
    } else if (has_bottom(s.action)) {
        h = std::max(kMinWindowHeight, s.start_win_h + dy);
    }

    if (x != s.start_win_x || y != s.start_win_y)
        glfwSetWindowPos(win, x, y);
    glfwSetWindowSize(win, w, h);
}

void handle_custom_chrome(ChromeState& state, GLFWwindow* win,
                          const ChromeCursors& cursors, bool allow_resize) {
    if (!win || glfwGetWindowAttrib(win, GLFW_ICONIFIED)) return;

    double cx, cy;
    glfwGetCursorPos(win, &cx, &cy);
    int ww, wh;
    glfwGetWindowSize(win, &ww, &wh);

    if (state.action != ChromeAction::None) {
        glfwSetCursor(win, cursor_for(cursors, state.action));
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            update_action(state, win, cx, cy);
        else {
            state.action = ChromeAction::None;
            glfwSetCursor(win, nullptr);
        }
        return;
    }

    if (glfwGetWindowAttrib(win, GLFW_MAXIMIZED)) {
        glfwSetCursor(win, nullptr);
        return;
    }

    ChromeAction ra = allow_resize ? action_at(cx, cy, ww, wh) : ChromeAction::None;
    bool titlebar = cy >= 0 && cy <= kTopChromeHeight;
    bool item_hovered = ImGui::IsAnyItemHovered();
    ChromeAction hover = is_resize(ra) ? ra
                       : (titlebar && !item_hovered ? ChromeAction::Move
                                                    : ChromeAction::None);

    glfwSetCursor(win, cursor_for(cursors, hover));
    if (hover != ChromeAction::None && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        begin_action(state, win, hover, cx, cy);
}

}  // namespace smidr
