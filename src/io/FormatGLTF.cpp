#include "io/FormatImport.h"
#include "io/MeshExport.h"
#include "io/json.hpp"
#include "core/PrimitivesAdvanced.h"

#include <cstdint>
#include <cstring>
#include <fstream>

namespace smidr {

bool export_gltf(const std::filesystem::path& path, const MeshGenerator& meshes) {
    std::vector<float> positions, normals;
    std::vector<uint32_t> indices;
    std::vector<std::pair<size_t, size_t>> mesh_ranges;

    for (auto& entry : meshes.entries()) {
        auto& m = entry.mesh;
        size_t idx_start = indices.size();
        uint32_t vert_offset = static_cast<uint32_t>(positions.size() / 3);
        for (auto& v : m.vertices) {
            Vec3 p = entry.transform.transform_point(v.position);
            Vec3 n = entry.transform.transform_normal(v.normal);
            positions.push_back(p.x); positions.push_back(p.y); positions.push_back(p.z);
            normals.push_back(n.x); normals.push_back(n.y); normals.push_back(n.z);
        }
        for (auto idx : m.indices) indices.push_back(idx + vert_offset);
        mesh_ranges.push_back({idx_start, indices.size()});
    }

    std::filesystem::path bin_path = path;
    bin_path.replace_extension(".bin");
    std::ofstream bin(bin_path, std::ios::binary);
    if (!bin.is_open()) return false;

    size_t pos_offset = 0;
    size_t pos_bytes = positions.size() * 4;
    bin.write(reinterpret_cast<const char*>(positions.data()), static_cast<std::streamsize>(pos_bytes));

    size_t norm_offset = pos_offset + pos_bytes;
    size_t norm_bytes = normals.size() * 4;
    bin.write(reinterpret_cast<const char*>(normals.data()), static_cast<std::streamsize>(norm_bytes));

    size_t idx_offset = norm_offset + norm_bytes;
    size_t idx_bytes = indices.size() * 4;
    bin.write(reinterpret_cast<const char*>(indices.data()), static_cast<std::streamsize>(idx_bytes));
    bin.close();

    nlohmann::json gltf;
    gltf["asset"] = {{"version", "2.0"}, {"generator", "smidr"}};
    gltf["scene"] = 0;
    gltf["scenes"] = {{{"nodes", nlohmann::json::array()}}};

    nlohmann::json& nodes = gltf["nodes"]; nodes = nlohmann::json::array();
    nlohmann::json& meshes_arr = gltf["meshes"]; meshes_arr = nlohmann::json::array();

    for (size_t i = 0; i < mesh_ranges.size(); ++i) {
        nodes.push_back({{"mesh", i}});
        gltf["scenes"][0]["nodes"].push_back(i);

        nlohmann::json mesh;
        mesh["primitives"] = nlohmann::json::array();
        nlohmann::json prim;
        prim["attributes"] = {{"POSITION", 0}, {"NORMAL", 1}};
        prim["indices"] = 2 + static_cast<int>(i);
        prim["mode"] = 4;
        mesh["primitives"].push_back(prim);
        meshes_arr.push_back(mesh);
    }

    gltf["buffers"] = {{{"uri", bin_path.filename().string()},
                        {"byteLength", static_cast<size_t>(pos_bytes + norm_bytes + idx_bytes)}}};

    nlohmann::json& bv = gltf["bufferViews"]; bv = nlohmann::json::array();
    bv.push_back({{"buffer", 0}, {"byteOffset", pos_offset}, {"byteLength", pos_bytes}, {"target", 34962}});
    bv.push_back({{"buffer", 0}, {"byteOffset", norm_offset}, {"byteLength", norm_bytes}, {"target", 34962}});

    nlohmann::json& acc = gltf["accessors"]; acc = nlohmann::json::array();
    acc.push_back({{"bufferView", 0}, {"componentType", 5126}, {"count", positions.size()/3}, {"type", "VEC3"}});
    acc.push_back({{"bufferView", 1}, {"componentType", 5126}, {"count", normals.size()/3}, {"type", "VEC3"}});

    for (size_t i = 0; i < mesh_ranges.size(); ++i) {
        size_t start_b = idx_offset + mesh_ranges[i].first * 4;
        size_t count = mesh_ranges[i].second - mesh_ranges[i].first;
        bv.push_back({{"buffer", 0}, {"byteOffset", start_b}, {"byteLength", count * 4}, {"target", 34963}});
        acc.push_back({{"bufferView", 2 + static_cast<int>(i)}, {"componentType", 5125},
                       {"count", count}, {"type", "SCALAR"}});
    }

    std::ofstream f(path);
    if (!f.is_open()) return false;
    f << gltf.dump(2);
    return f.good();
}

}  // namespace smidr
