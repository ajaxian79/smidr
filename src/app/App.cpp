#include "app/App.h"
#include "core/Commands.h"
#include "core/PrimitivesAdvanced.h"

#include <cstdio>
#include <memory>

namespace smidr {

void App::init(GLFWwindow* win, const smidr::ui::Fonts& fonts) {
    window_ = win;
    fonts_  = &fonts;
    renderer_.init();
    doc_.new_document("Untitled");
    history_.clear();
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
        case PrimitiveType::Sphere:    prim = std::make_unique<Sphere>();    base_name = "Sphere"; break;
        case PrimitiveType::Box:       prim = std::make_unique<Box>();       base_name = "Box"; break;
        case PrimitiveType::Cylinder:  prim = std::make_unique<Cylinder>();  base_name = "Cylinder"; break;
        case PrimitiveType::Cone:      prim = std::make_unique<Cone>();      base_name = "Cone"; break;
        case PrimitiveType::Torus:     prim = std::make_unique<Torus>();     base_name = "Torus"; break;
        case PrimitiveType::Ellipsoid: prim = std::make_unique<Ellipsoid>(); base_name = "Ellipsoid"; break;
        case PrimitiveType::Halfspace: prim = std::make_unique<Halfspace>(); base_name = "Halfspace"; break;
        case PrimitiveType::Pipe:      prim = std::make_unique<Pipe>();      base_name = "Pipe"; break;
        case PrimitiveType::Wedge:     prim = std::make_unique<Wedge>();     base_name = "Wedge"; break;
        case PrimitiveType::Arb8:           prim = std::make_unique<Arb8>();             base_name = "Arb8"; break;
        case PrimitiveType::Superellipsoid: prim = std::make_unique<Superellipsoid>();  base_name = "Superellipsoid"; break;
        case PrimitiveType::Particle:       prim = std::make_unique<Particle>();        base_name = "Particle"; break;
        case PrimitiveType::Arbn:           prim = std::make_unique<Arbn>();            base_name = "Arbn"; break;
        case PrimitiveType::RPC:            prim = std::make_unique<RPC>();             base_name = "RPC"; break;
        case PrimitiveType::RHC:            prim = std::make_unique<RHC>();             base_name = "RHC"; break;
        case PrimitiveType::EPA:            prim = std::make_unique<EPA>();             base_name = "EPA"; break;
        case PrimitiveType::EHY:            prim = std::make_unique<EHY>();             base_name = "EHY"; break;
        case PrimitiveType::ETO:            prim = std::make_unique<ETO>();             base_name = "ETO"; break;
        case PrimitiveType::Hyperboloid:    prim = std::make_unique<Hyperboloid>();     base_name = "Hyperboloid"; break;
        case PrimitiveType::Bot:            prim = std::make_unique<Bot>();             base_name = "Bot"; break;
        case PrimitiveType::Sketch:         prim = std::make_unique<SketchPrimitive>(); base_name = "Sketch"; break;
        case PrimitiveType::Extrude:        prim = std::make_unique<ExtrudePrimitive>();base_name = "Extrude"; break;
        case PrimitiveType::Revolve:        prim = std::make_unique<RevolvePrimitive>();base_name = "Revolve"; break;
        case PrimitiveType::DSP:            prim = std::make_unique<DSPPrimitive>();    base_name = "DSP"; break;
        case PrimitiveType::Metaball:       prim = std::make_unique<MetaballPrimitive>();base_name = "Metaball"; break;
        case PrimitiveType::Heart:          prim = std::make_unique<HeartPrimitive>();  base_name = "Heart"; break;
        case PrimitiveType::PointCloud:     prim = std::make_unique<PointCloudPrimitive>(); base_name = "PointCloud"; break;
        case PrimitiveType::Annotation:     prim = std::make_unique<AnnotationPrimitive>(); base_name = "Annotation"; break;
    }

    char name[64];
    std::snprintf(name, sizeof name, "%s.%03d", base_name, ++prim_counter_);

    history_.execute(std::make_unique<AddPrimitiveCommand>(
        doc_.scene(), name, std::move(prim)));
    doc_.mark_dirty();
    meshes_dirty_ = true;

    log(LogEntry::Info, std::string("Added ") + name);
}

void App::delete_selected() {
    auto sel = doc_.scene().selected_id();
    if (!sel) return;

    auto* node = doc_.scene().find(*sel);
    std::string name = node ? node->name : "?";

    history_.execute(std::make_unique<DeleteNodeCommand>(doc_.scene(), *sel));
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

void App::undo() {
    if (!history_.can_undo()) return;
    history_.undo();
    doc_.mark_dirty();
    meshes_dirty_ = true;
    log(LogEntry::Info, "Undo: " + history_.redo_description());
}

void App::redo() {
    if (!history_.can_redo()) return;
    history_.redo();
    doc_.mark_dirty();
    meshes_dirty_ = true;
    log(LogEntry::Info, "Redo: " + history_.undo_description());
}

void App::rebuild_meshes() {
    mesh_gen_.rebuild(doc_.scene());
}

void App::log(LogEntry::Level level, const std::string& msg) {
    log_.push_back({level, msg});
    if (log_.size() > 200) log_.erase(log_.begin());
}

}  // namespace smidr
