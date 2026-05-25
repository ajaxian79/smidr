#pragma once

#include <string>

namespace smidr {
class App;

void draw_console(App& app);
void execute_command(App& app, const std::string& input);

}  // namespace smidr
