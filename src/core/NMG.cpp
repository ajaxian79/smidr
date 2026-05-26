#include "core/NMG.h"
#include "core/CSGEval.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace smidr {

uint32_t NMGShell::add_vertex(Vec3 pos) {
    uint32_t id = static_cast<uint32_t>(vertices.size());
    vertices.push_back({pos, id});
    return id;
}

uint32_t NMGShell::add_edge(uint32_t v0, uint32_t v1) {
    uint32_t id = static_cast<uint32_t>(edges.size());
    edges.push_back({v0, v1, UINT32_MAX, UINT32_MAX, id});
    return id;
}

uint32_t NMGShell::add_face(const std::vector<uint32_t>& vert_ids) {
    if (vert_ids.size() < 3) return UINT32_MAX;

    NMGFace face;
    face.id = static_cast<uint32_t>(faces.size());

    NMGLoop loop;
    for (size_t i = 0; i < vert_ids.size(); ++i) {
        uint32_t v0 = vert_ids[i];
        uint32_t v1 = vert_ids[(i + 1) % vert_ids.size()];
        uint32_t eid = add_edge(v0, v1);
        loop.edge_ids.push_back(eid);

        auto& e = edges[eid];
        if (e.left_face == UINT32_MAX) e.left_face = face.id;
        else e.right_face = face.id;
    }
    face.loops.push_back(loop);

    Vec3 a = vertices[vert_ids[0]].position;
    Vec3 b = vertices[vert_ids[1]].position;
    Vec3 c = vertices[vert_ids[2]].position;
    face.normal = (b - a).cross(c - a).normalized();

    faces.push_back(face);
    return face.id;
}

bool NMGShell::is_closed() const {
    for (auto& e : edges) {
        if (e.left_face == UINT32_MAX || e.right_face == UINT32_MAX)
            return false;
    }
    return !edges.empty();
}

bool NMGShell::is_manifold() const {
    struct EdgeKey {
        uint32_t lo, hi;
        bool operator==(const EdgeKey& o) const { return lo == o.lo && hi == o.hi; }
    };
    struct Hash {
        size_t operator()(const EdgeKey& k) const {
            return std::hash<uint64_t>()(static_cast<uint64_t>(k.lo) << 32 | k.hi);
        }
    };

    std::unordered_map<EdgeKey, int, Hash> edge_count;
    for (auto& e : edges) {
        uint32_t lo = std::min(e.v0, e.v1), hi = std::max(e.v0, e.v1);
        edge_count[{lo, hi}]++;
    }
    for (auto& [key, count] : edge_count) {
        if (count > 2) return false;
    }
    return true;
}

int NMGShell::euler_characteristic() const {
    int V = static_cast<int>(vertices.size());
    int E = static_cast<int>(edges.size());
    int F = static_cast<int>(faces.size());
    return V - E + F;
}

Mesh NMGShell::to_mesh() const {
    Mesh mesh;
    for (auto& face : faces) {
        for (auto& loop : face.loops) {
            if (loop.edge_ids.size() < 3) continue;

            std::vector<uint32_t> verts;
            for (auto eid : loop.edge_ids) {
                if (eid < edges.size()) verts.push_back(edges[eid].v0);
            }

            for (size_t i = 1; i + 1 < verts.size(); ++i) {
                auto base = static_cast<uint32_t>(mesh.vertices.size());
                mesh.vertices.push_back({vertices[verts[0]].position, face.normal});
                mesh.vertices.push_back({vertices[verts[i]].position, face.normal});
                mesh.vertices.push_back({vertices[verts[i+1]].position, face.normal});
                mesh.indices.insert(mesh.indices.end(), {base, base+1, base+2});
            }
        }
    }
    return mesh;
}

