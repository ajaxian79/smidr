#pragma once

#include "core/Scene.h"

#include <filesystem>
#include <string>

namespace smidr {

class Document {
public:
    Document();

    Scene& scene() { return scene_; }
    const Scene& scene() const { return scene_; }

    const std::string& name() const { return name_; }
    void set_name(const std::string& n) { name_ = n; dirty_ = true; }

    bool dirty() const { return dirty_; }
    void mark_dirty() { dirty_ = true; }
    void clear_dirty() { dirty_ = false; }

    const std::filesystem::path& path() const { return path_; }

    bool save(const std::filesystem::path& p);
    bool load(const std::filesystem::path& p);

    void new_document(const std::string& name = "Untitled");

private:
    Scene                 scene_;
    std::string           name_ = "Untitled";
    std::filesystem::path path_;
    bool                  dirty_ = false;
};

}  // namespace smidr
