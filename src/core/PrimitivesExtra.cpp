#include "core/PrimitivesExtra.h"

#include <cmath>

#include "io/json.hpp"

namespace smidr {

// ── CLine ──────────────────────────────────────────────────────

Mesh CLinePrimitive::generate_mesh(int detail) const {
    Mesh mesh;
    Vec3 axis_vec = end - start;
    float h = axis_vec.length();
    if (h < 1e-6f) return mesh;
    Vec3 axis = axis_vec * (1.f / h);

    Vec3 right = (std::abs(axis.y) < 0.9f ? axis.cross({0,1,0}) : axis.cross({1,0,0})).normalized();
    Vec3 fwd = axis.cross(right).normalized();

    float ri = outer_radius - wall_thickness;
    if (ri < 0.01f) ri = 0.01f;

    auto tube = [&](float r, float ndir) {
        uint32_t s = static_cast<uint32_t>(mesh.vertices.size());
        for (int i = 0; i <= detail; ++i) {
            float th = kTwoPi * static_cast<float>(i) / static_cast<float>(detail);
            Vec3 off = (right * std::cos(th) + fwd * std::sin(th)) * r;
            Vec3 n = off.normalized() * ndir;
            mesh.vertices.push_back({start + off, n});
            mesh.vertices.push_back({end + off, n});
        }
        for (int i = 0; i < detail; ++i) {
            uint32_t a = s + static_cast<uint32_t>(i * 2);
            if (ndir > 0)
                mesh.indices.insert(mesh.indices.end(), {a, a+1, a+2, a+2, a+1, a+3});
            else
                mesh.indices.insert(mesh.indices.end(), {a, a+2, a+1, a+1, a+2, a+3});
        }
    };
    tube(outer_radius, 1.f);
    tube(ri, -1.f);
    return mesh;
}

AABB CLinePrimitive::local_bounds() const {
    AABB bb;
    Vec3 r{outer_radius, outer_radius, outer_radius};
    bb.expand(start - r); bb.expand(start + r);
    bb.expand(end - r); bb.expand(end + r);
    return bb;
}
std::unique_ptr<Primitive> CLinePrimitive::clone() const { return std::make_unique<CLinePrimitive>(*this); }
void CLinePrimitive::to_json(nlohmann::json& j) const {
    j["type"] = "cline";
    j["start"] = {start.x, start.y, start.z};
    j["end"] = {end.x, end.y, end.z};
    j["outer_radius"] = outer_radius;
    j["wall_thickness"] = wall_thickness;
}

// ── Joint ──────────────────────────────────────────────────────

Mesh JointPrimitive::generate_mesh(int detail) const {
    Mesh mesh;
    int stacks = detail / 2, slices = detail;
    for (int i = 0; i <= stacks; ++i) {
        float phi = kPi * static_cast<float>(i) / static_cast<float>(stacks);
        float sp = std::sin(phi), cp = std::cos(phi);
        for (int j = 0; j <= slices; ++j) {
            float th = kTwoPi * static_cast<float>(j) / static_cast<float>(slices);
            Vec3 n{sp * std::cos(th), cp, sp * std::sin(th)};
            mesh.vertices.push_back({position + n * radius, n});
        }
    }
    for (int i = 0; i < stacks; ++i)
        for (int j = 0; j < slices; ++j) {
            uint32_t a = static_cast<uint32_t>(i * (slices + 1) + j);
            uint32_t b = a + static_cast<uint32_t>(slices + 1);
            mesh.indices.insert(mesh.indices.end(), {a, b, a+1, a+1, b, b+1});
        }
    return mesh;
}

AABB JointPrimitive::local_bounds() const {
    Vec3 r{radius, radius, radius};
    return {position - r, position + r};
}
std::unique_ptr<Primitive> JointPrimitive::clone() const { return std::make_unique<JointPrimitive>(*this); }
void JointPrimitive::to_json(nlohmann::json& j) const {
    j["type"] = "joint";
    j["position"] = {position.x, position.y, position.z};
    j["axis"] = {axis.x, axis.y, axis.z};
    j["radius"] = radius;
    j["dof"] = dof;
}

// ── Grip ───────────────────────────────────────────────────────

Mesh GripPrimitive::generate_mesh(int) const {
    Mesh mesh;
    float s = size * 0.5f;
    auto arm = [&](Vec3 axis, Vec3 n) {
        Vec3 right = (std::abs(axis.y) < 0.9f ? axis.cross({0,1,0}) : axis.cross({1,0,0})).normalized();
        Vec3 fwd = axis.cross(right).normalized();
        float t = size * 0.05f;
        Vec3 corners[8] = {
            position - right*t - fwd*t, position + axis*s - right*t - fwd*t,
            position + axis*s + right*t - fwd*t, position - right*t + fwd*t + right*0*0,
            position + right*t + fwd*t, position + axis*s + right*t + fwd*t,
            position + axis*s - right*t + fwd*t, position - right*t + fwd*t
        };
        auto base = static_cast<uint32_t>(mesh.vertices.size());
        for (auto& c : corners) mesh.vertices.push_back({c, n});
        mesh.indices.insert(mesh.indices.end(),
            {base+0, base+1, base+5, base+0, base+5, base+4});
    };
    arm({1,0,0}, {0,1,0});
    arm({-1,0,0}, {0,1,0});
    arm({0,1,0}, {1,0,0});
    arm({0,-1,0}, {1,0,0});
    arm({0,0,1}, {0,1,0});
    arm({0,0,-1}, {0,1,0});
    return mesh;
}

AABB GripPrimitive::local_bounds() const {
    Vec3 s{size, size, size};
    return {position - s, position + s};
}
std::unique_ptr<Primitive> GripPrimitive::clone() const { return std::make_unique<GripPrimitive>(*this); }
void GripPrimitive::to_json(nlohmann::json& j) const {
    j["type"] = "grip";
    j["position"] = {position.x, position.y, position.z};
    j["size"] = size;
}

// ── Datum ──────────────────────────────────────────────────────

Mesh DatumPrimitive::generate_mesh(int) const {
    Mesh mesh;
    if (kind == Plane) {
        Vec3 t1 = (std::abs(normal.y) < 0.9f ? normal.cross({0,1,0}) : normal.cross({1,0,0})).normalized();
        Vec3 t2 = normal.cross(t1).normalized();
        float s = size;
        Vec3 corners[4] = {
            origin + t1*s + t2*s, origin - t1*s + t2*s,
            origin - t1*s - t2*s, origin + t1*s - t2*s
        };
        auto base = static_cast<uint32_t>(mesh.vertices.size());
        for (auto& c : corners) mesh.vertices.push_back({c, normal});
        mesh.indices.insert(mesh.indices.end(), {base, base+1, base+2, base, base+2, base+3});
    } else if (kind == Line) {
        Vec3 dir = normal;
        Vec3 t = (std::abs(dir.y) < 0.9f ? dir.cross({0,1,0}) : dir.cross({1,0,0})).normalized() * 0.02f;
        auto base = static_cast<uint32_t>(mesh.vertices.size());
        Vec3 a = origin - dir * size, b = origin + dir * size;
        mesh.vertices.push_back({a + t, dir}); mesh.vertices.push_back({a - t, dir});
        mesh.vertices.push_back({b + t, dir}); mesh.vertices.push_back({b - t, dir});
        mesh.indices.insert(mesh.indices.end(), {base, base+1, base+2, base+1, base+3, base+2});
    } else {
        float s = 0.05f;
        Vec3 verts[8] = {
            origin + Vec3{-s,-s,-s}, origin + Vec3{s,-s,-s}, origin + Vec3{s,s,-s}, origin + Vec3{-s,s,-s},
            origin + Vec3{-s,-s,s}, origin + Vec3{s,-s,s}, origin + Vec3{s,s,s}, origin + Vec3{-s,s,s}
        };
        struct F { int i[4]; Vec3 n; };
        F faces[6] = {
            {{0,3,2,1},{0,0,-1}}, {{4,5,6,7},{0,0,1}},
            {{0,1,5,4},{0,-1,0}}, {{2,3,7,6},{0,1,0}},
            {{1,2,6,5},{1,0,0}},  {{0,4,7,3},{-1,0,0}},
        };
        for (auto& f : faces) {
            auto base = static_cast<uint32_t>(mesh.vertices.size());
            for (int i : f.i) mesh.vertices.push_back({verts[i], f.n});
            mesh.indices.insert(mesh.indices.end(), {base, base+1, base+2, base, base+2, base+3});
        }
    }
    return mesh;
}

AABB DatumPrimitive::local_bounds() const {
    Vec3 s{size, size, size};
    return {origin - s, origin + s};
}
std::unique_ptr<Primitive> DatumPrimitive::clone() const { return std::make_unique<DatumPrimitive>(*this); }
void DatumPrimitive::to_json(nlohmann::json& j) const {
    j["type"] = "datum";
    j["kind"] = static_cast<int>(kind);
    j["origin"] = {origin.x, origin.y, origin.z};
    j["normal"] = {normal.x, normal.y, normal.z};
    j["size"] = size;
    j["label"] = label;
}

// ── Submodel ───────────────────────────────────────────────────

Mesh SubmodelPrimitive::generate_mesh(int) const {
    Mesh mesh;
    Vec3 hx{size.x*0.5f, 0, 0}, hy{0, size.y*0.5f, 0}, hz{0, 0, size.z*0.5f};
    Vec3 corners[8] = {
        -hx-hy-hz, hx-hy-hz, hx+hy-hz, -hx+hy-hz,
        -hx-hy+hz, hx-hy+hz, hx+hy+hz, -hx+hy+hz
    };
    struct F { int i[4]; Vec3 n; };
    F faces[6] = {
        {{0,3,2,1},{0,0,-1}}, {{4,5,6,7},{0,0,1}},
        {{0,1,5,4},{0,-1,0}}, {{2,3,7,6},{0,1,0}},
        {{1,2,6,5},{1,0,0}},  {{0,4,7,3},{-1,0,0}},
    };
    for (auto& f : faces) {
        auto base = static_cast<uint32_t>(mesh.vertices.size());
        for (int i : f.i) mesh.vertices.push_back({corners[i], f.n});
        mesh.indices.insert(mesh.indices.end(), {base, base+1, base+2, base, base+2, base+3});
    }
    return mesh;
}

AABB SubmodelPrimitive::local_bounds() const {
    Vec3 h = size * 0.5f;
    return {-h, h};
}
std::unique_ptr<Primitive> SubmodelPrimitive::clone() const { return std::make_unique<SubmodelPrimitive>(*this); }
void SubmodelPrimitive::to_json(nlohmann::json& j) const {
    j["type"] = "submodel"; j["path"] = path;
    j["size"] = {size.x, size.y, size.z};
}

// ── Script ─────────────────────────────────────────────────────

Mesh ScriptPrimitive::generate_mesh(int) const {
    Mesh mesh;
    Vec3 corners[8] = {
        bounds_min, {bounds_max.x, bounds_min.y, bounds_min.z},
        {bounds_max.x, bounds_max.y, bounds_min.z}, {bounds_min.x, bounds_max.y, bounds_min.z},
        {bounds_min.x, bounds_min.y, bounds_max.z}, {bounds_max.x, bounds_min.y, bounds_max.z},
        bounds_max, {bounds_min.x, bounds_max.y, bounds_max.z}
    };
    struct F { int i[4]; Vec3 n; };
    F faces[6] = {
        {{0,3,2,1},{0,0,-1}}, {{4,5,6,7},{0,0,1}},
        {{0,1,5,4},{0,-1,0}}, {{2,3,7,6},{0,1,0}},
        {{1,2,6,5},{1,0,0}},  {{0,4,7,3},{-1,0,0}},
    };
    for (auto& f : faces) {
        auto base = static_cast<uint32_t>(mesh.vertices.size());
        for (int i : f.i) mesh.vertices.push_back({corners[i], f.n});
        mesh.indices.insert(mesh.indices.end(), {base, base+1, base+2, base, base+2, base+3});
    }
    return mesh;
}

AABB ScriptPrimitive::local_bounds() const { return {bounds_min, bounds_max}; }
std::unique_ptr<Primitive> ScriptPrimitive::clone() const { return std::make_unique<ScriptPrimitive>(*this); }
void ScriptPrimitive::to_json(nlohmann::json& j) const {
    j["type"] = "script"; j["script_text"] = script_text;
    j["bounds_min"] = {bounds_min.x, bounds_min.y, bounds_min.z};
    j["bounds_max"] = {bounds_max.x, bounds_max.y, bounds_max.z};
}

// ── EBM ────────────────────────────────────────────────────────

EBMPrimitive::EBMPrimitive() {
    mask.resize(static_cast<size_t>(width * height));
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            mask[static_cast<size_t>(y * width + x)] = ((x + y) & 1) ? 255 : 0;
}

Mesh EBMPrimitive::generate_mesh(int) const {
    Mesh mesh;
    float cs = cell_size, h = extrude_height;
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            if (mask[static_cast<size_t>(y * width + x)] < 128) continue;
            Vec3 mn{static_cast<float>(x) * cs, 0, static_cast<float>(y) * cs};
            Vec3 mx = mn + Vec3{cs, h, cs};
            Vec3 corners[8] = {
                mn, {mx.x, mn.y, mn.z}, {mx.x, mn.y, mx.z}, {mn.x, mn.y, mx.z},
                {mn.x, mx.y, mn.z}, {mx.x, mx.y, mn.z}, mx, {mn.x, mx.y, mx.z}
            };
            struct F { int i[4]; Vec3 n; };
            F faces[6] = {
                {{0,1,2,3},{0,-1,0}}, {{4,7,6,5},{0,1,0}},
                {{0,4,5,1},{0,0,-1}}, {{2,6,7,3},{0,0,1}},
                {{1,5,6,2},{1,0,0}}, {{0,3,7,4},{-1,0,0}}
            };
            for (auto& f : faces) {
                auto base = static_cast<uint32_t>(mesh.vertices.size());
                for (int i : f.i) mesh.vertices.push_back({corners[i], f.n});
                mesh.indices.insert(mesh.indices.end(), {base, base+1, base+2, base, base+2, base+3});
            }
        }
    return mesh;
}

