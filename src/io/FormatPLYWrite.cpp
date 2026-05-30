#include "io/MeshExport.h"

#include <fstream>

namespace smidr {

bool export_ply(const std::filesystem::path& path, const MeshGenerator& meshes) {
    std::ofstream f(path);
    if (!f.is_open()) return false;

    size_t total_verts = 0, total_faces = 0;
    for (auto& e : meshes.entries()) {
        total_verts += e.mesh.vertices.size();
        total_faces += e.mesh.indices.size() / 3;
    }

    f << "ply\nformat ascii 1.0\ncomment smidr PLY export\n";
    f << "element vertex " << total_verts << "\n";
    f << "property float x\nproperty float y\nproperty float z\n";
    f << "property float nx\nproperty float ny\nproperty float nz\n";
    f << "element face " << total_faces << "\n";
    f << "property list uchar uint vertex_indices\n";
    f << "end_header\n";

    for (auto& entry : meshes.entries()) {
        for (auto& v : entry.mesh.vertices) {
            Vec3 p = entry.transform.transform_point(v.position);
            Vec3 n = entry.transform.transform_normal(v.normal);
            f << p.x << " " << p.y << " " << p.z << " "
              << n.x << " " << n.y << " " << n.z << "\n";
        }
    }

    uint32_t offset = 0;
    for (auto& entry : meshes.entries()) {
        auto& m = entry.mesh;
        for (size_t i = 0; i + 2 < m.indices.size(); i += 3) {
            f << "3 " << (m.indices[i] + offset) << " "
              << (m.indices[i+1] + offset) << " " << (m.indices[i+2] + offset) << "\n";
        }
        offset += static_cast<uint32_t>(m.vertices.size());
    }
    return f.good();
}

bool export_off(const std::filesystem::path& path, const MeshGenerator& meshes) {
    std::ofstream f(path);
    if (!f.is_open()) return false;

    size_t total_verts = 0, total_faces = 0;
    for (auto& e : meshes.entries()) {
        total_verts += e.mesh.vertices.size();
        total_faces += e.mesh.indices.size() / 3;
    }

    f << "OFF\n" << total_verts << " " << total_faces << " 0\n";

    for (auto& entry : meshes.entries()) {
        for (auto& v : entry.mesh.vertices) {
            Vec3 p = entry.transform.transform_point(v.position);
            f << p.x << " " << p.y << " " << p.z << "\n";
        }
    }

    uint32_t offset = 0;
    for (auto& entry : meshes.entries()) {
        auto& m = entry.mesh;
        for (size_t i = 0; i + 2 < m.indices.size(); i += 3) {
            f << "3 " << (m.indices[i] + offset) << " "
              << (m.indices[i+1] + offset) << " " << (m.indices[i+2] + offset) << "\n";
        }
        offset += static_cast<uint32_t>(m.vertices.size());
    }
    return f.good();
}

}  // namespace smidr
