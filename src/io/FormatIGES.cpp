#include "io/FormatImport.h"
#include "io/MeshExport.h"
#include "core/PrimitivesAdvanced.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

namespace smidr {

// IGES is a complex columnar fixed-width format with sections (Start,
// Global, Directory, Parameter, Terminate). Full support requires
// 100+ entity types. We export only Type 410 (3D plane facets via
// composite curve segments) and import a subset of common entities.

bool export_iges(const std::filesystem::path& path, const MeshGenerator& meshes) {
    std::ofstream f(path);
    if (!f.is_open()) return false;

    auto pad = [](const std::string& s, int n, char fill = ' ') {
        if (static_cast<int>(s.size()) >= n) return s.substr(0, n);
        return s + std::string(n - s.size(), fill);
    };

    int line_no = 1;
    auto write_line = [&](const std::string& content, char section) {
        std::string out = pad(content, 72);
        char num[12];
        std::snprintf(num, sizeof num, "%c%7d", section, line_no++);
        f << out << num << "\n";
    };

    write_line("Smidr IGES export", 'S');

    int gline = 1;
    auto global = [&](const std::string& s) {
        std::string out = pad(s, 72);
        char num[12];
        std::snprintf(num, sizeof num, "G%7d", gline++);
        f << out << num << "\n";
    };
    global("1H,,1H;,7Hsmidr.igs,7Hsmidr.igs,5Hsmidr,4H0001,32,38,6,38,15,");
    global("7Hsmidr.igs,1.,2,2HMM,1,0.01,15H20260101.000000,0.001,500.,");
    global("5HSmidr,5HSmidr,11,0,15H20260101.000000;");

    std::ostringstream dir, par;
    int de_line = 1, p_line = 1;

    for (auto& entry : meshes.entries()) {
        auto& m = entry.mesh;
        for (size_t i = 0; i + 2 < m.indices.size(); i += 3) {
            Vec3 v0 = entry.transform.transform_point(m.vertices[m.indices[i]].position);
            Vec3 v1 = entry.transform.transform_point(m.vertices[m.indices[i+1]].position);
            Vec3 v2 = entry.transform.transform_point(m.vertices[m.indices[i+2]].position);

            char dline[128];
            std::snprintf(dline, sizeof dline, "%8d%8d%8d%8d%8d%8d%8d%8d%8d%8d",
                          108, p_line, 0, 0, 0, 0, 0, 0, 0, 0);
            std::string dl(dline);
            std::string dnum;
            char buf[12];
            std::snprintf(buf, sizeof buf, "D%7d", de_line++);
            f << pad(dl, 72) << buf << "\n";
            std::snprintf(dline, sizeof dline, "%8d%8d%8d%8d%8d%8d        %8s%s",
                          108, 0, 0, 1, 0, 0, "PLANE", "");
            std::snprintf(buf, sizeof buf, "D%7d", de_line++);
            f << pad(std::string(dline), 72) << buf << "\n";

            Vec3 n = (v1 - v0).cross(v2 - v0).normalized();
            float d = n.dot(v0);
            char pline[160];
            std::snprintf(pline, sizeof pline,
                "108,%f,%f,%f,%f,0,%f,%f,%f,0;",
                n.x, n.y, n.z, d, v0.x, v0.y, v0.z);
            std::snprintf(buf, sizeof buf, " %7dP%7d", de_line - 2, p_line++);
            f << pad(std::string(pline), 64) << buf << "\n";
        }
    }

    char buf2[12];
    std::snprintf(buf2, sizeof buf2, "S%7dG%7dD%7dP%7d", 1, 3, de_line - 1, p_line - 1);
    f << pad("", 72) << "T      1\n";
    return f.good();
}

}  // namespace smidr
