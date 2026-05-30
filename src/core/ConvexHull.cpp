#include "core/ConvexHull.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <utility>

namespace smidr {

struct Face { int v[3]; Vec3 normal; float d; };

static Vec3 normal_of(Vec3 a, Vec3 b, Vec3 c) {
    return (b - a).cross(c - a).normalized();
}

Mesh convex_hull_3d(const std::vector<Vec3>& pts) {
    Mesh mesh;
    if (pts.size() < 4) return mesh;

    int idx_max_x = 0, idx_min_x = 0;
    for (size_t i = 1; i < pts.size(); ++i) {
        if (pts[i].x > pts[idx_max_x].x) idx_max_x = static_cast<int>(i);
        if (pts[i].x < pts[idx_min_x].x) idx_min_x = static_cast<int>(i);
    }
    int idx_max_y = idx_max_x;
    float best_dist = 0;
    Vec3 axis = pts[idx_max_x] - pts[idx_min_x];
    for (size_t i = 0; i < pts.size(); ++i) {
        Vec3 d = pts[i] - pts[idx_min_x];
        Vec3 perp = d - axis.normalized() * axis.normalized().dot(d);
        if (perp.length() > best_dist) {
            best_dist = perp.length();
            idx_max_y = static_cast<int>(i);
        }
    }
    int idx_max_z = idx_max_y;
    best_dist = 0;
    Vec3 n_seed = normal_of(pts[idx_min_x], pts[idx_max_x], pts[idx_max_y]);
    for (size_t i = 0; i < pts.size(); ++i) {
        float d = std::abs((pts[i] - pts[idx_min_x]).dot(n_seed));
        if (d > best_dist) {
            best_dist = d;
            idx_max_z = static_cast<int>(i);
        }
    }

    if (idx_min_x == idx_max_x || idx_max_y == idx_min_x || idx_max_y == idx_max_x
        || idx_max_z == idx_min_x || idx_max_z == idx_max_x || idx_max_z == idx_max_y) {
        return mesh;
    }

    std::vector<int> tet = {idx_min_x, idx_max_x, idx_max_y, idx_max_z};
    Vec3 c = (pts[tet[0]] + pts[tet[1]] + pts[tet[2]] + pts[tet[3]]) * 0.25f;
    std::vector<Face> faces;
    auto add_face = [&](int a, int b, int d) {
        Face f;
        f.v[0] = a; f.v[1] = b; f.v[2] = d;
        f.normal = normal_of(pts[a], pts[b], pts[d]);
        if (f.normal.dot(c - pts[a]) > 0) {
            std::swap(f.v[1], f.v[2]);
            f.normal = f.normal * -1.f;
        }
        f.d = f.normal.dot(pts[a]);
        faces.push_back(f);
    };
    add_face(tet[0], tet[1], tet[2]);
    add_face(tet[0], tet[1], tet[3]);
    add_face(tet[0], tet[2], tet[3]);
    add_face(tet[1], tet[2], tet[3]);

    std::set<int> in_hull(tet.begin(), tet.end());

    for (size_t pi = 0; pi < pts.size(); ++pi) {
        if (in_hull.count(static_cast<int>(pi))) continue;
        std::vector<int> visible;
        for (size_t fi = 0; fi < faces.size(); ++fi) {
            if (faces[fi].normal.dot(pts[pi]) - faces[fi].d > 1e-6f) {
                visible.push_back(static_cast<int>(fi));
            }
        }
        if (visible.empty()) continue;

        std::set<std::pair<int,int>> horizon;
        auto edge_pair = [](int a, int b) {
            return std::make_pair(std::min(a,b), std::max(a,b));
        };
        for (int vi : visible) {
            for (int e = 0; e < 3; ++e) {
                int a = faces[vi].v[e], b = faces[vi].v[(e+1)%3];
                auto k = edge_pair(a, b);
                if (horizon.count(k)) horizon.erase(k);
                else horizon.insert(k);
            }
        }

        std::sort(visible.begin(), visible.end(), std::greater<int>());
        for (int vi : visible) faces.erase(faces.begin() + vi);

        for (auto& [a, b] : horizon) {
            add_face(a, b, static_cast<int>(pi));
        }
        in_hull.insert(static_cast<int>(pi));
    }

    for (auto& f : faces) {
        uint32_t base = static_cast<uint32_t>(mesh.vertices.size());
        Vec3 n = f.normal;
        mesh.vertices.push_back({pts[f.v[0]], n});
        mesh.vertices.push_back({pts[f.v[1]], n});
        mesh.vertices.push_back({pts[f.v[2]], n});
        mesh.indices.insert(mesh.indices.end(), {base, base+1, base+2});
    }
    return mesh;
}

static bool in_circle(Vec3 a, Vec3 b, Vec3 c, Vec3 p) {
    float ax = a.x - p.x, ay = a.y - p.y;
    float bx = b.x - p.x, by = b.y - p.y;
    float cx = c.x - p.x, cy = c.y - p.y;
    float det = (ax * (by * (cx*cx + cy*cy) - cy * (bx*bx + by*by)))
              - (ay * (bx * (cx*cx + cy*cy) - cx * (bx*bx + by*by)))
              + ((ax*ax + ay*ay) * (bx * cy - by * cx));
    return det > 0;
}

DelaunayResult delaunay_2d(const std::vector<Vec3>& pts) {
    DelaunayResult res;
    if (pts.size() < 3) return res;

    res.vertices = pts;
    float min_x = 1e30f, min_y = 1e30f, max_x = -1e30f, max_y = -1e30f;
    for (auto& p : pts) {
        min_x = std::min(min_x, p.x); max_x = std::max(max_x, p.x);
        min_y = std::min(min_y, p.y); max_y = std::max(max_y, p.y);
    }
    float dx = max_x - min_x + 2.f, dy = max_y - min_y + 2.f;
    Vec3 super_a{min_x - dx * 2, min_y - dy, 0};
    Vec3 super_b{max_x + dx * 2, min_y - dy, 0};
    Vec3 super_c{(min_x + max_x) * 0.5f, max_y + dy * 3, 0};

    uint32_t sa = static_cast<uint32_t>(res.vertices.size()); res.vertices.push_back(super_a);
    uint32_t sb = static_cast<uint32_t>(res.vertices.size()); res.vertices.push_back(super_b);
    uint32_t sc = static_cast<uint32_t>(res.vertices.size()); res.vertices.push_back(super_c);

    std::vector<std::array<uint32_t, 3>> tris;
    tris.push_back({sa, sb, sc});

    for (uint32_t pi = 0; pi < pts.size(); ++pi) {
        std::vector<std::pair<uint32_t,uint32_t>> bad_edges;
        std::vector<std::array<uint32_t,3>> good;
        for (auto& t : tris) {
            if (in_circle(res.vertices[t[0]], res.vertices[t[1]], res.vertices[t[2]], res.vertices[pi])) {
                bad_edges.push_back({t[0], t[1]});
                bad_edges.push_back({t[1], t[2]});
                bad_edges.push_back({t[2], t[0]});
            } else {
                good.push_back(t);
            }
        }
        tris = good;

        std::vector<std::pair<uint32_t,uint32_t>> boundary;
        for (size_t i = 0; i < bad_edges.size(); ++i) {
            bool shared = false;
            for (size_t j = 0; j < bad_edges.size(); ++j) {
                if (i == j) continue;
                if ((bad_edges[i].first == bad_edges[j].first && bad_edges[i].second == bad_edges[j].second)
                    || (bad_edges[i].first == bad_edges[j].second && bad_edges[i].second == bad_edges[j].first)) {
                    shared = true; break;
                }
            }
            if (!shared) boundary.push_back(bad_edges[i]);
        }
        for (auto& e : boundary) {
            tris.push_back({e.first, e.second, pi});
        }
    }

    for (auto& t : tris) {
        if (t[0] == sa || t[0] == sb || t[0] == sc ||
            t[1] == sa || t[1] == sb || t[1] == sc ||
            t[2] == sa || t[2] == sb || t[2] == sc) continue;
        res.triangles.push_back(t[0]);
        res.triangles.push_back(t[1]);
        res.triangles.push_back(t[2]);
    }
    res.vertices.resize(pts.size());
    return res;
}

std::vector<VoronoiCell> voronoi_2d(const std::vector<Vec3>& pts,
                                       float min_x, float max_x,
                                       float min_y, float max_y) {
    std::vector<VoronoiCell> cells;
    for (size_t i = 0; i < pts.size(); ++i) {
        VoronoiCell cell;
        cell.site = pts[i];
        cell.vertices = {{min_x, min_y, 0}, {max_x, min_y, 0},
                          {max_x, max_y, 0}, {min_x, max_y, 0}};
        for (size_t j = 0; j < pts.size(); ++j) {
            if (i == j) continue;
            Vec3 mid = (pts[i] + pts[j]) * 0.5f;
            Vec3 normal = (pts[j] - pts[i]).normalized();
            std::vector<Vec3> clipped;
            for (size_t k = 0; k < cell.vertices.size(); ++k) {
                Vec3 a = cell.vertices[k];
                Vec3 b = cell.vertices[(k+1) % cell.vertices.size()];
                float da = (a - mid).dot(normal);
                float db = (b - mid).dot(normal);
                if (da <= 0) clipped.push_back(a);
                if ((da > 0) != (db > 0)) {
                    float t = da / (da - db);
                    clipped.push_back(a + (b - a) * t);
                }
            }
            cell.vertices = clipped;
        }
        if (cell.vertices.size() >= 3) cells.push_back(cell);
    }
    return cells;
}

}  // namespace smidr
