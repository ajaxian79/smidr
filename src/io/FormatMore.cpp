#include "io/FormatImport.h"
#include "io/MeshExport.h"
#include "core/PrimitivesAdvanced.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <regex>
#include <sstream>

namespace smidr {

// Collada (.dae) ASCII XML — common simple variant: <mesh> with
// <source> arrays for positions and <triangles>/<polylist> for faces.
bool import_collada(const std::filesystem::path& path, Scene& scene) {
    std::ifstream f(path);
    if (!f.is_open()) return false;
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

    auto bot = std::make_unique<Bot>();
    bot->bot_vertices.clear();
    bot->bot_faces.clear();

    // Find first <float_array> ... </float_array>
    std::regex pos_re("<float_array[^>]*>([^<]*)</float_array>");
    std::smatch m;
    if (std::regex_search(content, m, pos_re)) {
        std::istringstream is(m[1].str());
        float x, y, z;
        while (is >> x >> y >> z) {
            bot->bot_vertices.push_back({x, y, z});
        }
    }

    // Find <p> ... </p> (vertex indices, possibly with multiple inputs)
    std::regex p_re("<p>([^<]*)</p>");
    auto begin = std::sregex_iterator(content.begin(), content.end(), p_re);
    auto end = std::sregex_iterator();
    int stride = 1;
    std::smatch sm;
    std::regex input_re("<input[^/]*offset");
    auto in_it = std::sregex_iterator(content.begin(), content.end(), input_re);
    int count = 0;
    while (in_it != std::sregex_iterator()) { ++count; ++in_it; }
    if (count > 0) stride = count;

    for (auto it = begin; it != end; ++it) {
        std::istringstream is((*it)[1].str());
        std::vector<int> values;
        int v;
        while (is >> v) values.push_back(v);
        for (size_t i = 0; i + 3 * stride - 1 < values.size(); i += 3 * stride) {
            uint32_t a = static_cast<uint32_t>(values[i]);
            uint32_t b = static_cast<uint32_t>(values[i + stride]);
            uint32_t c = static_cast<uint32_t>(values[i + 2 * stride]);
            if (a < bot->bot_vertices.size() && b < bot->bot_vertices.size() && c < bot->bot_vertices.size()) {
                bot->bot_faces.insert(bot->bot_faces.end(), {a, b, c});
            }
        }
    }

    if (bot->bot_vertices.empty()) return false;
    scene.add_primitive(path.stem().string(), std::move(bot));
    return true;
}

// FBX ASCII — very limited subset. FBX is normally binary, but ASCII variant
// has Vertices: *N { a:x,y,z,... } and PolygonVertexIndex: *M { a:i,j,~k,... }
// where ~ marks the last vertex of a polygon (XOR'd with -1).
bool import_fbx(const std::filesystem::path& path, Scene& scene) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

    if (content.find("FBXVersion") == std::string::npos &&
        content.find("Kaydara FBX") == std::string::npos) return false;

    auto bot = std::make_unique<Bot>();
    bot->bot_vertices.clear();
    bot->bot_faces.clear();

    auto extract_array = [&](const std::string& key) -> std::string {
        size_t p = content.find(key);
        if (p == std::string::npos) return "";
        p = content.find('{', p);
        if (p == std::string::npos) return "";
        size_t q = content.find('}', p);
        if (q == std::string::npos) return "";
        std::string body = content.substr(p + 1, q - p - 1);
        size_t colon = body.find(':');
        return colon != std::string::npos ? body.substr(colon + 1) : body;
    };

    std::string verts = extract_array("Vertices:");
    if (!verts.empty()) {
        std::istringstream is(verts);
        float x, y, z;
        char comma;
        while (is >> x) {
            is >> comma >> y >> comma >> z;
            bot->bot_vertices.push_back({x, y, z});
            is >> comma;
        }
    }

    std::string idx = extract_array("PolygonVertexIndex:");
    if (!idx.empty()) {
        std::istringstream is(idx);
        std::vector<int> poly;
        int v;
        char comma;
        while (is >> v) {
            if (v < 0) {
                poly.push_back(~v);
                for (size_t i = 1; i + 1 < poly.size(); ++i) {
                    bot->bot_faces.push_back(static_cast<uint32_t>(poly[0]));
                    bot->bot_faces.push_back(static_cast<uint32_t>(poly[i]));
                    bot->bot_faces.push_back(static_cast<uint32_t>(poly[i+1]));
                }
                poly.clear();
            } else {
                poly.push_back(v);
            }
            is >> comma;
        }
    }

    if (bot->bot_vertices.empty()) return false;
    scene.add_primitive(path.stem().string(), std::move(bot));
    return true;
}

// 3DM (Rhino binary) — full format is openNURBS; we provide a stub
// that reports failure (real impl needs the openNURBS library).
bool import_3dm(const std::filesystem::path& path, Scene& scene) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return false;
    char header[16] = {};
    f.read(header, 4);
    if (std::strncmp(header, "3DMC", 4) != 0 && std::strncmp(header, "3DCC", 4) != 0)
        return false;
    (void)scene;
    return false;
}

bool export_collada(const std::filesystem::path& path, const MeshGenerator& meshes) {
    std::ofstream f(path);
    if (!f.is_open()) return false;

    f << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    f << "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n";
    f << "<asset><contributor><authoring_tool>smidr</authoring_tool></contributor>\n";
    f << "<up_axis>Y_UP</up_axis></asset>\n";
    f << "<library_geometries>\n";

    for (size_t ei = 0; ei < meshes.entries().size(); ++ei) {
        auto& entry = meshes.entries()[ei];
        auto& m = entry.mesh;
        f << "<geometry id=\"g" << ei << "\"><mesh>\n";
        f << "<source id=\"g" << ei << "-pos\"><float_array id=\"g" << ei << "-pos-arr\" count=\""
          << (m.vertices.size() * 3) << "\">";
        for (auto& v : m.vertices) {
            Vec3 p = entry.transform.transform_point(v.position);
            f << p.x << " " << p.y << " " << p.z << " ";
        }
        f << "</float_array>\n";
        f << "<technique_common><accessor source=\"#g" << ei << "-pos-arr\" count=\""
          << m.vertices.size() << "\" stride=\"3\">"
          << "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/><param name=\"Z\" type=\"float\"/>"
          << "</accessor></technique_common></source>\n";
        f << "<vertices id=\"g" << ei << "-v\"><input semantic=\"POSITION\" source=\"#g" << ei << "-pos\"/></vertices>\n";
        f << "<triangles count=\"" << (m.indices.size() / 3) << "\"><input semantic=\"VERTEX\" source=\"#g" << ei << "-v\" offset=\"0\"/><p>";
        for (auto idx : m.indices) f << idx << " ";
        f << "</p></triangles>\n";
        f << "</mesh></geometry>\n";
    }
    f << "</library_geometries>\n";
    f << "<scene><instance_visual_scene url=\"#s\"/></scene>\n";
    f << "<library_visual_scenes><visual_scene id=\"s\"><node id=\"root\">\n";
    for (size_t i = 0; i < meshes.entries().size(); ++i)
        f << "<instance_geometry url=\"#g" << i << "\"/>\n";
    f << "</node></visual_scene></library_visual_scenes>\n";
    f << "</COLLADA>\n";
    return f.good();
}

}  // namespace smidr
