#include "io/FormatImport.h"
#include "core/PrimitivesAdvanced.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>

namespace smidr {

std::string detect_format(const std::filesystem::path& path) {
    auto ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    if (ext == ".stl") return "stl";
    if (ext == ".obj") return "obj";
    if (ext == ".ply") return "ply";
    if (ext == ".off") return "off";
    return "";
}

bool import_stl(const std::filesystem::path& path, Scene& scene) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return false;

    char header[80];
    f.read(header, 80);
    uint32_t num_tris = 0;
    f.read(reinterpret_cast<char*>(&num_tris), 4);

    if (num_tris == 0 || num_tris > 10000000) return false;

    auto bot = std::make_unique<Bot>();
    bot->bot_vertices.clear();
    bot->bot_faces.clear();

    for (uint32_t i = 0; i < num_tris; ++i) {
        float data[12];
        f.read(reinterpret_cast<char*>(data), 48);
        uint16_t attr;
        f.read(reinterpret_cast<char*>(&attr), 2);
        if (!f.good()) break;

        uint32_t base = static_cast<uint32_t>(bot->bot_vertices.size());
        bot->bot_vertices.push_back({data[3], data[4], data[5]});
        bot->bot_vertices.push_back({data[6], data[7], data[8]});
        bot->bot_vertices.push_back({data[9], data[10], data[11]});
        bot->bot_faces.push_back(base);
        bot->bot_faces.push_back(base + 1);
        bot->bot_faces.push_back(base + 2);
    }

    scene.add_primitive(path.stem().string(), std::move(bot));
    return true;
}

bool import_obj(const std::filesystem::path& path, Scene& scene) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    auto bot = std::make_unique<Bot>();
    bot->bot_vertices.clear();
    bot->bot_faces.clear();

    std::string line;
    while (std::getline(f, line)) {
        std::istringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "v") {
            float x, y, z;
            ss >> x >> y >> z;
            bot->bot_vertices.push_back({x, y, z});
        } else if (token == "f") {
            std::vector<uint32_t> face_verts;
            std::string vert;
            while (ss >> vert) {
                int idx = std::atoi(vert.c_str());
                if (idx < 0) idx = static_cast<int>(bot->bot_vertices.size()) + idx + 1;
                if (idx > 0) face_verts.push_back(static_cast<uint32_t>(idx - 1));
            }
            for (size_t i = 1; i + 1 < face_verts.size(); ++i) {
                bot->bot_faces.push_back(face_verts[0]);
                bot->bot_faces.push_back(face_verts[i]);
                bot->bot_faces.push_back(face_verts[i + 1]);
            }
        }
    }

    if (bot->bot_vertices.empty()) return false;
    scene.add_primitive(path.stem().string(), std::move(bot));
    return true;
}

bool import_ply(const std::filesystem::path& path, Scene& scene) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    std::string line;
    std::getline(f, line);
    if (line.find("ply") == std::string::npos) return false;

    int vertex_count = 0, face_count = 0;
    bool in_header = true;
    bool binary = false;

    while (in_header && std::getline(f, line)) {
        if (line.find("format binary") != std::string::npos) binary = true;
        if (line.find("element vertex") != std::string::npos)
            std::sscanf(line.c_str(), "element vertex %d", &vertex_count);
        if (line.find("element face") != std::string::npos)
            std::sscanf(line.c_str(), "element face %d", &face_count);
        if (line.find("end_header") != std::string::npos) in_header = false;
    }

    if (binary || vertex_count == 0) return false;

    auto bot = std::make_unique<Bot>();
    bot->bot_vertices.clear();
    bot->bot_faces.clear();

    for (int i = 0; i < vertex_count && std::getline(f, line); ++i) {
        float x = 0, y = 0, z = 0;
        std::sscanf(line.c_str(), "%f %f %f", &x, &y, &z);
        bot->bot_vertices.push_back({x, y, z});
    }

    for (int i = 0; i < face_count && std::getline(f, line); ++i) {
        std::istringstream ss(line);
        int n; ss >> n;
        std::vector<uint32_t> verts;
        for (int j = 0; j < n; ++j) {
            int idx; ss >> idx;
            verts.push_back(static_cast<uint32_t>(idx));
        }
        for (size_t j = 1; j + 1 < verts.size(); ++j) {
            bot->bot_faces.push_back(verts[0]);
            bot->bot_faces.push_back(verts[j]);
            bot->bot_faces.push_back(verts[j + 1]);
        }
    }

    if (bot->bot_vertices.empty()) return false;
    scene.add_primitive(path.stem().string(), std::move(bot));
    return true;
}

bool import_off(const std::filesystem::path& path, Scene& scene) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    std::string line;
    std::getline(f, line);
    if (line.find("OFF") == std::string::npos) return false;

    int nv = 0, nf = 0, ne = 0;
    f >> nv >> nf >> ne;

    auto bot = std::make_unique<Bot>();
    bot->bot_vertices.clear();
    bot->bot_faces.clear();

    for (int i = 0; i < nv; ++i) {
        float x, y, z;
        f >> x >> y >> z;
        bot->bot_vertices.push_back({x, y, z});
    }

    for (int i = 0; i < nf; ++i) {
        int n; f >> n;
        std::vector<uint32_t> verts;
        for (int j = 0; j < n; ++j) {
            int idx; f >> idx;
            verts.push_back(static_cast<uint32_t>(idx));
        }
        for (size_t j = 1; j + 1 < verts.size(); ++j) {
            bot->bot_faces.push_back(verts[0]);
            bot->bot_faces.push_back(verts[j]);
            bot->bot_faces.push_back(verts[j + 1]);
        }
    }

    if (bot->bot_vertices.empty()) return false;
    scene.add_primitive(path.stem().string(), std::move(bot));
    return true;
}

}  // namespace smidr
