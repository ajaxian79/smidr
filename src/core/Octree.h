#pragma once

#include "core/Math.h"
#include "core/Primitive.h"
#include "core/RayCast.h"

#include <memory>
#include <vector>

namespace smidr {

struct TriangleRef {
    Vec3 v0, v1, v2;
    uint32_t node_id;
};

class Octree {
public:
    void build(const std::vector<TriangleRef>& tris, int max_depth = 6, int max_leaf_size = 16);

    std::optional<HitResult> ray_cast(const Ray& ray) const;

    int  node_count() const { return static_cast<int>(nodes_.size()); }
    int  triangle_count() const { return static_cast<int>(triangles_.size()); }

private:
    struct Node {
        AABB bounds;
        std::vector<int> tri_indices;
        int children[8] = {-1,-1,-1,-1,-1,-1,-1,-1};
        bool is_leaf = true;
    };

    int build_recursive(const std::vector<int>& indices, AABB bounds, int depth, int max_depth, int max_leaf_size);
    bool ray_aabb(const Ray& ray, const AABB& box, float& tmin) const;
    std::optional<HitResult> ray_cast_node(const Ray& ray, int node_idx) const;

    std::vector<Node> nodes_;
    std::vector<TriangleRef> triangles_;
};

}  // namespace smidr
