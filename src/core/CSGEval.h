#pragma once

#include "core/Primitive.h"
#include "core/Scene.h"

namespace smidr {

Mesh csg_union(const Mesh& a, const Mesh& b);
Mesh csg_intersection(const Mesh& a, const Mesh& b);
Mesh csg_difference(const Mesh& a, const Mesh& b);

Mesh evaluate_csg_node(const Scene& scene, const SceneNode& node, int detail = 32);

}  // namespace smidr
