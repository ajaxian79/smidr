#include "io/FormatImport.h"
#include "io/MeshExport.h"
#include "core/PrimitivesAdvanced.h"

#include <fstream>
#include <sstream>
#include <string>

namespace smidr {

bool import_dxf(const std::filesystem::path& path, Scene& scene) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    auto bot = std::make_unique<Bot>();
    bot->bot_vertices.clear();
    bot->bot_faces.clear();

    std::string line;
    auto read_pair = [&](int& code, std::string& val) -> bool {
        if (!std::getline(f, line)) return false;
        line.erase(0, line.find_first_not_of(" \t"));
        code = std::atoi(line.c_str());
        if (!std::getline(f, val)) return false;
        val.erase(0, val.find_first_not_of(" \t"));
        size_t end = val.find_last_not_of(" \t\r\n");
        if (end != std::string::npos) val.resize(end + 1);
        return true;
    };

    int code = 0;
    std::string val;
    bool in_3dface = false;
    Vec3 corners[4];
    int corner_idx = 0;

    while (read_pair(code, val)) {
        if (code == 0) {
            if (in_3dface && corner_idx >= 3) {
                uint32_t base = static_cast<uint32_t>(bot->bot_vertices.size());
                for (int i = 0; i < corner_idx; ++i) bot->bot_vertices.push_back(corners[i]);
                bot->bot_faces.push_back(base);
                bot->bot_faces.push_back(base + 1);
                bot->bot_faces.push_back(base + 2);
                if (corner_idx == 4) {
                    bot->bot_faces.push_back(base);
                    bot->bot_faces.push_back(base + 2);
                    bot->bot_faces.push_back(base + 3);
                }
            }
            in_3dface = (val == "3DFACE");
            corner_idx = 0;
            for (auto& c : corners) c = {0, 0, 0};
        } else if (in_3dface) {
            int idx = -1;
            int comp = -1;
            if (code >= 10 && code <= 13) { idx = code - 10; comp = 0; }
            else if (code >= 20 && code <= 23) { idx = code - 20; comp = 1; }
            else if (code >= 30 && code <= 33) { idx = code - 30; comp = 2; }
            if (idx >= 0 && idx < 4) {
                float v = std::atof(val.c_str());
                if (comp == 0) corners[idx].x = v;
                else if (comp == 1) corners[idx].y = v;
                else corners[idx].z = v;
                if (idx + 1 > corner_idx) corner_idx = idx + 1;
            }
        }
    }

    if (bot->bot_vertices.empty()) return false;
    scene.add_primitive(path.stem().string(), std::move(bot));
    return true;
}

bool export_dxf(const std::filesystem::path& path, const MeshGenerator& meshes) {
    std::ofstream f(path);
    if (!f.is_open()) return false;

    f << "0\nSECTION\n2\nHEADER\n0\nENDSEC\n";
    f << "0\nSECTION\n2\nENTITIES\n";

    for (auto& entry : meshes.entries()) {
        auto& m = entry.mesh;
        for (size_t i = 0; i + 2 < m.indices.size(); i += 3) {
            Vec3 v0 = entry.transform.transform_point(m.vertices[m.indices[i]].position);
            Vec3 v1 = entry.transform.transform_point(m.vertices[m.indices[i+1]].position);
            Vec3 v2 = entry.transform.transform_point(m.vertices[m.indices[i+2]].position);
            f << "0\n3DFACE\n8\n0\n";
            f << "10\n" << v0.x << "\n20\n" << v0.y << "\n30\n" << v0.z << "\n";
            f << "11\n" << v1.x << "\n21\n" << v1.y << "\n31\n" << v1.z << "\n";
            f << "12\n" << v2.x << "\n22\n" << v2.y << "\n32\n" << v2.z << "\n";
            f << "13\n" << v2.x << "\n23\n" << v2.y << "\n33\n" << v2.z << "\n";
        }
    }

    f << "0\nENDSEC\n0\nEOF\n";
    return f.good();
}

}  // namespace smidr