AABB EBMPrimitive::local_bounds() const {
    return {{0, 0, 0}, {static_cast<float>(width) * cell_size, extrude_height, static_cast<float>(height) * cell_size}};
}
std::unique_ptr<Primitive> EBMPrimitive::clone() const { return std::make_unique<EBMPrimitive>(*this); }
void EBMPrimitive::to_json(nlohmann::json& j) const {
    j["type"] = "ebm"; j["width"] = width; j["height"] = height;
    j["cell_size"] = cell_size; j["extrude_height"] = extrude_height;
}

// ── VOL ────────────────────────────────────────────────────────

VOLPrimitive::VOLPrimitive() {
    voxels.resize(static_cast<size_t>(dim_x * dim_y * dim_z));
    for (int z = 0; z < dim_z; ++z)
        for (int y = 0; y < dim_y; ++y)
            for (int x = 0; x < dim_x; ++x) {
                float dx = static_cast<float>(x) - dim_x*0.5f;
                float dy = static_cast<float>(y) - dim_y*0.5f;
                float dz = static_cast<float>(z) - dim_z*0.5f;
                float d = std::sqrt(dx*dx+dy*dy+dz*dz);
                voxels[static_cast<size_t>((z*dim_y+y)*dim_x+x)] = d < dim_x*0.4f ? 255 : 0;
            }
}

