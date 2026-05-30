#include "app/Console.h"
#include "app/App.h"
#include "app/CommandRegistry.h"
#include "gui/ui.h"

#include <cstring>
#include <fstream>
#include <sstream>
#include <imgui.h>

namespace smidr {

static char cmd_buf[256] = {};

void execute_command(App& app, const std::string& input) {
    if (input.empty()) return;
    auto& registry = CommandRegistry::instance();
    if (!registry.execute(app, input)) {
        std::istringstream ss(input);
        std::string verb;
        ss >> verb;
        if (!verb.empty())
            app.log(LogEntry::Error, "Unknown command: " + verb + " (type 'help')");
    }
}


void draw_console(App& app) {
    ui::SectionHeader("Console");

    ImGui::BeginChild("##console_scroll",
                      ImVec2(0, -ImGui::GetFrameHeightWithSpacing()),
                      ImGuiChildFlags(0), ImGuiWindowFlags_HorizontalScrollbar);

    for (auto& entry : app.log_entries()) {
        ImU32 col;
        switch (entry.level) {
            case LogEntry::Warning: col = ui::tokens::status::warning; break;
            case LogEntry::Error:   col = ui::tokens::status::destructive; break;
            default:                col = ui::tokens::ink::muted; break;
        }
        ImGui::TextColored(ui::tokens::to_vec4(col), "%s", entry.message.c_str());
    }

    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.f);

    ImGui::EndChild();

    ImGui::SetNextItemWidth(-1);
    bool submitted = ImGui::InputText("##cmd", cmd_buf, sizeof cmd_buf,
                                       ImGuiInputTextFlags_EnterReturnsTrue);
    if (submitted && cmd_buf[0] != '\0') {
        std::string input = cmd_buf;
        app.log(LogEntry::Info, "> " + input);
        execute_command(app, input);
        cmd_buf[0] = '\0';
        ImGui::SetKeyboardFocusHere(-1);
    }
}

}  // namespace smidr
