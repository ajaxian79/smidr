#include "core/CSGEval.h"
#include "core/RayCast.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace smidr {

static bool point_inside_mesh(Vec3 p, const Mesh& mesh) {
    Ray ray{p, {0.3713f, 0.7427f, 0.5570f}};
    int hits = 0;
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        Vec3 v0 = mesh.vertices[mesh.indices[i]].position;
        Vec3 v1 = mesh.vertices[mesh.indices[i + 1]].position;
        Vec3 v2 = mesh.vertices[mesh.indices[i + 2]].position;
        if (ray_triangle(ray, v0, v1, v2) > 0.f) ++hits;
    }
    return (hits % 2) == 1;
}

static Vec3 triangle_normal(Vec3 a, Vec3 b, Vec3 c) {
    return (b - a).cross(c - a).normalized();
}

static Mesh classify_and_merge(const Mesh& a, const Mesh& b,
                                bool keep_a_inside, bool keep_a_outside,
                                bool keep_b_inside, bool keep_b_outside,
                                bool flip_b_inside) {
    Mesh result;

    auto process = [&](const Mesh& source, const Mesh& other,
                       bool keep_inside, bool keep_outside, bool flip_inside) {
        for (size_t i = 0; i + 2 < source.indices.size(); i += 3) {
            Vec3 v0 = source.vertices[source.indices[i]].position;
            Vec3 v1 = source.vertices[source.indices[i + 1]].position;
            Vec3 v2 = source.vertices[source.indices[i + 2]].position;

            Vec3 centroid = (v0 + v1 + v2) * (1.f / 3.f);
            bool inside = point_inside_mesh(centroid, other);

            if ((inside && keep_inside) || (!inside && keep_outside)) {
                auto base = static_cast<uint32_t>(result.vertices.size());
                Vec3 n0 = source.vertices[source.indices[i]].normal;
                Vec3 n1 = source.vertices[source.indices[i + 1]].normal;
                Vec3 n2 = source.vertices[source.indices[i + 2]].normal;

                if (inside && flip_inside) {
                    n0 = -n0; n1 = -n1; n2 = -n2;
                    result.vertices.push_back({v0, n0});
                    result.vertices.push_back({v2, n2});
                    result.vertices.push_back({v1, n1});
                } else {
                    result.vertices.push_back({v0, n0});
                    result.vertices.push_back({v1, n1});
                    result.vertices.push_back({v2, n2});
                }
                result.indices.push_back(base);
                result.indices.push_back(base + 1);
                result.indices.push_back(base + 2);
            }
        }
    };

    process(a, b, keep_a_inside, keep_a_outside, false);
    process(b, a, keep_b_inside, keep_b_outside, flip_b_inside);

    return result;
}

Mesh csg_union(const Mesh& a, const Mesh& b) {
    return classify_and_merge(a, b,
        false, true,   // A: keep outside B
        false, true,   // B: keep outside A
        false);
}

Mesh csg_intersection(const Mesh& a, const Mesh& b) {
    return classify_and_merge(a, b,
        true, false,   // A: keep inside B
        true, false,   // B: keep inside A
        false);
}

Mesh csg_difference(const Mesh& a, const Mesh& b) {
    return classify_and_merge(a, b,
        false, true,   // A: keep outside B
        true, false,   // B: keep inside A (flipped)
        true);         // flip B normals when inside A
}

Mesh evaluate_csg_node(const Scene& scene, const SceneNode& node, int detail) {
    if (node.primitive) {
        Mesh m = node.primitive->generate_mesh(detail);
        Mat4 xform = node.world_transform(scene);
        for (auto& v : m.vertices) {
            v.position = xform.transform_point(v.position);
            v.normal = xform.transform_normal(v.normal);
        }
        return m;
    }

    if (node.children.size() < 2) {
        if (node.children.size() == 1) {
            auto* child = scene.find(node.children[0]);
            if (child) return evaluate_csg_node(scene, *child, detail);
        }
        return {};
    }

    auto* left = scene.find(node.children[0]);
    auto* right = scene.find(node.children[1]);
    if (!left || !right) return {};

    Mesh a = evaluate_csg_node(scene, *left, detail);
    Mesh b = evaluate_csg_node(scene, *right, detail);

    switch (node.boolean_op) {
        case BooleanOp::Union:        return csg_union(a, b);
        case BooleanOp::Intersection: return csg_intersection(a, b);
        case BooleanOp::Difference:   return csg_difference(a, b);
    }
    return a;
}

}  // namespace smidr