Mesh VOLPrimitive::generate_mesh(int) const {
    Mesh mesh;
    float cs = cell_size;
    auto idx = [&](int x, int y, int z) {
        return static_cast<size_t>((z*dim_y+y)*dim_x+x);
    };
    auto on = [&](int x, int y, int z) {
        if (x<0||y<0||z<0||x>=dim_x||y>=dim_y||z>=dim_z) return false;
        return voxels[idx(x,y,z)] >= threshold;
    };
    for (int z = 0; z < dim_z; ++z)
        for (int y = 0; y < dim_y; ++y)
            for (int x = 0; x < dim_x; ++x) {
                if (!on(x,y,z)) continue;
                Vec3 mn{static_cast<float>(x)*cs, static_cast<float>(y)*cs, static_cast<float>(z)*cs};
                Vec3 mx = mn + Vec3{cs, cs, cs};
                struct Face { int dx, dy, dz; Vec3 n; int v[4][3]; };
                Face faces[6] = {
                    {1,0,0,{1,0,0},{{1,0,0},{1,1,0},{1,1,1},{1,0,1}}},
                    {-1,0,0,{-1,0,0},{{0,0,1},{0,1,1},{0,1,0},{0,0,0}}},
                    {0,1,0,{0,1,0},{{0,1,0},{0,1,1},{1,1,1},{1,1,0}}},
                    {0,-1,0,{0,-1,0},{{0,0,0},{1,0,0},{1,0,1},{0,0,1}}},
                    {0,0,1,{0,0,1},{{0,0,1},{1,0,1},{1,1,1},{0,1,1}}},
                    {0,0,-1,{0,0,-1},{{0,1,0},{1,1,0},{1,0,0},{0,0,0}}}
                };
                for (auto& f : faces) {
                    if (on(x+f.dx, y+f.dy, z+f.dz)) continue;
                    auto base = static_cast<uint32_t>(mesh.vertices.size());
                    for (auto& v : f.v) {
                        mesh.vertices.push_back({{mn.x + static_cast<float>(v[0])*cs,
                                                   mn.y + static_cast<float>(v[1])*cs,
                                                   mn.z + static_cast<float>(v[2])*cs}, f.n});
                    }
                    mesh.indices.insert(mesh.indices.end(), {base, base+1, base+2, base, base+2, base+3});
                }
            }
    return mesh;
}

