#pragma once

#include "core/Document.h"
#include "core/MeshGenerator.h"
#include "render/Renderer.h"
#include "render/Camera.h"

#include <string>
#include <vector>

struct GLFWwindow;

namespace smidr::ui { struct Fonts; }

namespace smidr {

struct LogEntry {
    enum Level { Info, Warning, Error };
    Level       level;
    std::string message;
};

class App {
public:
    void init(GLFWwindow* win, const smidr::ui::Fonts& fonts);
    void update();
    bool wants_quit() const { return quit_; }

    void add_primitive(PrimitiveType type);
    void delete_selected();
    void boolean_selected(BooleanOp op);

    Document& document() { return doc_; }
    Camera&   camera()   { return camera_; }
    Renderer& renderer() { return renderer_; }
    MeshGenerator& mesh_gen() { return mesh_gen_; }

    void log(LogEntry::Level level, const std::string& msg);
    const std::vector<LogEntry>& log_entries() const { return log_; }

    bool show_splash = true;

private:
    void rebuild_meshes();

    Document      doc_;
    Camera        camera_;
    Renderer      renderer_;
    MeshGenerator mesh_gen_;

    GLFWwindow*         window_ = nullptr;
    const smidr::ui::Fonts* fonts_  = nullptr;

    bool quit_ = false;
    bool meshes_dirty_ = true;
    int  prim_counter_ = 0;

    std::vector<LogEntry> log_;
};

}  // namespace smidr
