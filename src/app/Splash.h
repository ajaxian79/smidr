#pragma once

#include <GLFW/glfw3.h>

namespace smidr::ui { struct Fonts; }

namespace smidr {

enum class SplashAction { None, NewDocument, OpenFile };

SplashAction draw_splash_screen(const ui::Fonts& fonts);

void transition_splash_to_workspace(GLFWwindow* win);

}  // namespace smidr