AABB VOLPrimitive::local_bounds() const {
    return {{0,0,0}, {static_cast<float>(dim_x)*cell_size,
                      static_cast<float>(dim_y)*cell_size,
                      static_cast<float>(dim_z)*cell_size}};
}
std::unique_ptr<Primitive> VOLPrimitive::clone() const { return std::make_unique<VOLPrimitive>(*this); }
void VOLPrimitive::to_json(nlohmann::json& j) const {
    j["type"] = "vol";
    j["dim_x"] = dim_x; j["dim_y"] = dim_y; j["dim_z"] = dim_z;
    j["cell_size"] = cell_size; j["threshold"] = threshold;
}

// ── HF ─────────────────────────────────────────────────────────

HFPrimitive::HFPrimitive() {
    heights.resize(static_cast<size_t>(width * depth));
    for (int y = 0; y < depth; ++y)
        for (int x = 0; x < width; ++x)
            heights[static_cast<size_t>(y * width + x)] =
                std::sin(static_cast<float>(x) * 0.4f) * std::cos(static_cast<float>(y) * 0.4f) * 0.5f;
}

Mesh HFPrimitive::generate_mesh(int) const {
    Mesh mesh;
    for (int y = 0; y < depth; ++y)
        for (int x = 0; x < width; ++x) {
            Vec3 p{static_cast<float>(x) * cell_size,
                   heights[static_cast<size_t>(y * width + x)],
                   static_cast<float>(y) * cell_size};
            mesh.vertices.push_back({p, {0, 1, 0}});
        }
    for (int y = 0; y < depth - 1; ++y)
        for (int x = 0; x < width - 1; ++x) {
            uint32_t a = static_cast<uint32_t>(y * width + x);
            uint32_t b = a + static_cast<uint32_t>(width);
            mesh.indices.insert(mesh.indices.end(), {a, b, a+1, a+1, b, b+1});
        }
    return mesh;
}

