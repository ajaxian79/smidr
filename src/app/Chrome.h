#pragma once

#include <GLFW/glfw3.h>

namespace smidr {

constexpr float kTopChromeHeight    = 44.0f;
constexpr float kMenuBarHeight      = 28.0f;
constexpr float kStatusBarHeight    = 24.0f;
constexpr double kChromeResizeMargin = 8.0;
constexpr int kMinWindowWidth       = 760;
constexpr int kMinWindowHeight      = 460;
constexpr int kSplashWidth          = 960;
constexpr int kSplashHeight         = 560;
constexpr float kWindowControlSize  = 28.0f;

enum class ChromeAction {
    None,
    Move,
    ResizeLeft, ResizeRight, ResizeTop, ResizeBottom,
    ResizeTopLeft, ResizeTopRight, ResizeBottomLeft, ResizeBottomRight,
};

struct ChromeCursors {
    GLFWcursor* hresize = nullptr;
    GLFWcursor* vresize = nullptr;
    GLFWcursor* nwse    = nullptr;
    GLFWcursor* nesw    = nullptr;
};

struct ChromeState {
    ChromeAction action = ChromeAction::None;
    double start_global_x = 0.0;
    double start_global_y = 0.0;
    int start_win_x = 0, start_win_y = 0;
    int start_win_w = 0, start_win_h = 0;
};

ChromeCursors create_chrome_cursors();
void destroy_chrome_cursors(ChromeCursors& c);

GLFWwindow* create_undecorated_window(int w, int h, const char* title,
                                       int min_w, int min_h,
                                       bool resizable, bool visible);
void center_window_on_monitor(GLFWwindow* win);

void handle_custom_chrome(ChromeState& state, GLFWwindow* win,
                          const ChromeCursors& cursors,
                          bool allow_resize = true);

}  // namespace smidr
