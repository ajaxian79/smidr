#include "io/FormatImport.h"
#include "io/MeshExport.h"
#include "core/PrimitivesAdvanced.h"

#include <fstream>
#include <sstream>

namespace smidr {

bool export_vrml(const std::filesystem::path& path, const MeshGenerator& meshes) {
    std::ofstream f(path);
    if (!f.is_open()) return false;

    f << "#VRML V2.0 utf8\n";
    f << "# Smidr VRML export\n\n";

    for (size_t ei = 0; ei < meshes.entries().size(); ++ei) {
        auto& entry = meshes.entries()[ei];
        auto& m = entry.mesh;

        f << "Shape {\n  appearance Appearance {\n    material Material {\n";
        f << "      diffuseColor " << entry.color.x << " " << entry.color.y << " " << entry.color.z << "\n";
        f << "      transparency " << (1.f - entry.opacity) << "\n";
        f << "    }\n  }\n  geometry IndexedFaceSet {\n";
        f << "    coord Coordinate {\n      point [\n";
        for (auto& v : m.vertices) {
            Vec3 p = entry.transform.transform_point(v.position);
            f << "        " << p.x << " " << p.y << " " << p.z << ",\n";
        }
        f << "      ]\n    }\n";
        f << "    coordIndex [\n";
        for (size_t i = 0; i + 2 < m.indices.size(); i += 3) {
            f << "      " << m.indices[i] << ", " << m.indices[i+1] << ", " << m.indices[i+2] << ", -1,\n";
        }
        f << "    ]\n  }\n}\n\n";
    }
    return f.good();
}

bool export_x3d(const std::filesystem::path& path, const MeshGenerator& meshes) {
    std::ofstream f(path);
    if (!f.is_open()) return false;

    f << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    f << "<X3D profile=\"Immersive\" version=\"3.3\">\n";
    f << "  <Scene>\n";

    for (auto& entry : meshes.entries()) {
        auto& m = entry.mesh;
        f << "    <Shape>\n      <Appearance>\n";
        f << "        <Material diffuseColor=\"" << entry.color.x << " "
          << entry.color.y << " " << entry.color.z << "\"/>\n";
        f << "      </Appearance>\n";
        f << "      <IndexedFaceSet coordIndex=\"";
        for (size_t i = 0; i + 2 < m.indices.size(); i += 3) {
            f << m.indices[i] << " " << m.indices[i+1] << " " << m.indices[i+2] << " -1 ";
        }
        f << "\">\n        <Coordinate point=\"";
        for (auto& v : m.vertices) {
            Vec3 p = entry.transform.transform_point(v.position);
            f << p.x << " " << p.y << " " << p.z << " ";
        }
        f << "\"/>\n      </IndexedFaceSet>\n    </Shape>\n";
    }

    f << "  </Scene>\n</X3D>\n";
    return f.good();
}

bool import_vrml(const std::filesystem::path& path, Scene& scene) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    auto bot = std::make_unique<Bot>();
    bot->bot_vertices.clear();
    bot->bot_faces.clear();

    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

    size_t pt_start = content.find("point [");
    size_t coord_start = content.find("coordIndex [");
    if (pt_start == std::string::npos || coord_start == std::string::npos) return false;

    size_t pt_end = content.find(']', pt_start);
    std::string points = content.substr(pt_start + 7, pt_end - pt_start - 7);
    std::istringstream ps(points);
    float x, y, z;
    char comma;
    while (ps >> x >> y >> z) {
        bot->bot_vertices.push_back({x, y, z});
        ps >> comma;
    }

    size_t idx_end = content.find(']', coord_start);
    std::string indices = content.substr(coord_start + 12, idx_end - coord_start - 12);
    std::istringstream is(indices);
    int v;
    std::vector<uint32_t> face;
    while (is >> v) {
        if (v == -1) {
            for (size_t i = 1; i + 1 < face.size(); ++i) {
                bot->bot_faces.push_back(face[0]);
                bot->bot_faces.push_back(face[i]);
                bot->bot_faces.push_back(face[i+1]);
            }
            face.clear();
        } else {
            face.push_back(static_cast<uint32_t>(v));
        }
        is >> comma;
    }

    if (bot->bot_vertices.empty()) return false;
    scene.add_primitive(path.stem().string(), std::move(bot));
    return true;
}

}  // namespace smidr
