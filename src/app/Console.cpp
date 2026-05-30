#include "app/Console.h"
#include "app/App.h"
#include "core/Analysis.h"
#include "io/MeshExport.h"
#include "io/FormatImport.h"
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
        if (path.empty()) path = "export." + fmt;
        bool ok = false;
        if (fmt == "stl")  ok = export_stl(path, app.mesh_gen());
        else if (fmt == "obj")  ok = export_obj(path, app.mesh_gen());
        else if (fmt == "ply")  ok = export_ply(path, app.mesh_gen());
        else if (fmt == "off")  ok = export_off(path, app.mesh_gen());
        else if (fmt == "dxf")  ok = export_dxf(path, app.mesh_gen());
        else if (fmt == "vrml" || fmt == "wrl") ok = export_vrml(path, app.mesh_gen());
        else if (fmt == "x3d")  ok = export_x3d(path, app.mesh_gen());
        else if (fmt == "gltf") ok = export_gltf(path, app.mesh_gen());
        else if (fmt == "iges" || fmt == "igs") ok = export_iges(path, app.mesh_gen());
        else { app.log(LogEntry::Error, "Unknown format: " + fmt); return; }
        ok ? app.log(LogEntry::Info, "Exported " + path)
            : app.log(LogEntry::Error, "Export failed");
    } else if (verb == "import" || verb == "imp") {
        std::string path;
        ss >> path;
        if (path.empty()) { app.log(LogEntry::Warning, "Usage: import <file.stl|obj|ply|off|dxf|vrml>"); return; }
        std::string fmt = detect_format(path);
        bool ok = false;
        if (fmt == "stl") ok = import_stl(path, app.document().scene());
        else if (fmt == "obj") ok = import_obj(path, app.document().scene());
        else if (fmt == "ply") ok = import_ply(path, app.document().scene());
        else if (fmt == "off") ok = import_off(path, app.document().scene());
        else if (fmt == "dxf") ok = import_dxf(path, app.document().scene());
        else if (fmt == "vrml") ok = import_vrml(path, app.document().scene());
        else { app.log(LogEntry::Error, "Unknown format: " + path); return; }
        if (ok) {
            app.document().mark_dirty();
            app.log(LogEntry::Info, "Imported " + path);
        } else {
            app.log(LogEntry::Error, "Import failed: " + path);
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
    } else if (verb == "dup" || verb == "cp" || verb == "copy") {
        auto sel = app.document().scene().selected_id();
        if (!sel) { app.log(LogEntry::Warning, "Nothing selected"); return; }
        auto* node = app.document().scene().find(*sel);
        if (!node || !node->primitive) { app.log(LogEntry::Warning, "Cannot duplicate"); return; }
        auto clone = node->primitive->clone();
        std::string name = node->name + "_copy";
        auto id = app.document().scene().add_primitive(name, std::move(clone));
        auto* newn = app.document().scene().find(id);
        if (newn) { newn->position = node->position + Vec3{0.5f, 0, 0}; newn->color = node->color; }
        app.document().scene().select(id);
        app.document().mark_dirty();
        app.log(LogEntry::Info, "Duplicated → " + name);
    } else if (verb == "rename" || verb == "mv") {
        std::string new_name;
        ss >> new_name;
        auto sel = app.document().scene().selected_id();
        if (!sel) { app.log(LogEntry::Warning, "Nothing selected"); return; }
        if (new_name.empty()) { app.log(LogEntry::Warning, "Usage: rename <name>"); return; }
        auto* n = app.document().scene().find(*sel);
        if (n) { n->name = new_name; app.document().mark_dirty(); app.log(LogEntry::Info, "Renamed to " + new_name); }
    } else if (verb == "color") {
        float r = 0.6f, g = 0.6f, b = 0.7f;
        ss >> r >> g >> b;
        auto sel = app.document().scene().selected_id();
        if (!sel) { app.log(LogEntry::Warning, "Nothing selected"); return; }
        auto* n = app.document().scene().find(*sel);
        if (n) { n->color = {r, g, b}; app.document().mark_dirty(); }
    } else if (verb == "rot" || verb == "rotate") {
        auto sel = app.document().scene().selected_id();
        if (!sel) { app.log(LogEntry::Warning, "Nothing selected"); return; }
        auto* n = app.document().scene().find(*sel);
        if (!n) return;
        float rx = 0, ry = 0, rz = 0;
        ss >> rx >> ry >> rz;
        n->rotation = Vec3{rx, ry, rz};
        app.document().mark_dirty();
    } else if (verb == "hide") {
        auto sel = app.document().scene().selected_id();
        if (!sel) return;
        auto* n = app.document().scene().find(*sel);
        if (n) { n->visible = false; app.document().mark_dirty(); app.log(LogEntry::Info, "Hidden " + n->name); }
    } else if (verb == "show" || verb == "unhide") {
        auto sel = app.document().scene().selected_id();
        if (!sel) return;
        auto* n = app.document().scene().find(*sel);
        if (n) { n->visible = true; app.document().mark_dirty(); app.log(LogEntry::Info, "Shown " + n->name); }
    } else if (verb == "showall") {
        app.document().scene().for_each([](const SceneNode&) {});
        for (auto rid : app.document().scene().root_ids()) {
            auto* n = app.document().scene().find(rid);
            if (n) n->visible = true;
        }
        app.document().mark_dirty();
        app.log(LogEntry::Info, "All objects shown");
    } else if (verb == "bb" || verb == "bounds") {
        auto sel = app.document().scene().selected_id();
        if (!sel) { app.log(LogEntry::Warning, "Nothing selected"); return; }
        auto* n = app.document().scene().find(*sel);
        if (!n || !n->primitive) return;
        auto bb = n->primitive->local_bounds();
        char buf[256];
        std::snprintf(buf, sizeof buf, "AABB: (%.3f,%.3f,%.3f) → (%.3f,%.3f,%.3f) size=(%.3f,%.3f,%.3f)",
            bb.min_pt.x, bb.min_pt.y, bb.min_pt.z, bb.max_pt.x, bb.max_pt.y, bb.max_pt.z,
            bb.max_pt.x-bb.min_pt.x, bb.max_pt.y-bb.min_pt.y, bb.max_pt.z-bb.min_pt.z);
        app.log(LogEntry::Info, buf);
    } else if (verb == "group" || verb == "g") {
        std::string name = "Group";
        ss >> name;
        auto id = app.document().scene().add_group(name);
        app.document().scene().select(id);
        app.document().mark_dirty();
        app.log(LogEntry::Info, "Created group " + name);
    } else if (verb == "union" || verb == "u") {
        app.boolean_selected(BooleanOp::Union);
    } else if (verb == "subtract" || verb == "sub" || verb == "diff") {
        app.boolean_selected(BooleanOp::Difference);
    } else if (verb == "intersect" || verb == "int") {
        app.boolean_selected(BooleanOp::Intersection);
    } else if (verb == "frame" || verb == "f") {
        AABB bb;
        app.document().scene().for_each([&](const SceneNode& n) {
            if (n.primitive) bb.merge(n.primitive->local_bounds());
        });
        app.camera().frame(bb);
    } else if (verb == "top") {
        app.camera().yaw = 0; app.camera().pitch = 89.f;
    } else if (verb == "front") {
        app.camera().yaw = 0; app.camera().pitch = 0;
    } else if (verb == "side" || verb == "right") {
        app.camera().yaw = 90; app.camera().pitch = 0;
    } else if (verb == "reset_view" || verb == "ae") {
        app.camera().yaw = 45; app.camera().pitch = 30; app.camera().distance = 8;
        app.camera().target = {0,0,0};
    } else if (verb == "zoom") {
        float z = 1.f; ss >> z;
        app.camera().distance /= z;
    } else if (verb == "shaders") {
        app.log(LogEntry::Info, "Available shaders: plastic flat cook_torrance checker noise wood camo toon cloud mirror glass emission");
    } else if (verb == "info") {
        auto sel = app.document().scene().selected_id();
        if (!sel) { app.log(LogEntry::Warning, "Nothing selected"); return; }
        auto* n = app.document().scene().find(*sel);
        if (!n) return;
        char buf[512];
        std::snprintf(buf, sizeof buf, "[%u] %s type=%s pos=(%.2f,%.2f,%.2f) rot=(%.1f,%.1f,%.1f) scale=(%.2f,%.2f,%.2f) visible=%s",
            n->id, n->name.c_str(), n->primitive ? primitive_type_name(n->primitive->type()) : "group",
            n->position.x, n->position.y, n->position.z,
            n->rotation.x, n->rotation.y, n->rotation.z,
            n->scale_vec.x, n->scale_vec.y, n->scale_vec.z,
            n->visible ? "yes" : "no");
        app.log(LogEntry::Info, buf);
    } else if (verb == "count") {
        int count = 0;
        app.document().scene().for_each([&](const SceneNode&) { ++count; });
        app.log(LogEntry::Info, "Objects: " + std::to_string(count));
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