void NMGShell::merge_vertices(float tolerance) {
    float tol2 = tolerance * tolerance;
    std::vector<uint32_t> remap(vertices.size());
    for (size_t i = 0; i < vertices.size(); ++i) remap[i] = static_cast<uint32_t>(i);

    for (size_t i = 0; i < vertices.size(); ++i) {
        if (remap[i] != static_cast<uint32_t>(i)) continue;
        for (size_t j = i + 1; j < vertices.size(); ++j) {
            Vec3 d = vertices[j].position - vertices[i].position;
            if (d.dot(d) < tol2) remap[j] = static_cast<uint32_t>(i);
        }
    }

    for (auto& e : edges) {
        e.v0 = remap[e.v0];
        e.v1 = remap[e.v1];
    }
}

void NMGShell::flip_face(uint32_t face_id) {
    if (face_id >= faces.size()) return;
    faces[face_id].normal = -faces[face_id].normal;
    for (auto& loop : faces[face_id].loops) {
        std::reverse(loop.edge_ids.begin(), loop.edge_ids.end());
    }
}

void NMGShell::compute_face_normals() {
    for (auto& face : faces) {
        if (face.loops.empty() || face.loops[0].edge_ids.size() < 3) continue;
        auto& loop = face.loops[0];
        uint32_t e0 = loop.edge_ids[0], e1 = loop.edge_ids[1];
        if (e0 >= edges.size() || e1 >= edges.size()) continue;
        Vec3 a = vertices[edges[e0].v0].position;
        Vec3 b = vertices[edges[e0].v1].position;
        Vec3 c = vertices[edges[e1].v1].position;
        face.normal = (b - a).cross(c - a).normalized();
    }
}

bool NMGShell::validate() const {
    for (auto& e : edges) {
        if (e.v0 >= vertices.size() || e.v1 >= vertices.size()) return false;
    }
    for (auto& f : faces) {
        for (auto& l : f.loops) {
            for (auto eid : l.edge_ids) {
                if (eid >= edges.size()) return false;
            }
        }
    }
    return true;
}

NMGShell NMGShell::from_mesh(const Mesh& mesh, float merge_tol) {
    NMGShell shell;
    for (auto& v : mesh.vertices) shell.add_vertex(v.position);
    shell.merge_vertices(merge_tol);

    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        shell.add_face({mesh.indices[i], mesh.indices[i+1], mesh.indices[i+2]});
    }
    shell.compute_face_normals();
    return shell;
}

NMGShell NMGShell::make_box(Vec3 mn, Vec3 mx) {
    NMGShell s;
    s.add_vertex(mn);
    s.add_vertex({mx.x, mn.y, mn.z});
    s.add_vertex({mx.x, mx.y, mn.z});
    s.add_vertex({mn.x, mx.y, mn.z});
    s.add_vertex({mn.x, mn.y, mx.z});
    s.add_vertex({mx.x, mn.y, mx.z});
    s.add_vertex(mx);
    s.add_vertex({mn.x, mx.y, mx.z});

    s.add_face({0, 3, 2, 1});
    s.add_face({4, 5, 6, 7});
    s.add_face({0, 1, 5, 4});
    s.add_face({2, 3, 7, 6});
    s.add_face({1, 2, 6, 5});
    s.add_face({0, 4, 7, 3});
    return s;
}

NMGShell NMGShell::make_tetrahedron(Vec3 a, Vec3 b, Vec3 c, Vec3 d) {
    NMGShell s;
    s.add_vertex(a); s.add_vertex(b); s.add_vertex(c); s.add_vertex(d);
    s.add_face({0, 1, 2});
    s.add_face({0, 1, 3});
    s.add_face({1, 2, 3});
    s.add_face({2, 0, 3});
    return s;
}

Mesh nmg_boolean_union(const NMGShell& a, const NMGShell& b) {
    Mesh ma = a.to_mesh(), mb = b.to_mesh();
    return csg_union(ma, mb);
}

Mesh nmg_boolean_intersection(const NMGShell& a, const NMGShell& b) {
    Mesh ma = a.to_mesh(), mb = b.to_mesh();
    return csg_intersection(ma, mb);
}

Mesh nmg_boolean_difference(const NMGShell& a, const NMGShell& b) {
    Mesh ma = a.to_mesh(), mb = b.to_mesh();
    return csg_difference(ma, mb);
}

}  // namespace smidr
