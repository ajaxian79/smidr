#include "io/MeshExport.h"

#include <cstdint>
#include <cstring>
#include <fstream>

namespace smidr {

bool export_stl(const std::filesystem::path& path, const MeshGenerator& meshes) {
    std::ofstream f(path, std::ios::binary);
    if (!f.is_open()) return false;

    uint32_t total_tris = 0;
    for (auto& e : meshes.entries())
        total_tris += static_cast<uint32_t>(e.mesh.indices.size() / 3);

    char header[80] = {};
    std::strncpy(header, "smidr STL export", sizeof header - 1);
    f.write(header, 80);
    f.write(reinterpret_cast<const char*>(&total_tris), 4);

    for (auto& entry : meshes.entries()) {
        auto& m = entry.mesh;
        for (size_t i = 0; i + 2 < m.indices.size(); i += 3) {
            Vec3 v0 = entry.transform.transform_point(m.vertices[m.indices[i]].position);
            Vec3 v1 = entry.transform.transform_point(m.vertices[m.indices[i+1]].position);
            Vec3 v2 = entry.transform.transform_point(m.vertices[m.indices[i+2]].position);
            Vec3 n = (v1 - v0).cross(v2 - v0).normalized();

            float data[12] = {n.x, n.y, n.z, v0.x, v0.y, v0.z,
                               v1.x, v1.y, v1.z, v2.x, v2.y, v2.z};
            f.write(reinterpret_cast<const char*>(data), 48);
            uint16_t attr = 0;
            f.write(reinterpret_cast<const char*>(&attr), 2);
        }
    }
    return f.good();
}

bool export_obj(const std::filesystem::path& path, const MeshGenerator& meshes) {
    std::ofstream f(path);
    if (!f.is_open()) return false;

    f << "# smidr OBJ export\n";
    uint32_t vert_offset = 1;

    for (size_t ei = 0; ei < meshes.entries().size(); ++ei) {
        auto& entry = meshes.entries()[ei];
        auto& m = entry.mesh;

        f << "o mesh_" << ei << "\n";

        for (auto& v : m.vertices) {
            Vec3 p = entry.transform.transform_point(v.position);
            Vec3 n = entry.transform.transform_normal(v.normal);
            f << "v " << p.x << " " << p.y << " " << p.z << "\n";
            f << "vn " << n.x << " " << n.y << " " << n.z << "\n";
        }

        for (size_t i = 0; i + 2 < m.indices.size(); i += 3) {
            uint32_t a = m.indices[i] + vert_offset;
            uint32_t b = m.indices[i+1] + vert_offset;
            uint32_t c = m.indices[i+2] + vert_offset;
            f << "f " << a << "//" << a << " "
                       << b << "//" << b << " "
                       << c << "//" << c << "\n";
        }

        vert_offset += static_cast<uint32_t>(m.vertices.size());
    }
    return f.good();
}

}  // namespace smidr
