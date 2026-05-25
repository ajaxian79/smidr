#include "core/Document.h"

#include <fstream>
#include "io/json.hpp"

namespace smidr {

Document::Document() = default;

bool Document::save(const std::filesystem::path& p) {
    nlohmann::json j;
    j["format"]  = "smidr";
    j["version"] = "0.1.0";
    j["name"]    = name_;

    nlohmann::json sj;
    scene_.to_json(sj);
    j["scene"] = sj;

    std::ofstream f(p);
    if (!f.is_open()) return false;
    f << j.dump(2);
    f.close();

    path_  = p;
    dirty_ = false;
    return true;
}

bool Document::load(const std::filesystem::path& p) {
    std::ifstream f(p);
    if (!f.is_open()) return false;

    nlohmann::json j;
    try {
        f >> j;
    } catch (...) {
        return false;
    }

    if (j.value("format", "") != "smidr") return false;

    name_ = j.value("name", "Untitled");
    scene_.from_json(j.at("scene"));

    path_  = p;
    dirty_ = false;
    return true;
}

void Document::new_document(const std::string& name) {
    scene_.clear();
    name_  = name;
    path_.clear();
    dirty_ = false;
}

}  // namespace smidr
