#include "app/Console.h"
#include "app/App.h"
#include "core/Analysis.h"
#include "io/MeshExport.h"
#include "render/RayTracer.h"
#include "gui/ui.h"

#include <cstring>
#include <fstream>
#include <sstream>
#include <imgui.h>

namespace smidr {

static char cmd_buf[256] = {};

void execute_command(App& app, const std::string& input) {
    std::istringstream ss(input);
    std::string verb;
    ss >> verb;

    if (verb == "in" || verb == "create") {
        std::string type_name;
        ss >> type_name;
        if (type_name.empty()) {
            app.log(LogEntry::Warning, "Usage: in <type> [name]");
            return;
        }
        try {
            auto pt = primitive_type_from_name(type_name);
            app.add_primitive(pt);
        } catch (...) {
            app.log(LogEntry::Error, "Unknown type: " + type_name);
        }
    } else if (verb == "kill" || verb == "delete") {
        app.delete_selected();
    } else if (verb == "undo") {
        app.undo();
    } else if (verb == "redo") {
        app.redo();
    } else if (verb == "ls" || verb == "list") {
        app.document().scene().for_each([&](const SceneNode& n) {
            char line[128];
            std::snprintf(line, sizeof line, "  [%u] %s (%s)",
                          n.id, n.name.c_str(),
                          n.primitive ? primitive_type_name(n.primitive->type()) : "group");
            app.log(LogEntry::Info, line);
        });
    } else if (verb == "select" || verb == "sel") {
        unsigned int id = 0;
        if (ss >> id) {
            auto* node = app.document().scene().find(static_cast<NodeId>(id));
            if (node) {
                app.document().scene().select(static_cast<NodeId>(id));
                app.log(LogEntry::Info, "Selected " + node->name);
            } else {
                app.log(LogEntry::Error, "No node with id " + std::to_string(id));
            }
        }
    } else if (verb == "translate" || verb == "tra") {
        auto sel = app.document().scene().selected_id();
        if (!sel) { app.log(LogEntry::Warning, "Nothing selected"); return; }
        auto* n = app.document().scene().find(*sel);
        if (!n) return;
        float x = 0, y = 0, z = 0;
        ss >> x >> y >> z;
        n->position = Vec3{x, y, z};
        app.document().mark_dirty();
        app.log(LogEntry::Info, "Moved " + n->name);
    } else if (verb == "scale" || verb == "sca") {
        auto sel = app.document().scene().selected_id();
        if (!sel) { app.log(LogEntry::Warning, "Nothing selected"); return; }
        auto* n = app.document().scene().find(*sel);
        if (!n) return;
        float s = 1;
        ss >> s;
        n->scale_vec = Vec3{s, s, s};
        app.document().mark_dirty();
    } else if (verb == "wireframe" || verb == "wire") {
        app.renderer().wireframe = !app.renderer().wireframe;
    } else if (verb == "save") {
        std::string path;
        ss >> path;
        if (path.empty()) path = "untitled.smidr";
        if (app.document().save(path))
            app.log(LogEntry::Info, "Saved " + path);
        else
            app.log(LogEntry::Error, "Save failed: " + path);
    } else if (verb == "load") {
        std::string path;
        ss >> path;
        if (path.empty()) { app.log(LogEntry::Warning, "Usage: load <path>"); return; }
        if (app.document().load(path))
            app.log(LogEntry::Info, "Loaded " + path);
        else
            app.log(LogEntry::Error, "Load failed: " + path);
    } else if (verb == "analyze" || verb == "ana") {
        app.mesh_gen().rebuild(app.document().scene());
        for (auto& e : app.mesh_gen().entries()) {
            auto r = analyze_mesh(e.mesh);
            auto* node = app.document().scene().find(e.node_id);
            std::string name = node ? node->name : "?";
            char buf[512];
            std::snprintf(buf, sizeof buf,
                "%s: vol=%.4f area=%.4f tris=%d verts=%d centroid=(%.3f,%.3f,%.3f)",
                name.c_str(), r.volume, r.surface_area, r.triangle_count,
                r.vertex_count, r.centroid.x, r.centroid.y, r.centroid.z);
            app.log(LogEntry::Info, buf);
            std::snprintf(buf, sizeof buf,
                "  mass=%.4f Ixx=%.4f Iyy=%.4f Izz=%.4f",
                r.mass, r.Ixx, r.Iyy, r.Izz);
            app.log(LogEntry::Info, buf);
        }
    } else if (verb == "export") {
        std::string fmt, path;
        ss >> fmt >> path;
        if (fmt.empty()) { app.log(LogEntry::Warning, "Usage: export stl|obj <path>"); return; }
        app.mesh_gen().rebuild(app.document().scene());
        if (fmt == "stl") {
            if (path.empty()) path = "export.stl";
            export_stl(path, app.mesh_gen()) ? app.log(LogEntry::Info, "Exported " + path)
                                              : app.log(LogEntry::Error, "Export failed");
        } else if (fmt == "obj") {
            if (path.empty()) path = "export.obj";
            export_obj(path, app.mesh_gen()) ? app.log(LogEntry::Info, "Exported " + path)
                                              : app.log(LogEntry::Error, "Export failed");
        } else {
            app.log(LogEntry::Error, "Unknown format: " + fmt);
        }
    } else if (verb == "rt" || verb == "raytrace") {
        int w = 640, h = 480, spp = 1, depth = 4;
        std::string path = "render.ppm";
        ss >> w >> h >> spp;
        if (w < 1) w = 640;
        if (h < 1) h = 480;
        if (spp < 1) spp = 1;

        app.mesh_gen().rebuild(app.document().scene());
        MaterialLibrary mats;
        RayTracer tracer;
        tracer.build_scene(app.mesh_gen(), mats);

        RayTraceSettings settings;
        settings.width = w; settings.height = h;
        settings.samples_per_pixel = spp;
        settings.max_depth = depth;

        app.log(LogEntry::Info, "Ray tracing " + std::to_string(w) + "x" + std::to_string(h)
                + " spp=" + std::to_string(spp) + "...");
        auto pixels = tracer.render(app.camera(), settings);

        std::ofstream ppm(path, std::ios::binary);
        if (ppm.is_open()) {
            ppm << "P6\n" << w << " " << h << "\n255\n";
            ppm.write(reinterpret_cast<const char*>(pixels.data()),
                      static_cast<std::streamsize>(pixels.size()));
            app.log(LogEntry::Info, "Rendered to " + path);
        } else {
            app.log(LogEntry::Error, "Could not write " + path);
        }
    } else if (verb == "clear") {
        app.document().scene().clear();
        app.document().mark_dirty();
        app.log(LogEntry::Info, "Scene cleared");
    } else if (verb == "help" || verb == "?") {
        app.log(LogEntry::Info, "Commands:");
        app.log(LogEntry::Info, "  in <type>       — create primitive (sphere, box, cylinder, cone, torus, ellipsoid, pipe, wedge, arb8, halfspace)");
        app.log(LogEntry::Info, "  kill            — delete selected");
        app.log(LogEntry::Info, "  ls              — list all objects");
        app.log(LogEntry::Info, "  sel <id>        — select by id");
        app.log(LogEntry::Info, "  tra <x> <y> <z> — translate selected");
        app.log(LogEntry::Info, "  sca <s>         — uniform scale selected");
        app.log(LogEntry::Info, "  undo / redo     — undo/redo");
        app.log(LogEntry::Info, "  wire            — toggle wireframe");
        app.log(LogEntry::Info, "  save <path>     — save document");
        app.log(LogEntry::Info, "  load <path>     — load document");
        app.log(LogEntry::Info, "  clear           — clear scene");
    } else if (!verb.empty()) {
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
