#pragma once

#include "core/Primitive.h"

namespace smidr {

Mesh chamfer_edges(const Mesh& mesh, float dist);
Mesh fillet_edges(const Mesh& mesh, float radius, int segments = 4);
Mesh offset_mesh(const Mesh& mesh, float distance);
Mesh shell_mesh(const Mesh& mesh, float thickness);
Mesh inset_faces(const Mesh& mesh, float amount);
Mesh extrude_faces(const Mesh& mesh, Vec3 direction, float distance);

}  // namespace smidr
