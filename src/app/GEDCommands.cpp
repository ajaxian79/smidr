#include "app/CommandRegistry.h"
#include "app/App.h"
#include "app/AssetLibrary.h"
#include "core/Analysis.h"
#include "core/PrimitivesAdvanced.h"
#include "core/MeshOps.h"

#include <cstdio>
#include <memory>

namespace smidr {

// Install named BRL-CAD GED command aliases and small helpers so that
// the user's muscle memory from MGED works in Smidr's console.

void install_ged_commands() {
    auto& r = CommandRegistry::instance();

    auto noop = [](App& app, std::istringstream&) {
        app.log(LogEntry::Info, "(GED stub - no-op for now)");
    };

    // ─── Database / inventory ────────────────────────────────────
    r.register_command({"3ptarb", "3ptarb", "Build ARB8 from 3 points (stub)", noop, {}});
    r.register_command({"adc", "adc", "Angle/distance cursor toggle (stub)", noop, {}});
    r.register_command({"adjust", "adjust", "Adjust attribute (stub)", noop, {}});
    r.register_command({"ae2dir", "ae2dir <az> <el>", "Convert az/el to direction",
        [](App& app, std::istringstream& ss) {
            float az, el; if (!(ss >> az >> el)) return;
            float ar = az * kDegToRad, er = el * kDegToRad;
            Vec3 d{std::cos(er) * std::sin(ar), std::sin(er), std::cos(er) * std::cos(ar)};
            char buf[128];
            std::snprintf(buf, sizeof buf, "dir: (%.4f, %.4f, %.4f)", d.x, d.y, d.z);
            app.log(LogEntry::Info, buf);
        }, {}});
    r.register_command({"dir2ae", "dir2ae <x> <y> <z>", "Direction → azimuth/elevation",
        [](App& app, std::istringstream& ss) {
            float x, y, z; if (!(ss >> x >> y >> z)) return;
            Vec3 d = Vec3{x, y, z}.normalized();
            float el = std::asin(d.y) * 180.f / kPi;
            float az = std::atan2(d.x, d.z) * 180.f / kPi;
            char buf[128];
            std::snprintf(buf, sizeof buf, "az=%.2f el=%.2f", az, el);
            app.log(LogEntry::Info, buf);
        }, {}});
    r.register_command({"annotate", "annotate", "Add annotation", noop, {}});
    r.register_command({"arced", "arced", "Edit arc (stub)", noop, {}});
    r.register_command({"arot", "arot", "Arc rotation (stub)", noop, {}});
    r.register_command({"attr", "attr <id>", "Show attributes",
        [](App& app, std::istringstream& ss) {
            unsigned int id; if (!(ss >> id)) return;
            auto* n = app.document().scene().find(id);
            if (!n) return;
            char buf[256];
            std::snprintf(buf, sizeof buf, "name=%s color=(%.2f,%.2f,%.2f) visible=%s",
                n->name.c_str(), n->color.x, n->color.y, n->color.z,
                n->visible ? "yes" : "no");
            app.log(LogEntry::Info, buf);
        }, {}});
    r.register_command({"bev", "bev", "Bevel (stub)", noop, {}});
    r.register_command({"brep", "brep", "BREP operations (stub)", noop, {}});
    r.register_command({"cat", "cat <id>", "Show object info",
        [](App& app, std::istringstream& ss) {
            unsigned int id; if (!(ss >> id)) return;
            auto* n = app.document().scene().find(id);
            if (!n) return;
            char buf[256];
            std::snprintf(buf, sizeof buf, "type=%s pos=(%.3f,%.3f,%.3f)",
                n->primitive ? primitive_type_name(n->primitive->type()) : "group",
                n->position.x, n->position.y, n->position.z);
            app.log(LogEntry::Info, buf);
        }, {}});
    r.register_command({"cc", "cc", "Cylinder cone (stub)", noop, {}});
    r.register_command({"check", "check", "Check geometry validity",
        [](App& app, std::istringstream&) {
            app.mesh_gen().rebuild(app.document().scene());
            int total = 0, watertight_count = 0;
            for (auto& e : app.mesh_gen().entries()) {
                ++total;
                if (is_watertight(e.mesh)) ++watertight_count;
            }
            char buf[128];
            std::snprintf(buf, sizeof buf, "%d/%d objects watertight", watertight_count, total);
            app.log(LogEntry::Info, buf);
        }, {}});
    r.register_command({"close", "close", "Close database (stub)", noop, {}});
    r.register_command({"coil", "coil", "Create coil (stub)", noop, {}});
    r.register_command({"comb_color", "comb_color", "Combination color (stub)", noop, {}});
    r.register_command({"combmem", "combmem", "Combination members (stub)", noop, {}});
    r.register_command({"comb_std", "comb_std", "Standard combination (stub)", noop, {}});
    r.register_command({"concat", "concat", "Concatenate databases (stub)", noop, {}});
    r.register_command({"constraint", "constraint", "Add constraint (stub)", noop, {}});
    r.register_command({"copyeval", "copyeval", "Copy evaluated (stub)", noop, {}});
    r.register_command({"copymat", "copymat", "Copy material (stub)", noop, {}});
    r.register_command({"cpi", "cpi", "Copy instance (stub)", noop, {}});
    r.register_command({"dbip", "dbip", "Database info pointer (stub)", noop, {}});
    r.register_command({"debug", "debug", "Debug toggle (stub)", noop, {}});
    r.register_command({"decompose", "decompose", "Decompose region (stub)", noop, {}});
    r.register_command({"delay", "delay <ms>", "Sleep for ms (stub)", noop, {}});
    r.register_command({"dm", "dm", "Display manager (stub)", noop, {}});
    r.register_command({"draw", "draw", "Draw object (already visible)",
        [](App& app, std::istringstream&) {
            for (auto rid : app.document().scene().root_ids()) {
                auto* n = app.document().scene().find(rid);
                if (n) n->visible = true;
            }
            app.document().mark_dirty();
        }, {}});
    r.register_command({"dump", "dump <path>", "Dump to file (uses save)",
        [](App& app, std::istringstream& ss) {
            std::string p; ss >> p;
            if (p.empty()) p = "dump.smidr";
            if (app.document().save(p)) app.log(LogEntry::Info, "Dumped to " + p);
        }, {}});
    r.register_command({"dup", "dup <new_name>", "Duplicate (alias of cp)",
        [](App& app, std::istringstream& ss) {
            std::string nn;
            std::getline(ss, nn);
            auto sel = app.document().scene().selected_id();
            if (!sel) return;
            auto* n = app.document().scene().find(*sel);
            if (!n || !n->primitive) return;
            auto id = app.document().scene().add_primitive(n->name + "_dup", n->primitive->clone());
            app.document().scene().select(id);
            app.document().mark_dirty();
        }, {}});
    r.register_command({"eac", "eac", "Edit access control (stub)", noop, {}});
    r.register_command({"edcodes", "edcodes", "Edit codes (stub)", noop, {}});
    r.register_command({"edcomb", "edcomb", "Edit combination (stub)", noop, {}});
    r.register_command({"edcolor", "edcolor", "Edit color (use color cmd)", noop, {}});
    r.register_command({"edit", "edit", "Edit object (use properties panel)", noop, {}});
    r.register_command({"editit", "editit", "Edit in text editor (stub)", noop, {}});
    r.register_command({"edmater", "edmater", "Edit material (stub)", noop, {}});
    r.register_command({"env", "env", "Environment vars (stub)", noop, {}});
    r.register_command({"erase", "erase", "Erase from view (use hide)",
        [](App& app, std::istringstream&) {
            auto sel = app.document().scene().selected_id();
            if (!sel) return;
            auto* n = app.document().scene().find(*sel);
            if (n) n->visible = false;
            app.document().mark_dirty();
        }, {}});
    r.register_command({"expand", "expand", "Expand wildcard (stub)", noop, {}});
    r.register_command({"eye_pos", "eye_pos", "Show eye position",
        [](App& app, std::istringstream&) {
            Vec3 eye = app.camera().eye();
            char buf[128];
            std::snprintf(buf, sizeof buf, "eye: (%.3f, %.3f, %.3f)", eye.x, eye.y, eye.z);
            app.log(LogEntry::Info, buf);
        }, {}});
    r.register_command({"facetize", "facetize", "Facetize selected (alias for analyze)",
        [](App& app, std::istringstream&) {
            app.mesh_gen().rebuild(app.document().scene());
            app.log(LogEntry::Info, "Facetized scene to mesh entries");
        }, {}});
    r.register_command({"fb2pix", "fb2pix", "Framebuffer to pix (use screenshot)", noop, {}});
    r.register_command({"fbclear", "fbclear", "Clear framebuffer (stub)", noop, {}});
    r.register_command({"form", "form", "Form factor (stub)", noop, {}});
    r.register_command({"fracture", "fracture", "Fracture mesh (use fracture lib)", noop, {}});
    r.register_command({"garbage_collect", "garbage_collect", "GC", noop, {"gc"}});
    r.register_command({"gdiff", "gdiff", "Geometry diff (stub)", noop, {}});
    r.register_command({"get", "get <name>", "Get attribute (stub)", noop, {}});
    r.register_command({"get_autoview", "get_autoview", "Get auto view (stub)", noop, {}});
    r.register_command({"get_comb", "get_comb", "Get combination (stub)", noop, {}});
    r.register_command({"get_eyemodel", "get_eyemodel", "Get eye + view model",
        [](App& app, std::istringstream&) {
            Vec3 e = app.camera().eye();
            char buf[256];
            std::snprintf(buf, sizeof buf, "eye=(%.2f,%.2f,%.2f) yaw=%.2f pitch=%.2f dist=%.2f",
                e.x, e.y, e.z, app.camera().yaw, app.camera().pitch, app.camera().distance);
            app.log(LogEntry::Info, buf);
        }, {}});
    r.register_command({"get_type", "get_type <id>", "Get primitive type",
        [](App& app, std::istringstream& ss) {
            unsigned int id; if (!(ss >> id)) return;
            auto* n = app.document().scene().find(id);
            if (!n) return;
            app.log(LogEntry::Info, n->primitive ? primitive_type_name(n->primitive->type()) : "group");
        }, {}});
    r.register_command({"glob", "glob <pattern>", "Glob match (uses find)",
        [](App& app, std::istringstream& ss) {
            std::string pat; ss >> pat;
            app.document().scene().for_each([&](const SceneNode& n) {
                if (n.name.find(pat) != std::string::npos) {
                    char buf[128];
                    std::snprintf(buf, sizeof buf, "  [%u] %s", n.id, n.name.c_str());
                    app.log(LogEntry::Info, buf);
                }
            });
        }, {}});
    r.register_command({"gqa", "gqa", "Geometry quality analysis (= analyze)",
        [](App& app, std::istringstream&) {
            app.mesh_gen().rebuild(app.document().scene());
            for (auto& e : app.mesh_gen().entries()) {
                auto rr = analyze_mesh(e.mesh);
                auto* node = app.document().scene().find(e.node_id);
                std::string nm = node ? node->name : "?";
                char buf[256];
                std::snprintf(buf, sizeof buf, "%s: V=%.4f A=%.4f", nm.c_str(), rr.volume, rr.surface_area);
                app.log(LogEntry::Info, buf);
            }
        }, {}});
    r.register_command({"graph", "graph", "Dependency graph (stub)", noop, {}});
    r.register_command({"grid", "grid", "Grid toggle (stub)", noop, {}});
    r.register_command({"heal", "heal", "Heal mesh (uses fill_holes)",
        [](App& app, std::istringstream&) {
            app.log(LogEntry::Info, "Use 'fill_holes' for heal operation");
        }, {}});
    r.register_command({"hide_all", "hide_all", "Hide all", [](App& app, std::istringstream&) {
        for (auto rid : app.document().scene().root_ids()) {
            auto* n = app.document().scene().find(rid);
            if (n) n->visible = false;
        }
        app.document().mark_dirty();
    }, {}});
    r.register_command({"how", "how <name>", "How is object built (stub)", noop, {}});
    r.register_command({"human", "human", "Place human reference (stub)", noop, {}});
    r.register_command({"illum", "illum", "Illuminate object (use sel)", noop, {}});
    r.register_command({"inside", "inside <id> <x> <y> <z>", "Is point inside object",
        [](App& app, std::istringstream& ss) {
            unsigned int id; float x, y, z;
            if (!(ss >> id >> x >> y >> z)) return;
            app.mesh_gen().rebuild(app.document().scene());
            for (auto& e : app.mesh_gen().entries()) {
                if (e.node_id != id) continue;
                bool in = point_inside_closed_mesh({x, y, z}, e.mesh);
                app.log(LogEntry::Info, in ? "inside" : "outside");
                return;
            }
        }, {}});
    r.register_command({"instance", "instance", "Make instance reference (stub)", noop, {}});
    r.register_command({"item", "item", "Get item from list (stub)", noop, {}});
    r.register_command({"keep", "keep", "Keep only listed (stub)", noop, {}});
    r.register_command({"keypoint", "keypoint", "Set keypoint (stub)", noop, {}});
    r.register_command({"killall", "killall", "Kill all objects", [](App& app, std::istringstream&) {
        app.document().scene().clear();
        app.document().mark_dirty();
    }, {}});
    r.register_command({"killrefs", "killrefs", "Kill referenced (stub)", noop, {}});
    r.register_command({"killtree", "killtree", "Kill tree (stub)", noop, {}});
    r.register_command({"l", "l <id>", "List node info", [](App& app, std::istringstream& ss) {
        unsigned int id; if (!(ss >> id)) return;
        auto* n = app.document().scene().find(id);
        if (!n) return;
        char buf[256];
        std::snprintf(buf, sizeof buf, "%s @(%.2f,%.2f,%.2f) [%s]",
            n->name.c_str(), n->position.x, n->position.y, n->position.z,
            n->primitive ? primitive_type_name(n->primitive->type()) : "group");
        app.log(LogEntry::Info, buf);
    }, {}});
    r.register_command({"listeval", "listeval", "List evaluated (stub)", noop, {}});
    r.register_command({"lookat", "lookat <x> <y> <z>", "Look at point",
        [](App& app, std::istringstream& ss) {
            float x, y, z; if (!(ss >> x >> y >> z)) return;
            app.camera().target = {x, y, z};
        }, {"look_at"}});
    r.register_command({"M", "M", "Memory stats (stub)", noop, {}});
    r.register_command({"make_name", "make_name <base>", "Generate unique name",
        [](App& app, std::istringstream& ss) {
            std::string base; ss >> base;
            std::string nm = base + std::to_string(app.document().scene().nodes().size());
            app.log(LogEntry::Info, nm);
        }, {}});
    r.register_command({"mater", "mater", "Material assignment (stub)", noop, {}});
    r.register_command({"memprint", "memprint", "Memory print (stub)", noop, {}});
    r.register_command({"meta", "meta", "Metadata view (stub)", noop, {}});
    r.register_command({"model2view_lu", "model2view_lu", "Model→view LU (stub)", noop, {}});
    r.register_command({"move_arb_edge", "move_arb_edge", "Move ARB edge (stub)", noop, {}});
    r.register_command({"move_arb_face", "move_arb_face", "Move ARB face (stub)", noop, {}});
    r.register_command({"nirt", "nirt", "Natalie's interactive ray tracer (stub)", noop, {}});
    r.register_command({"nmg_collapse", "nmg_collapse", "NMG collapse (stub)", noop, {}});
    r.register_command({"nmg_fix_normals", "nmg_fix_normals", "Fix normals",
        [](App& app, std::istringstream&) {
            app.log(LogEntry::Info, "Recomputed normals on next mesh rebuild");
        }, {}});
    r.register_command({"opendb", "opendb <path>", "Open database (use load)",
        [](App& app, std::istringstream& ss) {
            std::string p; ss >> p;
            if (app.document().load(p)) app.log(LogEntry::Info, "Opened " + p);
        }, {}});
    r.register_command({"paths", "paths", "Show paths", noop, {}});
    r.register_command({"perspective", "perspective <fov>", "Set FOV",
        [](App& app, std::istringstream& ss) {
            float fov = 45.f; ss >> fov; app.camera().fov = fov;
        }, {"pmodel"}});
    r.register_command({"pix2fb", "pix2fb", "Pix to framebuffer (stub)", noop, {}});
    r.register_command({"plot", "plot", "Plot vlist (stub)", noop, {}});
    r.register_command({"polybinout", "polybinout", "Polygon binary out (stub)", noop, {}});
    r.register_command({"prcolor", "prcolor", "Print color (stub)", noop, {}});
    r.register_command({"prgrf", "prgrf", "Print graph (stub)", noop, {}});
    r.register_command({"prefix", "prefix <p>", "Add prefix (stub)", noop, {}});
    r.register_command({"prj", "prj", "Project (stub)", noop, {}});
    r.register_command({"ps", "ps", "Postscript out (stub)", noop, {}});
    r.register_command({"push", "push", "Push transformations (stub)", noop, {}});
    r.register_command({"put", "put", "Put attribute (stub)", noop, {}});
    r.register_command({"q", "q", "Quit",
        [](App& app, std::istringstream&) { app.request_quit(); }, {"quit"}});
    r.register_command({"qray", "qray", "Quick ray (stub)", noop, {}});
    r.register_command({"r", "r", "Create region (stub)", noop, {}});
    r.register_command({"raydiff", "raydiff", "Ray-based geometric diff (stub)", noop, {}});
    r.register_command({"region", "region <name>", "Make region", [](App& app, std::istringstream& ss) {
        std::string nm = "Region"; ss >> nm;
        auto id = app.document().scene().add_group(nm);
        app.document().scene().select(id);
        app.document().mark_dirty();
    }, {}});
    r.register_command({"remrt", "remrt", "Remote raytrace (stub)", noop, {}});
    r.register_command({"render", "render", "Render scene (use rt)", noop, {}});
    r.register_command({"rmater", "rmater", "Read material (stub)", noop, {}});
    r.register_command({"rmats", "rmats", "Read material settings (stub)", noop, {}});
    r.register_command({"rrt", "rrt", "Remote rt (stub)", noop, {}});
    r.register_command({"rtarea", "rtarea", "Surface area (use area)",
        [](App& app, std::istringstream&) {
            app.mesh_gen().rebuild(app.document().scene());
            float total = 0.f;
            for (auto& e : app.mesh_gen().entries())
                total += compute_surface_area(e.mesh);
            char buf[64];
            std::snprintf(buf, sizeof buf, "Total area: %.4f", total);
            app.log(LogEntry::Info, buf);
        }, {}});
    r.register_command({"rtcheck", "rtcheck", "RT overlap check (stub)", noop, {}});
    r.register_command({"rtedge", "rtedge", "RT edge detection (stub)", noop, {}});
    r.register_command({"rtweight", "rtweight", "RT weight (= mass)",
        [](App& app, std::istringstream& ss) {
            float density = 1.f; ss >> density;
            app.mesh_gen().rebuild(app.document().scene());
            float total = 0.f;
            for (auto& e : app.mesh_gen().entries()) {
                auto rr = analyze_mesh(e.mesh, density);
                total += rr.mass;
            }
            char buf[64];
            std::snprintf(buf, sizeof buf, "Total mass: %.4f", total);
            app.log(LogEntry::Info, buf);
        }, {}});
    r.register_command({"set", "set <var> <val>", "Set variable (stub)", noop, {}});
    r.register_command({"shells", "shells", "List shells (stub)", noop, {}});
    r.register_command({"showmats", "showmats", "Show material matrices (stub)", noop, {}});
    r.register_command({"size", "size", "View size (= distance)",
        [](App& app, std::istringstream&) {
            char buf[64];
            std::snprintf(buf, sizeof buf, "View distance: %.3f", app.camera().distance);
            app.log(LogEntry::Info, buf);
        }, {"isize"}});
    r.register_command({"solid_report", "solid_report", "Per-solid report",
        [](App& app, std::istringstream&) {
            app.mesh_gen().rebuild(app.document().scene());
            for (auto& e : app.mesh_gen().entries()) {
                auto rr = analyze_mesh(e.mesh);
                auto* node = app.document().scene().find(e.node_id);
                if (!node) continue;
                char buf[256];
                std::snprintf(buf, sizeof buf, "%s: V=%.4f A=%.4f T=%d",
                    node->name.c_str(), rr.volume, rr.surface_area, rr.triangle_count);
                app.log(LogEntry::Info, buf);
            }
        }, {}});
    r.register_command({"summary", "summary", "Database summary",
        [](App& app, std::istringstream&) {
            int total = 0;
            app.document().scene().for_each([&](const SceneNode&) { ++total; });
            char buf[128];
            std::snprintf(buf, sizeof buf, "Objects: %d / Roots: %zu",
                          total, app.document().scene().root_ids().size());
            app.log(LogEntry::Info, buf);
        }, {}});
    r.register_command({"suffix", "suffix <s>", "Add suffix (stub)", noop, {}});
    r.register_command({"sync", "sync", "Sync to disk (stub)", noop, {}});
    r.register_command({"tabobj", "tabobj", "Tabulate object (stub)", noop, {}});
    r.register_command({"tie", "tie", "Tie display managers (stub)", noop, {}});
    r.register_command({"title", "title <text>", "Set database title (stub)", noop, {}});
    r.register_command({"tol", "tol", "Tolerance settings (stub)", noop, {}});
    r.register_command({"tops", "tops", "List top-level objects",
        [](App& app, std::istringstream&) {
            for (auto rid : app.document().scene().root_ids()) {
                auto* n = app.document().scene().find(rid);
                if (n) app.log(LogEntry::Info, "  " + n->name);
            }
        }, {}});
    r.register_command({"track", "track", "Build track (stub)", noop, {}});
    r.register_command({"vars", "vars", "List variables (stub)", noop, {}});
    r.register_command({"vdraw", "vdraw", "Virtual draw (stub)", noop, {}});
    r.register_command({"version", "version", "Version info",
        [](App& app, std::istringstream&) {
            app.log(LogEntry::Info, "Smidr 0.x — vertically integrated CAD");
        }, {}});
    r.register_command({"view", "view", "View parameters (= info)",
        [](App& app, std::istringstream&) {
            char buf[256];
            std::snprintf(buf, sizeof buf, "yaw=%.2f pitch=%.2f fov=%.2f dist=%.2f",
                app.camera().yaw, app.camera().pitch, app.camera().fov, app.camera().distance);
            app.log(LogEntry::Info, buf);
        }, {}});
    r.register_command({"voxelize", "voxelize <resolution>", "Voxelize selected (stub)", noop, {}});
    r.register_command({"weight", "weight", "Compute mass weight (= rtweight)",
        [](App& app, std::istringstream& ss) {
            float d = 1.f; ss >> d;
            app.mesh_gen().rebuild(app.document().scene());
            float total = 0.f;
            for (auto& e : app.mesh_gen().entries()) total += analyze_mesh(e.mesh, d).mass;
            char buf[64];
            std::snprintf(buf, sizeof buf, "Total weight: %.4f", total);
            app.log(LogEntry::Info, buf);
        }, {}});
    r.register_command({"whichair", "whichair", "Which air regions (stub)", noop, {}});
    r.register_command({"whichid", "whichid", "Which region id (stub)", noop, {}});
    r.register_command({"x", "x", "Wireframe toggle (= wire)",
        [](App& app, std::istringstream&) { app.renderer().wireframe = !app.renderer().wireframe; }, {}});
    r.register_command({"xpush", "xpush", "Push transforms recursively (stub)", noop, {}});
    r.register_command({"Z", "Z", "Zap (clear)",
        [](App& app, std::istringstream&) {
            for (auto rid : app.document().scene().root_ids()) {
                auto* n = app.document().scene().find(rid);
                if (n) n->visible = false;
            }
            app.document().mark_dirty();
        }, {"zap"}});
}

}  // namespace smidr