AABB HFPrimitive::local_bounds() const {
    float w = static_cast<float>(width - 1) * cell_size;
    float d = static_cast<float>(depth - 1) * cell_size;
    float hmin = 1e30f, hmax = -1e30f;
    for (float h : heights) { hmin = std::min(hmin, h); hmax = std::max(hmax, h); }
    return {{0, hmin, 0}, {w, hmax, d}};
}
std::unique_ptr<Primitive> HFPrimitive::clone() const { return std::make_unique<HFPrimitive>(*this); }
void HFPrimitive::to_json(nlohmann::json& j) const {
    j["type"] = "hf"; j["width"] = width; j["depth"] = depth;
    j["cell_size"] = cell_size;
}

// ── ARS ────────────────────────────────────────────────────────

ARSPrimitive::ARSPrimitive() {
    grid.resize(static_cast<size_t>(ncurves * points_per_curve));
    for (int i = 0; i < ncurves; ++i) {
        float v = static_cast<float>(i) / static_cast<float>(ncurves - 1);
        float y = v * 2.f - 1.f;
        for (int j = 0; j < points_per_curve; ++j) {
            float u = static_cast<float>(j) / static_cast<float>(points_per_curve - 1);
            float angle = kTwoPi * u;
            float r = 1.f - std::abs(y);
            grid[static_cast<size_t>(i * points_per_curve + j)] =
                {r * std::cos(angle), y, r * std::sin(angle)};
        }
    }
}

Mesh ARSPrimitive::generate_mesh(int) const {
    Mesh mesh;
    for (auto& v : grid) {
        mesh.vertices.push_back({v, v.normalized()});
    }
    for (int i = 0; i < ncurves - 1; ++i)
        for (int j = 0; j < points_per_curve - 1; ++j) {
            uint32_t a = static_cast<uint32_t>(i * points_per_curve + j);
            uint32_t b = a + static_cast<uint32_t>(points_per_curve);
            mesh.indices.insert(mesh.indices.end(), {a, b, a+1, a+1, b, b+1});
        }
    return mesh;
}

AABB ARSPrimitive::local_bounds() const {
    AABB bb;
    for (auto& v : grid) bb.expand(v);
    return bb;
}
std::unique_ptr<Primitive> ARSPrimitive::clone() const { return std::make_unique<ARSPrimitive>(*this); }
void ARSPrimitive::to_json(nlohmann::json& j) const {
    j["type"] = "ars"; j["ncurves"] = ncurves;
    j["points_per_curve"] = points_per_curve;
}

}  // namespace smidr
