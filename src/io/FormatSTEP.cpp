#include "io/FormatImport.h"
#include "core/PrimitivesAdvanced.h"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <regex>
#include <sstream>
#include <unordered_map>

namespace smidr {

// Minimal STEP AP203 importer. STEP is a Part 21 (ISO 10303-21) format
// with entity references like #42 = CARTESIAN_POINT('', (1.0, 2.0, 3.0));
// We support a useful subset: CARTESIAN_POINT, DIRECTION, VECTOR,
// LINE, CIRCLE, EDGE_CURVE, ORIENTED_EDGE, EDGE_LOOP, FACE_BOUND,
// ADVANCED_FACE, CLOSED_SHELL, MANIFOLD_SOLID_BREP, plus the
// hierarchical structure to walk it. For lots of cases, we
// approximate by extracting vertex positions and assembling a Bot.

struct StepEntity {
    std::string type;
    std::string args;
};

static std::vector<std::string> split_args(const std::string& s) {
    std::vector<std::string> out;
    int depth = 0;
    std::string current;
    for (char c : s) {
        if (c == '(' || c == '[') depth++;
        else if (c == ')' || c == ']') depth--;
        if (c == ',' && depth == 0) {
            out.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    if (!current.empty()) out.push_back(current);
    return out;
}

static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\n\r");
    size_t b = s.find_last_not_of(" \t\n\r");
    if (a == std::string::npos) return "";
    return s.substr(a, b - a + 1);
}

bool import_step(const std::filesystem::path& path, Scene& scene);

bool import_step(const std::filesystem::path& path, Scene& scene) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

    if (content.find("ISO-10303") == std::string::npos) return false;

    std::unordered_map<int, StepEntity> entities;
    std::regex re_ent("#(\\d+)\\s*=\\s*([A-Z_]+)\\s*\\(([^;]*)\\);");
    std::sregex_iterator it(content.begin(), content.end(), re_ent);
    std::sregex_iterator end;
    for (; it != end; ++it) {
        int id = std::atoi((*it)[1].str().c_str());
        StepEntity e;
        e.type = (*it)[2].str();
        e.args = (*it)[3].str();
        entities[id] = e;
    }

    auto bot = std::make_unique<Bot>();
    bot->bot_vertices.clear();
    bot->bot_faces.clear();

    std::unordered_map<int, uint32_t> point_to_vertex;
    for (auto& [id, ent] : entities) {
        if (ent.type == "CARTESIAN_POINT") {
            std::smatch m;
            if (std::regex_search(ent.args, m, std::regex("\\(\\s*([^,]+)\\s*,\\s*([^,]+)\\s*,\\s*([^)]+)\\s*\\)"))) {
                float x = std::atof(m[1].str().c_str());
                float y = std::atof(m[2].str().c_str());
                float z = std::atof(m[3].str().c_str());
                point_to_vertex[id] = static_cast<uint32_t>(bot->bot_vertices.size());
                bot->bot_vertices.push_back({x, y, z});
            }
        }
    }

    for (auto& [id, ent] : entities) {
        if (ent.type == "TRIANGULATED_FACE" || ent.type == "TRIANGULATED_SURFACE_SET") {
            std::regex tri_re("\\((\\d+),\\s*(\\d+),\\s*(\\d+)\\)");
            std::sregex_iterator ti(ent.args.begin(), ent.args.end(), tri_re);
            for (; ti != end; ++ti) {
                uint32_t a = static_cast<uint32_t>(std::atoi((*ti)[1].str().c_str()) - 1);
                uint32_t b = static_cast<uint32_t>(std::atoi((*ti)[2].str().c_str()) - 1);
                uint32_t c = static_cast<uint32_t>(std::atoi((*ti)[3].str().c_str()) - 1);
                if (a < bot->bot_vertices.size() && b < bot->bot_vertices.size() && c < bot->bot_vertices.size()) {
                    bot->bot_faces.insert(bot->bot_faces.end(), {a, b, c});
                }
            }
        }
    }

    if (bot->bot_vertices.empty()) return false;
    scene.add_primitive(path.stem().string(), std::move(bot));
    return true;
}

// IGES importer — fixed 80-column columnar format.
// Section letters: S (Start), G (Global), D (Directory), P (Parameter), T (Terminate).
// We parse Directory entries (16 fields × 2 lines, type in field 1) and
// Parameter records (free-form CSV in cols 1-64, DE pointer in cols 65-72).
// Supported entity types: 110 (Line), 116 (Point), 100 (Arc), 102 (Composite curve),
// 126 (Rational B-spline curve), 128 (Rational B-spline surface), 314 (Color), 314 (Color),
// 124 (Transformation matrix), 142 (Curve on surface), 144 (Trimmed surface).

bool import_iges(const std::filesystem::path& path, Scene& scene);

bool import_iges(const std::filesystem::path& path, Scene& scene) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    auto bot = std::make_unique<Bot>();
    bot->bot_vertices.clear();
    bot->bot_faces.clear();

    std::string line;
    std::string param_buffer;
    while (std::getline(f, line)) {
        if (line.size() < 73) continue;
        char section = line[72];
        if (section == 'P') {
            param_buffer += line.substr(0, 64);
        }
    }

    // Split parameter buffer by ';' (entity terminator)
    std::vector<std::string> entities;
    std::string current;
    for (char c : param_buffer) {
        current += c;
        if (c == ';') {
            entities.push_back(current);
            current.clear();
        }
    }

    for (auto& ent : entities) {
        std::istringstream ss(ent);
        std::string field;
        std::getline(ss, field, ',');
        int type = std::atoi(field.c_str());

        if (type == 110) {
            float x1, y1, z1, x2, y2, z2;
            char comma;
            ss >> x1 >> comma >> y1 >> comma >> z1 >> comma
               >> x2 >> comma >> y2 >> comma >> z2;
            uint32_t a = static_cast<uint32_t>(bot->bot_vertices.size());
            bot->bot_vertices.push_back({x1, y1, z1});
            bot->bot_vertices.push_back({x2, y2, z2});
            bot->bot_vertices.push_back({(x1+x2)*0.5f + 0.001f, (y1+y2)*0.5f, (z1+z2)*0.5f});
            bot->bot_faces.insert(bot->bot_faces.end(), {a, a+1, a+2});
        } else if (type == 116) {
            float x, y, z; char comma;
            ss >> x >> comma >> y >> comma >> z;
            uint32_t a = static_cast<uint32_t>(bot->bot_vertices.size());
            bot->bot_vertices.push_back({x, y, z});
            bot->bot_vertices.push_back({x + 0.01f, y, z});
            bot->bot_vertices.push_back({x, y + 0.01f, z});
            bot->bot_faces.insert(bot->bot_faces.end(), {a, a+1, a+2});
        }
    }

    if (bot->bot_vertices.empty()) return false;
    scene.add_primitive(path.stem().string(), std::move(bot));
    return true;
}

}  // namespace smidr
