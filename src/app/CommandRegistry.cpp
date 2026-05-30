#include "app/CommandRegistry.h"
#include "app/App.h"
#include "app/Gizmo.h"
#include "app/LightsAndAnim.h"
#include "app/ScriptEngine.h"
#include "app/AssetLibrary.h"
#include "core/Analysis.h"
#include "core/PrimitivesAdvanced.h"
#include "core/PrimitivesExtra.h"
#include "core/MeshOps.h"
#include "core/Slicer.h"
#include "core/GCode.h"
#include "io/MeshExport.h"
#include "io/FormatImport.h"
#include "io/Screenshot.h"
#include "render/RayTracer.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <memory>

namespace smidr {

CommandRegistry& CommandRegistry::instance() {
    static CommandRegistry registry;
    return registry;
}

void CommandRegistry::register_command(CommandInfo info) {
    auto cmd = std::make_shared<CommandInfo>(std::move(info));
    by_name_[cmd->name] = cmd;
    for (auto& alias : cmd->aliases) by_name_[alias] = cmd;
    all_.push_back(cmd);
}

bool CommandRegistry::execute(App& app, const std::string& input) {
    std::istringstream ss(input);
    std::string verb;
    ss >> verb;
    auto it = by_name_.find(verb);
    if (it == by_name_.end()) return false;
    it->second->handler(app, ss);
    return true;
}

std::vector<const CommandInfo*> CommandRegistry::list() const {
    std::vector<const CommandInfo*> out;
    for (auto& c : all_) out.push_back(c.get());
    std::sort(out.begin(), out.end(),
              [](const CommandInfo* a, const CommandInfo* b) { return a->name < b->name; });
    return out;
}

const CommandInfo* CommandRegistry::find(const std::string& name) const {
    auto it = by_name_.find(name);
    return it != by_name_.end() ? it->second.get() : nullptr;
}

static std::unique_ptr<Primitive> make_primitive(const std::string& name) {
    if (name == "sphere")         return std::make_unique<Sphere>();
    if (name == "box")            return std::make_unique<Box>();
    if (name == "cylinder")       return std::make_unique<Cylinder>();
    if (name == "cone")           return std::make_unique<Cone>();
    if (name == "torus")          return std::make_unique<Torus>();
    if (name == "ellipsoid")      return std::make_unique<Ellipsoid>();
    if (name == "halfspace")      return std::make_unique<Halfspace>();
    if (name == "pipe")           return std::make_unique<Pipe>();
    if (name == "wedge")          return std::make_unique<Wedge>();
    if (name == "arb8")           return std::make_unique<Arb8>();
    if (name == "superellipsoid") return std::make_unique<Superellipsoid>();
    if (name == "particle")       return std::make_unique<Particle>();
    if (name == "arbn")           return std::make_unique<Arbn>();
    if (name == "rpc")            return std::make_unique<RPC>();
    if (name == "rhc")            return std::make_unique<RHC>();
    if (name == "epa")            return std::make_unique<EPA>();
    if (name == "ehy")            return std::make_unique<EHY>();
    if (name == "eto")            return std::make_unique<ETO>();
    if (name == "hyperboloid")    return std::make_unique<Hyperboloid>();
    if (name == "bot")            return std::make_unique<Bot>();
    if (name == "sketch")         return std::make_unique<SketchPrimitive>();
    if (name == "extrude")        return std::make_unique<ExtrudePrimitive>();
    if (name == "revolve")        return std::make_unique<RevolvePrimitive>();
    if (name == "dsp")            return std::make_unique<DSPPrimitive>();
    if (name == "metaball")       return std::make_unique<MetaballPrimitive>();
    if (name == "heart")          return std::make_unique<HeartPrimitive>();
    if (name == "pointcloud")     return std::make_unique<PointCloudPrimitive>();
    if (name == "annotation")     return std::make_unique<AnnotationPrimitive>();
    if (name == "cline")          return std::make_unique<CLinePrimitive>();
    if (name == "joint")          return std::make_unique<JointPrimitive>();
    if (name == "grip")           return std::make_unique<GripPrimitive>();
    if (name == "datum")          return std::make_unique<DatumPrimitive>();
    if (name == "submodel")       return std::make_unique<SubmodelPrimitive>();
    if (name == "script")         return std::make_unique<ScriptPrimitive>();
    if (name == "ebm")            return std::make_unique<EBMPrimitive>();
    if (name == "vol")            return std::make_unique<VOLPrimitive>();
    if (name == "hf")             return std::make_unique<HFPrimitive>();
    if (name == "ars")            return std::make_unique<ARSPrimitive>();
    return nullptr;
}

#define CMD(name_, usage_, help_, body) \
    register_command({name_, usage_, help_, [](App& app, std::istringstream& ss) body, {}})

void CommandRegistry::install_default_commands() {
    auto& r = instance();

    r.register_command({"in", "in <type>", "Create primitive",
        [](App& app, std::istringstream& ss) {
            std::string type;
            ss >> type;
            if (type.empty()) { app.log(LogEntry::Warning, "Usage: in <type>"); return; }
            auto p = make_primitive(type);
            if (!p) { app.log(LogEntry::Error, "Unknown type: " + type); return; }
            try {
                app.add_primitive(p->type());
            } catch (...) { app.log(LogEntry::Error, "Failed to create"); }
        }, {"create", "make"}});

    r.register_command({"kill", "kill", "Delete selected", [](App& app, std::istringstream&) {
        app.delete_selected();
    }, {"delete", "rm", "d"}});

    r.register_command({"undo", "undo", "Undo last action",
        [](App& app, std::istringstream&) { app.undo(); }, {"u"}});

    r.register_command({"redo", "redo", "Redo last undone action",
        [](App& app, std::istringstream&) { app.redo(); }, {}});

    r.register_command({"ls", "ls", "List all objects", [](App& app, std::istringstream&) {
        app.document().scene().for_each([&](const SceneNode& n) {
            char line[128];
            std::snprintf(line, sizeof line, "  [%u] %s (%s)",
                          n.id, n.name.c_str(),
                          n.primitive ? primitive_type_name(n.primitive->type()) : "group");
            app.log(LogEntry::Info, line);
        });
    }, {"list", "dir"}});

    r.register_command({"sel", "sel <id>", "Select object by id",
        [](App& app, std::istringstream& ss) {
            unsigned int id = 0;
            if (ss >> id) {
                auto* n = app.document().scene().find(static_cast<NodeId>(id));
                if (n) { app.document().scene().select(static_cast<NodeId>(id));
                         app.log(LogEntry::Info, "Selected " + n->name); }
                else app.log(LogEntry::Error, "No node " + std::to_string(id));
            }
        }, {"select"}});

    r.register_command({"tra", "tra <x> <y> <z>", "Translate selected",
        [](App& app, std::istringstream& ss) {
            auto sel = app.document().scene().selected_id();
            if (!sel) return;
            auto* n = app.document().scene().find(*sel);
            if (!n) return;
            float x=0,y=0,z=0; ss >> x >> y >> z;
            n->position = {x,y,z};
            app.document().mark_dirty();
        }, {"translate", "t"}});

    r.register_command({"rot", "rot <x> <y> <z>", "Rotate selected (degrees)",
        [](App& app, std::istringstream& ss) {
            auto sel = app.document().scene().selected_id();
            if (!sel) return;
            auto* n = app.document().scene().find(*sel);
            if (!n) return;
            float x=0,y=0,z=0; ss >> x >> y >> z;
            n->rotation = {x,y,z};
            app.document().mark_dirty();
        }, {"rotate"}});

    r.register_command({"sca", "sca <s>", "Uniform scale selected",
        [](App& app, std::istringstream& ss) {
            auto sel = app.document().scene().selected_id();
            if (!sel) return;
            auto* n = app.document().scene().find(*sel);
            if (!n) return;
            float s = 1.f; ss >> s;
            n->scale_vec = {s,s,s};
            app.document().mark_dirty();
        }, {"scale"}});

    r.register_command({"sca_xyz", "sca_xyz <x> <y> <z>", "Non-uniform scale",
        [](App& app, std::istringstream& ss) {
            auto sel = app.document().scene().selected_id();
            if (!sel) return;
            auto* n = app.document().scene().find(*sel);
            if (!n) return;
            float x=1,y=1,z=1; ss >> x >> y >> z;
            n->scale_vec = {x,y,z};
            app.document().mark_dirty();
        }, {}});

    r.register_command({"color", "color <r> <g> <b>", "Set color (0-1)",
        [](App& app, std::istringstream& ss) {
            auto sel = app.document().scene().selected_id();
            if (!sel) return;
            auto* n = app.document().scene().find(*sel);
            if (!n) return;
            float r=0.6f,g=0.6f,b=0.7f; ss >> r >> g >> b;
            n->color = {r,g,b};
            app.document().mark_dirty();
        }, {"col"}});

    r.register_command({"opacity", "opacity <0..1>", "Set opacity",
        [](App& app, std::istringstream& ss) {
            auto sel = app.document().scene().selected_id();
            if (!sel) return;
            auto* n = app.document().scene().find(*sel);
            if (!n) return;
            float o = 1.f; ss >> o;
            n->opacity = o;
            app.document().mark_dirty();
        }, {"alpha"}});

    r.register_command({"hide", "hide", "Hide selected",
        [](App& app, std::istringstream&) {
            auto sel = app.document().scene().selected_id();
            if (!sel) return;
            auto* n = app.document().scene().find(*sel);
            if (n) { n->visible = false; app.document().mark_dirty(); }
        }, {"h"}});

    r.register_command({"show", "show", "Show selected",
        [](App& app, std::istringstream&) {
            auto sel = app.document().scene().selected_id();
            if (!sel) return;
            auto* n = app.document().scene().find(*sel);
            if (n) { n->visible = true; app.document().mark_dirty(); }
        }, {"unhide", "s"}});

    r.register_command({"showall", "showall", "Show all objects",
        [](App& app, std::istringstream&) {
            for (auto rid : app.document().scene().root_ids()) {
                auto* n = app.document().scene().find(rid);
                if (n) n->visible = true;
            }
            app.document().mark_dirty();
        }, {"unhideall"}});

    r.register_command({"hideall", "hideall", "Hide all objects",
        [](App& app, std::istringstream&) {
            for (auto rid : app.document().scene().root_ids()) {
                auto* n = app.document().scene().find(rid);
                if (n) n->visible = false;
            }
            app.document().mark_dirty();
        }, {"blast"}});

    r.register_command({"dup", "dup", "Duplicate selected",
        [](App& app, std::istringstream&) {
            auto sel = app.document().scene().selected_id();
            if (!sel) return;
            auto* node = app.document().scene().find(*sel);
            if (!node || !node->primitive) return;
            auto clone = node->primitive->clone();
            std::string name = node->name + "_copy";
            auto id = app.document().scene().add_primitive(name, std::move(clone));
            auto* newn = app.document().scene().find(id);
            if (newn) { newn->position = node->position + Vec3{0.5f,0,0}; newn->color = node->color; }
            app.document().scene().select(id);
            app.document().mark_dirty();
        }, {"cp", "copy", "clone"}});

    r.register_command({"mirror", "mirror <x|y|z>", "Mirror selected on axis",
        [](App& app, std::istringstream& ss) {
            auto sel = app.document().scene().selected_id();
            if (!sel) return;
            auto* node = app.document().scene().find(*sel);
            if (!node || !node->primitive) return;
            std::string axis = "x"; ss >> axis;
            auto clone = node->primitive->clone();
            auto id = app.document().scene().add_primitive(node->name + "_mirror", std::move(clone));
            auto* newn = app.document().scene().find(id);
            if (newn) {
                newn->position = node->position;
                newn->color = node->color;
                newn->scale_vec = node->scale_vec;
                if (axis == "x") newn->scale_vec.x = -newn->scale_vec.x;
                else if (axis == "y") newn->scale_vec.y = -newn->scale_vec.y;
                else newn->scale_vec.z = -newn->scale_vec.z;
            }
            app.document().scene().select(id);
            app.document().mark_dirty();
        }, {}});

    r.register_command({"array", "array <n> <dx> <dy> <dz>", "Linear array of duplicates",
        [](App& app, std::istringstream& ss) {
            auto sel = app.document().scene().selected_id();
            if (!sel) return;
            auto* node = app.document().scene().find(*sel);
            if (!node || !node->primitive) return;
            int n = 3; float dx = 1, dy = 0, dz = 0;
            ss >> n >> dx >> dy >> dz;
            for (int i = 1; i < n; ++i) {
                auto clone = node->primitive->clone();
                char nm[64];
                std::snprintf(nm, sizeof nm, "%s_arr%d", node->name.c_str(), i);
                auto id = app.document().scene().add_primitive(nm, std::move(clone));
                auto* nn = app.document().scene().find(id);
                if (nn) { nn->position = node->position + Vec3{dx,dy,dz} * static_cast<float>(i);
                          nn->color = node->color; }
            }
            app.document().mark_dirty();
        }, {}});

    r.register_command({"rename", "rename <name>", "Rename selected",
        [](App& app, std::istringstream& ss) {
            std::string nn; ss >> nn;
            if (nn.empty()) return;
            auto sel = app.document().scene().selected_id();
            if (!sel) return;
            auto* n = app.document().scene().find(*sel);
            if (n) { n->name = nn; app.document().mark_dirty(); }
        }, {"mv", "ren"}});

    r.register_command({"group", "group <name>", "Create group",
        [](App& app, std::istringstream& ss) {
            std::string nm = "Group"; ss >> nm;
            auto id = app.document().scene().add_group(nm);
            app.document().scene().select(id);
            app.document().mark_dirty();
        }, {"g", "comb"}});

    r.register_command({"union", "union", "Union two selected",
        [](App& app, std::istringstream&) { app.boolean_selected(BooleanOp::Union); }, {"u", "+"}});
    r.register_command({"sub", "sub", "Subtract",
        [](App& app, std::istringstream&) { app.boolean_selected(BooleanOp::Difference); }, {"subtract", "diff", "-"}});
    r.register_command({"int", "int", "Intersect",
        [](App& app, std::istringstream&) { app.boolean_selected(BooleanOp::Intersection); }, {"intersect"}});

    r.register_command({"bb", "bb", "Show bounding box",
        [](App& app, std::istringstream&) {
            auto sel = app.document().scene().selected_id();
            if (!sel) return;
            auto* n = app.document().scene().find(*sel);
            if (!n || !n->primitive) return;
            auto bb = n->primitive->local_bounds();
            char buf[256];
            std::snprintf(buf, sizeof buf, "AABB: (%.3f,%.3f,%.3f)→(%.3f,%.3f,%.3f) size=(%.3f,%.3f,%.3f)",
                bb.min_pt.x,bb.min_pt.y,bb.min_pt.z,bb.max_pt.x,bb.max_pt.y,bb.max_pt.z,
                bb.max_pt.x-bb.min_pt.x,bb.max_pt.y-bb.min_pt.y,bb.max_pt.z-bb.min_pt.z);
            app.log(LogEntry::Info, buf);
        }, {"bounds"}});

    r.register_command({"analyze", "analyze", "Analyze geometry",
        [](App& app, std::istringstream&) {
            app.mesh_gen().rebuild(app.document().scene());
            for (auto& e : app.mesh_gen().entries()) {
                auto rr = analyze_mesh(e.mesh);
                auto* node = app.document().scene().find(e.node_id);
                std::string nm = node ? node->name : "?";
                char buf[512];
                std::snprintf(buf, sizeof buf, "%s: V=%.4f A=%.4f tris=%d cm=(%.2f,%.2f,%.2f)",
                    nm.c_str(), rr.volume, rr.surface_area, rr.triangle_count,
                    rr.centroid.x, rr.centroid.y, rr.centroid.z);
                app.log(LogEntry::Info, buf);
            }
        }, {"ana", "gqa"}});

    r.register_command({"frame", "frame", "Frame all in view",
        [](App& app, std::istringstream&) {
            AABB bb;
            app.document().scene().for_each([&](const SceneNode& n) {
                if (n.primitive) bb.merge(n.primitive->local_bounds());
            });
            app.camera().frame(bb);
        }, {"f", "fit", "view_all"}});

    r.register_command({"top", "top", "Top view",
        [](App& app, std::istringstream&) { app.camera().yaw=0; app.camera().pitch=89.f; }, {"v_top"}});
    r.register_command({"front", "front", "Front view",
        [](App& app, std::istringstream&) { app.camera().yaw=0; app.camera().pitch=0; }, {"v_front"}});
    r.register_command({"side", "side", "Right side view",
        [](App& app, std::istringstream&) { app.camera().yaw=90; app.camera().pitch=0; }, {"right", "v_right"}});
    r.register_command({"left", "left", "Left side view",
        [](App& app, std::istringstream&) { app.camera().yaw=-90; app.camera().pitch=0; }, {"v_left"}});
    r.register_command({"back", "back", "Back view",
        [](App& app, std::istringstream&) { app.camera().yaw=180; app.camera().pitch=0; }, {"v_back"}});
    r.register_command({"bottom", "bottom", "Bottom view",
        [](App& app, std::istringstream&) { app.camera().yaw=0; app.camera().pitch=-89.f; }, {"v_bot"}});
    r.register_command({"iso", "iso", "Isometric view",
        [](App& app, std::istringstream&) { app.camera().yaw=45; app.camera().pitch=35.f; app.camera().distance=8; }, {"reset_view"}});

    r.register_command({"ae", "ae <azim> <elev>", "Set azimuth/elevation",
        [](App& app, std::istringstream& ss) {
            float a = 0, e = 0; ss >> a >> e;
            app.camera().yaw = a; app.camera().pitch = e;
        }, {}});

    r.register_command({"zoom", "zoom <factor>", "Zoom in/out",
        [](App& app, std::istringstream& ss) {
            float z = 2.f; ss >> z;
            app.camera().distance /= z;
        }, {"z"}});

    r.register_command({"wire", "wire", "Toggle wireframe",
        [](App& app, std::istringstream&) { app.renderer().wireframe = !app.renderer().wireframe; }, {"wireframe"}});

    r.register_command({"save", "save <path>", "Save document",
        [](App& app, std::istringstream& ss) {
            std::string p; ss >> p;
            if (p.empty()) p = "untitled.smidr";
            app.document().save(p) ? app.log(LogEntry::Info, "Saved " + p)
                                    : app.log(LogEntry::Error, "Save failed");
        }, {}});

    r.register_command({"load", "load <path>", "Load document",
        [](App& app, std::istringstream& ss) {
            std::string p; ss >> p;
            if (p.empty()) return;
            app.document().load(p) ? app.log(LogEntry::Info, "Loaded " + p)
                                    : app.log(LogEntry::Error, "Load failed");
        }, {}});

    r.register_command({"import", "import <file>", "Import mesh",
        [](App& app, std::istringstream& ss) {
            std::string p; ss >> p;
            if (p.empty()) return;
            std::string fmt = detect_format(p);
            bool ok = false;
            if (fmt=="stl") ok=import_stl(p,app.document().scene());
            else if (fmt=="obj") ok=import_obj(p,app.document().scene());
            else if (fmt=="ply") ok=import_ply(p,app.document().scene());
            else if (fmt=="off") ok=import_off(p,app.document().scene());
            else if (fmt=="dxf") ok=import_dxf(p,app.document().scene());
            else if (fmt=="vrml") ok=import_vrml(p,app.document().scene());
            else if (fmt=="step") ok=import_step(p,app.document().scene());
            else if (fmt=="iges") ok=import_iges(p,app.document().scene());
            ok ? (app.document().mark_dirty(), app.log(LogEntry::Info, "Imported " + p))
                : app.log(LogEntry::Error, "Import failed: " + p);
        }, {"imp"}});

    r.register_command({"export", "export <fmt> [path]", "Export mesh",
        [](App& app, std::istringstream& ss) {
            std::string fmt, p; ss >> fmt >> p;
            if (fmt.empty()) return;
            if (p.empty()) p = "export." + fmt;
            app.mesh_gen().rebuild(app.document().scene());
            bool ok = false;
            if (fmt=="stl") ok=export_stl(p,app.mesh_gen());
            else if (fmt=="obj") ok=export_obj(p,app.mesh_gen());
            else if (fmt=="ply") ok=export_ply(p,app.mesh_gen());
            else if (fmt=="off") ok=export_off(p,app.mesh_gen());
            else if (fmt=="dxf") ok=export_dxf(p,app.mesh_gen());
            else if (fmt=="vrml"||fmt=="wrl") ok=export_vrml(p,app.mesh_gen());
            else if (fmt=="x3d") ok=export_x3d(p,app.mesh_gen());
            else if (fmt=="gltf") ok=export_gltf(p,app.mesh_gen());
            else if (fmt=="iges"||fmt=="igs") ok=export_iges(p,app.mesh_gen());
            ok ? app.log(LogEntry::Info, "Exported " + p) : app.log(LogEntry::Error, "Export failed");
        }, {"exp"}});

    r.register_command({"rt", "rt [w] [h] [spp]", "Ray trace to render.ppm",
        [](App& app, std::istringstream& ss) {
            int w=640, h=480, spp=1;
            ss >> w >> h >> spp;
            app.mesh_gen().rebuild(app.document().scene());
            MaterialLibrary mats;
            RayTracer tr;
            tr.build_scene(app.mesh_gen(), mats);
            RayTraceSettings st;
            st.width = w; st.height = h; st.samples_per_pixel = spp;
            auto px = tr.render(app.camera(), st);
            std::ofstream out("render.ppm", std::ios::binary);
            if (out.is_open()) {
                out << "P6\n" << w << " " << h << "\n255\n";
                out.write(reinterpret_cast<const char*>(px.data()),
                          static_cast<std::streamsize>(px.size()));
                app.log(LogEntry::Info, "Rendered render.ppm");
            }
        }, {"raytrace", "render"}});

    r.register_command({"clear", "clear", "Clear scene",
        [](App& app, std::istringstream&) {
            app.document().scene().clear();
            app.document().mark_dirty();
        }, {"new", "scrub"}});

    r.register_command({"count", "count", "Count objects",
        [](App& app, std::istringstream&) {
            int c = 0; app.document().scene().for_each([&](const SceneNode&) { ++c; });
            app.log(LogEntry::Info, "Objects: " + std::to_string(c));
        }, {"summary"}});

    r.register_command({"info", "info", "Selected object info",
        [](App& app, std::istringstream&) {
            auto sel = app.document().scene().selected_id();
            if (!sel) return;
            auto* n = app.document().scene().find(*sel);
            if (!n) return;
            char buf[512];
            std::snprintf(buf, sizeof buf, "[%u] %s type=%s pos=(%.2f,%.2f,%.2f) vis=%s",
                n->id, n->name.c_str(),
                n->primitive ? primitive_type_name(n->primitive->type()) : "group",
                n->position.x, n->position.y, n->position.z,
                n->visible ? "yes" : "no");
            app.log(LogEntry::Info, buf);
        }, {"i", "attr"}});

    r.register_command({"types", "types", "List primitive types",
        [](App& app, std::istringstream&) {
            app.log(LogEntry::Info, "Types: sphere box cylinder cone torus ellipsoid halfspace "
                                     "pipe wedge arb8 superellipsoid particle arbn rpc rhc epa ehy eto "
                                     "hyperboloid bot sketch extrude revolve dsp metaball heart pointcloud annotation");
        }, {}});

    r.register_command({"shaders", "shaders", "List shaders",
        [](App& app, std::istringstream&) {
            app.log(LogEntry::Info, "Shaders: plastic flat cook_torrance checker noise wood camo toon cloud mirror glass emission");
        }, {}});

    r.register_command({"who", "who", "List visible objects",
        [](App& app, std::istringstream&) {
            app.document().scene().for_each([&](const SceneNode& n) {
                if (n.visible) app.log(LogEntry::Info, "  " + n.name);
            });
        }, {"visible"}});

    r.register_command({"find", "find <pattern>", "Find by name substring",
        [](App& app, std::istringstream& ss) {
            std::string pat; ss >> pat;
            if (pat.empty()) return;
            app.document().scene().for_each([&](const SceneNode& n) {
                if (n.name.find(pat) != std::string::npos) {
                    char buf[128];
                    std::snprintf(buf, sizeof buf, "  [%u] %s", n.id, n.name.c_str());
                    app.log(LogEntry::Info, buf);
                }
            });
        }, {"search"}});

    r.register_command({"exists", "exists <name>", "Check object exists",
        [](App& app, std::istringstream& ss) {
            std::string nm; ss >> nm;
            bool found = false;
            app.document().scene().for_each([&](const SceneNode& n) {
                if (n.name == nm) found = true;
            });
            app.log(LogEntry::Info, found ? "yes" : "no");
        }, {}});

    r.register_command({"units", "units [mm|cm|m|in|ft]", "Set/show units",
        [](App& app, std::istringstream& ss) {
            std::string u; ss >> u;
            if (u.empty()) app.log(LogEntry::Info, "Units: mm (default)");
            else app.log(LogEntry::Info, "Units set to " + u);
        }, {}});

    r.register_command({"history", "history", "Show undo stack",
        [](App& app, std::istringstream&) {
            char buf[128];
            std::snprintf(buf, sizeof buf, "undo:%d redo:%d",
                          app.history().undo_count(), app.history().redo_count());
            app.log(LogEntry::Info, buf);
        }, {"hist"}});

    r.register_command({"echo", "echo <text>", "Print to console",
        [](App& app, std::istringstream& ss) {
            std::string rest;
            std::getline(ss, rest);
            app.log(LogEntry::Info, rest);
        }, {}});

    r.register_command({"reset", "reset", "Reset camera and view",
        [](App& app, std::istringstream&) {
            app.camera().yaw=45; app.camera().pitch=30; app.camera().distance=8;
            app.camera().target = {0,0,0};
        }, {"reset_view"}});

    r.register_command({"light", "light add|list|clear",
        "Manage scene lights",
        [](App& app, std::istringstream& ss) {
            std::string sub; ss >> sub;
            auto& mgr = app_lights();
            if (sub == "add") {
                SceneLight l;
                l.name = "Light." + std::to_string(mgr.lights().size());
                ss >> l.position.x >> l.position.y >> l.position.z;
                mgr.add(l);
                app.log(LogEntry::Info, "Added " + l.name);
            } else if (sub == "clear") {
                mgr.clear();
                app.log(LogEntry::Info, "Lights cleared");
            } else {
                for (auto& l : mgr.lights()) {
                    char buf[256];
                    std::snprintf(buf, sizeof buf, "  %s @(%.1f,%.1f,%.1f) I=%.2f %s",
                                  l.name.c_str(), l.position.x, l.position.y, l.position.z,
                                  l.intensity, l.enabled ? "ON" : "OFF");
                    app.log(LogEntry::Info, buf);
                }
            }
        }, {}});

    r.register_command({"key", "key add|list|clear",
        "Animation keyframes",
        [](App& app, std::istringstream& ss) {
            std::string sub; ss >> sub;
            auto& tl = app_timeline();
            if (sub == "add") {
                auto sel = app.document().scene().selected_id();
                if (!sel) return;
                auto* n = app.document().scene().find(*sel);
                if (n) {
                    tl.add_keyframe(n->id, tl.current_time, n->position, n->rotation, n->scale_vec);
                    app.log(LogEntry::Info, "Keyframe at t=" + std::to_string(tl.current_time));
                }
            } else if (sub == "clear") {
                tl.tracks.clear();
            } else {
                char buf[128];
                std::snprintf(buf, sizeof buf, "%zu tracks, t=%.2f/%.2f, fps=%.1f, %s",
                              tl.tracks.size(), tl.current_time, tl.duration, tl.fps,
                              tl.playing ? "PLAYING" : "STOPPED");
                app.log(LogEntry::Info, buf);
            }
        }, {"keyframe"}});

    r.register_command({"select_all", "select_all", "Select all visible",
        [](App& app, std::istringstream&) {
            app.document().scene().for_each([&](const SceneNode& n) {
                if (n.visible)
                    const_cast<SceneNode&>(n).selected = true;
            });
            app.document().mark_dirty();
        }, {"sel_all"}});

    r.register_command({"select_none", "select_none", "Deselect all",
        [](App& app, std::istringstream&) {
            app.document().scene().clear_selection();
            app.document().mark_dirty();
        }, {"deselect"}});

    r.register_command({"invert_sel", "invert_sel", "Invert selection",
        [](App& app, std::istringstream&) {
            app.document().scene().for_each([&](const SceneNode& n) {
                const_cast<SceneNode&>(n).selected = !n.selected;
            });
            app.document().mark_dirty();
        }, {}});

    r.register_command({"sel_by_type", "sel_by_type <type>", "Select by primitive type",
        [](App& app, std::istringstream& ss) {
            std::string type; ss >> type;
            app.document().scene().clear_selection();
            app.document().scene().for_each([&](const SceneNode& n) {
                if (n.primitive && primitive_type_name(n.primitive->type()) == type)
                    const_cast<SceneNode&>(n).selected = true;
            });
            app.document().mark_dirty();
        }, {}});

    r.register_command({"isolate", "isolate", "Hide everything except selection",
        [](App& app, std::istringstream&) {
            app.document().scene().for_each([&](const SceneNode& n) {
                const_cast<SceneNode&>(n).visible = n.selected;
            });
            app.document().mark_dirty();
        }, {}});

    r.register_command({"sel_count", "sel_count", "Number selected",
        [](App& app, std::istringstream&) {
            app.log(LogEntry::Info, std::to_string(app.document().scene().selection_count()) + " selected");
        }, {}});

    r.register_command({"screenshot", "screenshot [path] [w] [h]",
        "Render and save framebuffer to PNG",
        [](App& app, std::istringstream& ss) {
            std::string p = "screenshot.png";
            int w = app.renderer().width(), h = app.renderer().height();
            ss >> p >> w >> h;
            if (capture_framebuffer(p, w, h)) {
                app.log(LogEntry::Info, "Saved " + p);
            } else {
                app.log(LogEntry::Error, "Screenshot failed");
            }
        }, {"capture", "snap"}});

    r.register_command({"slice", "slice <step> [out.dxf]", "Slice mesh to DXF",
        [](App& app, std::istringstream& ss) {
            float step = 0.5f;
            std::string out = "slices.dxf";
            ss >> step >> out;
            app.mesh_gen().rebuild(app.document().scene());
            Mesh combined;
            for (auto& e : app.mesh_gen().entries()) {
                uint32_t off = static_cast<uint32_t>(combined.vertices.size());
                for (auto& v : e.mesh.vertices) {
                    Vertex tv;
                    tv.position = e.transform.transform_point(v.position);
                    tv.normal = e.transform.transform_normal(v.normal);
                    combined.vertices.push_back(tv);
                }
                for (auto idx : e.mesh.indices) combined.indices.push_back(idx + off);
            }
            AABB bb = combined.bounds();
            auto stack = slice_z_stack(combined, bb.min_pt.z, bb.max_pt.z, step);
            if (export_slices_dxf(out, stack, bb.min_pt.z, step)) {
                app.log(LogEntry::Info, "Sliced " + std::to_string(stack.size())
                        + " layers into " + out);
            } else { app.log(LogEntry::Error, "Slice export failed"); }
        }, {}});

    r.register_command({"gcode", "gcode [out.gcode] [layer_h]", "Generate G-code",
        [](App& app, std::istringstream& ss) {
            std::string out = "out.gcode";
            float lh = 0.2f;
            ss >> out >> lh;
            app.mesh_gen().rebuild(app.document().scene());
            Mesh combined;
            for (auto& e : app.mesh_gen().entries()) {
                uint32_t off = static_cast<uint32_t>(combined.vertices.size());
                for (auto& v : e.mesh.vertices) {
                    Vertex tv;
                    tv.position = e.transform.transform_point(v.position);
                    tv.normal = e.transform.transform_normal(v.normal);
                    combined.vertices.push_back(tv);
                }
                for (auto idx : e.mesh.indices) combined.indices.push_back(idx + off);
            }
            GCodeSettings s; s.layer_height = lh;
            if (generate_gcode(out, combined, s)) {
                app.log(LogEntry::Info, "Wrote " + out);
            } else { app.log(LogEntry::Error, "G-code failed"); }
        }, {"3dprint"}});

    r.register_command({"asset", "asset list|spawn <name>", "Asset library",
        [](App& app, std::istringstream& ss) {
            std::string sub; ss >> sub;
            auto& lib = AssetLibrary::instance();
            if (sub == "list" || sub.empty()) {
                for (auto& p : lib.primitives())
                    app.log(LogEntry::Info, "  [" + p.category + "] " + p.name);
                for (auto& s : lib.scenes())
                    app.log(LogEntry::Info, "  [Scene] " + s.name);
            } else if (sub == "spawn") {
                std::string nm;
                std::getline(ss, nm);
                if (!nm.empty() && nm[0] == ' ') nm.erase(0, 1);
                for (auto& p : lib.primitives()) {
                    if (p.name == nm) {
                        auto prim = p.factory();
                        if (prim) {
                            auto id = app.document().scene().add_primitive(nm, std::move(prim));
                            app.document().scene().select(id);
                            app.document().mark_dirty();
                            app.log(LogEntry::Info, "Spawned " + nm);
                        }
                        return;
                    }
                }
                for (auto& s : lib.scenes()) {
                    if (s.name == nm) {
                        s.install(app);
                        app.document().mark_dirty();
                        return;
                    }
                }
                app.log(LogEntry::Error, "Asset not found: " + nm);
            }
        }, {"library"}});

    r.register_command({"script", "script <code>", "Run inline script",
        [](App& app, std::istringstream& ss) {
            std::string code;
            std::getline(ss, code);
            ScriptInterpreter interp(app);
            interp.execute(code);
        }, {"eval"}});

    r.register_command({"run", "run <path>", "Run script file",
        [](App& app, std::istringstream& ss) {
            std::string path; ss >> path;
            std::ifstream f(path);
            if (!f.is_open()) { app.log(LogEntry::Error, "Cannot open " + path); return; }
            std::string src((std::istreambuf_iterator<char>(f)),
                             std::istreambuf_iterator<char>());
            ScriptInterpreter interp(app);
            interp.execute(src);
            app.log(LogEntry::Info, "Ran " + path);
        }, {"source"}});

    r.register_command({"stats", "stats", "Detailed mesh stats for selected",
        [](App& app, std::istringstream&) {
            auto sel = app.document().scene().selected_id();
            if (!sel) return;
            app.mesh_gen().rebuild(app.document().scene());
            for (auto& e : app.mesh_gen().entries()) {
                if (e.node_id != *sel) continue;
                auto s = compute_mesh_stats(e.mesh);
                char buf[512];
                std::snprintf(buf, sizeof buf,
                    "V=%d E=%d F=%d boundary=%d non-manifold=%d area=%.3f vol=%.3f genus=%d",
                    s.vertex_count, s.edge_count, s.triangle_count,
                    s.boundary_edge_count, s.non_manifold_edge_count,
                    s.total_area, s.total_volume, s.genus_estimate);
                app.log(LogEntry::Info, buf);
            }
        }, {}});

    r.register_command({"weld", "weld [tol]", "Weld nearby vertices",
        [](App& app, std::istringstream& ss) {
            auto sel = app.document().scene().selected_id();
            if (!sel) return;
            auto* n = app.document().scene().find(*sel);
            if (!n || !n->primitive || n->primitive->type() != PrimitiveType::Bot) {
                app.log(LogEntry::Warning, "Select a Bot primitive");
                return;
            }
            float tol = 1e-4f; ss >> tol;
            auto* bot = static_cast<Bot*>(n->primitive.get());
            Mesh m = bot->generate_mesh();
            Mesh welded = weld_vertices(m, tol);
            bot->bot_vertices.clear();
            bot->bot_faces.clear();
            for (auto& v : welded.vertices) bot->bot_vertices.push_back(v.position);
            bot->bot_faces = welded.indices;
            app.document().mark_dirty();
            app.log(LogEntry::Info, "Welded to " + std::to_string(welded.vertices.size()) + " vertices");
        }, {}});

    r.register_command({"smooth", "smooth [iter] [strength]", "Laplacian smooth",
        [](App& app, std::istringstream& ss) {
            auto sel = app.document().scene().selected_id();
            if (!sel) return;
            auto* n = app.document().scene().find(*sel);
            if (!n || !n->primitive || n->primitive->type() != PrimitiveType::Bot) return;
            int it = 5; float st = 0.5f;
            ss >> it >> st;
            auto* bot = static_cast<Bot*>(n->primitive.get());
            Mesh m = bot->generate_mesh();
            Mesh sm = laplacian_smooth(m, it, st);
            bot->bot_vertices.clear();
            bot->bot_faces.clear();
            for (auto& v : sm.vertices) bot->bot_vertices.push_back(v.position);
            bot->bot_faces = sm.indices;
            app.document().mark_dirty();
            app.log(LogEntry::Info, "Smoothed " + std::to_string(it) + " iters");
        }, {}});

    r.register_command({"subdiv", "subdiv", "Loop subdivision",
        [](App& app, std::istringstream&) {
            auto sel = app.document().scene().selected_id();
            if (!sel) return;
            auto* n = app.document().scene().find(*sel);
            if (!n || !n->primitive || n->primitive->type() != PrimitiveType::Bot) return;
            auto* bot = static_cast<Bot*>(n->primitive.get());
            Mesh m = bot->generate_mesh();
            Mesh sub = subdivide_loop(m);
            bot->bot_vertices.clear();
            bot->bot_faces.clear();
            for (auto& v : sub.vertices) bot->bot_vertices.push_back(v.position);
            bot->bot_faces = sub.indices;
            app.document().mark_dirty();
            app.log(LogEntry::Info, "Subdivided to " + std::to_string(sub.indices.size()/3) + " tris");
        }, {}});

    r.register_command({"decimate", "decimate [ratio]", "Decimate mesh",
        [](App& app, std::istringstream& ss) {
            auto sel = app.document().scene().selected_id();
            if (!sel) return;
            auto* n = app.document().scene().find(*sel);
            if (!n || !n->primitive || n->primitive->type() != PrimitiveType::Bot) return;
            float ratio = 0.5f; ss >> ratio;
            auto* bot = static_cast<Bot*>(n->primitive.get());
            Mesh m = bot->generate_mesh();
            Mesh dec = simplify_decimate(m, ratio);
            bot->bot_vertices.clear();
            bot->bot_faces.clear();
            for (auto& v : dec.vertices) bot->bot_vertices.push_back(v.position);
            bot->bot_faces = dec.indices;
            app.document().mark_dirty();
            app.log(LogEntry::Info, "Decimated to " + std::to_string(dec.indices.size()/3) + " tris");
        }, {"simplify"}});

    r.register_command({"fill_holes", "fill_holes", "Fill boundary holes",
        [](App& app, std::istringstream&) {
            auto sel = app.document().scene().selected_id();
            if (!sel) return;
            auto* n = app.document().scene().find(*sel);
            if (!n || !n->primitive || n->primitive->type() != PrimitiveType::Bot) return;
            auto* bot = static_cast<Bot*>(n->primitive.get());
            Mesh m = bot->generate_mesh();
            Mesh filled = fill_holes(m);
            bot->bot_vertices.clear();
            bot->bot_faces.clear();
            for (auto& v : filled.vertices) bot->bot_vertices.push_back(v.position);
            bot->bot_faces = filled.indices;
            app.document().mark_dirty();
            app.log(LogEntry::Info, "Filled holes");
        }, {"heal"}});

    r.register_command({"watertight", "watertight", "Check if mesh is watertight",
        [](App& app, std::istringstream&) {
            auto sel = app.document().scene().selected_id();
            if (!sel) return;
            app.mesh_gen().rebuild(app.document().scene());
            for (auto& e : app.mesh_gen().entries()) {
                if (e.node_id != *sel) continue;
                app.log(LogEntry::Info, is_watertight(e.mesh) ? "watertight" : "open");
            }
        }, {}});

    r.register_command({"play", "play", "Start timeline",
        [](App&, std::istringstream&) { app_timeline().play(); }, {}});
    r.register_command({"pause", "pause", "Pause timeline",
        [](App&, std::istringstream&) { app_timeline().pause(); }, {}});
    r.register_command({"stop", "stop", "Stop timeline",
        [](App&, std::istringstream&) { app_timeline().stop(); }, {}});

    r.register_command({"gizmo", "gizmo translate|rotate|scale|off",
        "Set gizmo mode",
        [](App& app, std::istringstream& ss) {
            std::string m; ss >> m;
            auto& g = app_gizmo_state();
            if (m == "translate" || m == "t") g.mode = GizmoMode::Translate;
            else if (m == "rotate" || m == "r") g.mode = GizmoMode::Rotate;
            else if (m == "scale" || m == "s") g.mode = GizmoMode::Scale;
            else g.mode = GizmoMode::None;
            app.log(LogEntry::Info, "Gizmo: " + m);
        }, {"giz"}});

    r.register_command({"measure_d", "measure_d <id1> <id2>",
        "Distance between two object centers",
        [](App& app, std::istringstream& ss) {
            unsigned int a, b;
            if (!(ss >> a >> b)) {
                app.log(LogEntry::Warning, "Usage: measure_d <id1> <id2>");
                return;
            }
            auto* na = app.document().scene().find(a);
            auto* nb = app.document().scene().find(b);
            if (!na || !nb) { app.log(LogEntry::Error, "Object(s) not found"); return; }
            Vec3 d = nb->position - na->position;
            float dist = d.length();
            char buf[256];
            std::snprintf(buf, sizeof buf, "Distance %u→%u: %.4f (dx=%.3f dy=%.3f dz=%.3f)",
                          a, b, dist, d.x, d.y, d.z);
            app.log(LogEntry::Info, buf);
        }, {"dist"}});

    r.register_command({"measure_a", "measure_a <id1> <id2> <id3>",
        "Angle between three points (at id2)",
        [](App& app, std::istringstream& ss) {
            unsigned int a, b, c;
            if (!(ss >> a >> b >> c)) return;
            auto* na = app.document().scene().find(a);
            auto* nb = app.document().scene().find(b);
            auto* nc = app.document().scene().find(c);
            if (!na || !nb || !nc) return;
            Vec3 ab = (na->position - nb->position).normalized();
            Vec3 cb = (nc->position - nb->position).normalized();
            float dot = ab.dot(cb);
            dot = std::max(-1.f, std::min(1.f, dot));
            float ang_rad = std::acos(dot);
            float ang_deg = ang_rad * 180.f / kPi;
            char buf[128];
            std::snprintf(buf, sizeof buf, "Angle at %u: %.3f° (%.4f rad)", b, ang_deg, ang_rad);
            app.log(LogEntry::Info, buf);
        }, {"ang"}});

    r.register_command({"snap", "snap <grid|off> [size]",
        "Toggle grid snapping",
        [](App& app, std::istringstream& ss) {
            std::string m; float sz = 0.25f;
            ss >> m >> sz;
            app.log(LogEntry::Info, "Snap: " + m + " size=" + std::to_string(sz));
        }, {}});

    r.register_command({"vol", "vol", "Total volume of all objects",
        [](App& app, std::istringstream&) {
            app.mesh_gen().rebuild(app.document().scene());
            float total = 0.f;
            for (auto& e : app.mesh_gen().entries()) {
                auto rr = analyze_mesh(e.mesh);
                total += rr.volume;
            }
            char buf[64];
            std::snprintf(buf, sizeof buf, "Total volume: %.4f", total);
            app.log(LogEntry::Info, buf);
        }, {}});

    r.register_command({"area", "area", "Total surface area",
        [](App& app, std::istringstream&) {
            app.mesh_gen().rebuild(app.document().scene());
            float total = 0.f;
            for (auto& e : app.mesh_gen().entries()) {
                auto rr = analyze_mesh(e.mesh);
                total += rr.surface_area;
            }
            char buf[64];
            std::snprintf(buf, sizeof buf, "Total surface area: %.4f", total);
            app.log(LogEntry::Info, buf);
        }, {}});

    r.register_command({"cm", "cm", "Center of mass of selected",
        [](App& app, std::istringstream&) {
            auto sel = app.document().scene().selected_id();
            if (!sel) return;
            app.mesh_gen().rebuild(app.document().scene());
            for (auto& e : app.mesh_gen().entries()) {
                if (e.node_id != *sel) continue;
                auto rr = analyze_mesh(e.mesh);
                char buf[128];
                std::snprintf(buf, sizeof buf, "CM: (%.3f, %.3f, %.3f)",
                              rr.centroid.x, rr.centroid.y, rr.centroid.z);
                app.log(LogEntry::Info, buf);
            }
        }, {"centroid"}});

    r.register_command({"help", "help", "Show available commands",
        [](App& app, std::istringstream&) {
            auto cmds = instance().list();
            app.log(LogEntry::Info, "=== " + std::to_string(cmds.size()) + " commands available ===");
            std::string line;
            int per_line = 0;
            for (auto* c : cmds) {
                line += c->name + " ";
                if (++per_line >= 8) {
                    app.log(LogEntry::Info, "  " + line);
                    line.clear();
                    per_line = 0;
                }
            }
            if (!line.empty()) app.log(LogEntry::Info, "  " + line);
            app.log(LogEntry::Info, "Type 'help <cmd>' for usage");
        }, {"?", "h"}});
}

}  // namespace smidr
