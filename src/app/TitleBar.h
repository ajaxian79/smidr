#pragma once

#include <functional>

struct GLFWwindow;
namespace smidr::ui { struct Fonts; }

namespace smidr {

class App;

void draw_titlebar(App& app, const ui::Fonts& fonts, GLFWwindow* win,
                   std::function<void()> toolbar_fn = {});

}  // namespace smidr
