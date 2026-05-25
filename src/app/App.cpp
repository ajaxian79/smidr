#include "app/App.h"

#include <cstdio>
#include <memory>

namespace smidr {

void App::init(GLFWwindow* win, const smidr::ui::Fonts& fonts) {
    window_ = win;
    fonts_  = &fonts;
    renderer_.init();
    doc_.new_document("Untitled");
    log(LogEntry::Info, "smidr v0.1.0 ready");
}

void App::update() {
    if (meshes_dirty_) {
        rebuild_meshes();
        meshes_dirty_ = false;
    }
}

void App::add_primitive(PrimitiveType type) {
    std::unique_ptr<Primitive> prim;
    const char* base_name = "Object";

    switch (type) {
        case PrimitiveType::Sphere:
            prim = std::make_unique<Sphere>();
            base_name = "Sphere";
            break;
        case PrimitiveType::Box:
            prim = std::make_unique<Box>();
            base_name = "Box";
            break;
        case PrimitiveType::Cylinder:
            prim = std::make_unique<Cylinder>();
            base_name = "Cylinder";
            break;
        case PrimitiveType::Cone:
            prim = std::make_unique<Cone>();
            base_name = "Cone";
            break;
        case PrimitiveType::Torus:
            prim = std::make_unique<Torus>();
            base_name = "Torus";
            break;
        case PrimitiveType::Ellipsoid:
            prim = std::make_unique<Ellipsoid>();
            base_name = "Ellipsoid";
            break;
        case PrimitiveType::Halfspace:
            prim = std::make_unique<Halfspace>();
            base_name = "Halfspace";
            break;
        case PrimitiveType::Pipe:
            prim = std::make_unique<Pipe>();
            base_name = "Pipe";
            break;
        case PrimitiveType::Wedge:
            prim = std::make_unique<Wedge>();
            base_name = "Wedge";
            break;
        case PrimitiveType::Arb8:
            prim = std::make_unique<Arb8>();
            base_name = "Arb8";
            break;
    }

    char name[64];
    std::snprintf(name, sizeof name, "%s.%03d", base_name, ++prim_counter_);

    auto id = doc_.scene().add_primitive(name, std::move(prim));
    doc_.scene().select(id);
    doc_.mark_dirty();
    meshes_dirty_ = true;

    log(LogEntry::Info, std::string("Added ") + name);
}

void App::delete_selected() {
    auto sel = doc_.scene().selected_id();
    if (!sel) return;

    auto* node = doc_.scene().find(*sel);
    std::string name = node ? node->name : "?";

    doc_.scene().remove_node(*sel);
    doc_.mark_dirty();
    meshes_dirty_ = true;
    log(LogEntry::Info, "Deleted " + name);
}

void App::boolean_selected(BooleanOp op) {
    auto sel = doc_.scene().selected_id();
    if (!sel) {
        log(LogEntry::Warning, "Select two objects for boolean op");
        return;
    }

    auto& roots = doc_.scene().root_ids();
    if (roots.size() < 2) {
        log(LogEntry::Warning, "Need at least 2 root objects");
        return;
    }

    NodeId a = *sel;
    NodeId b = kInvalidNode;
    for (auto rid : roots) {
        if (rid != a) { b = rid; break; }
    }
    if (b == kInvalidNode) return;

    char name[64];
    std::snprintf(name, sizeof name, "%s.%03d",
                  boolean_op_name(op), ++prim_counter_);

    auto id = doc_.scene().add_boolean(name, op, a, b);
    doc_.scene().select(id);
    doc_.mark_dirty();
    meshes_dirty_ = true;

    log(LogEntry::Info, std::string("Created ") + name);
}

void App::rebuild_meshes() {
    mesh_gen_.rebuild(doc_.scene());
}

void App::log(LogEntry::Level level, const std::string& msg) {
    log_.push_back({level, msg});
    if (log_.size() > 200) log_.erase(log_.begin());
}

}  // namespace smidr
