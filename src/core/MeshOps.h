#pragma once

#include "core/Primitive.h"

namespace smidr {

Mesh weld_vertices(const Mesh& mesh, float tolerance = 1e-5f);

Mesh remove_degenerate_triangles(const Mesh& mesh, float min_area = 1e-8f);

Mesh recompute_normals(const Mesh& mesh, bool smooth = true);

Mesh simplify_decimate(const Mesh& mesh, float target_ratio = 0.5f);

Mesh subdivide_loop(const Mesh& mesh);

Mesh fill_holes(const Mesh& mesh);

Mesh laplacian_smooth(const Mesh& mesh, int iterations = 5, float strength = 0.5f);

bool is_watertight(const Mesh& mesh);

struct MeshStats {
    int vertex_count;
    int triangle_count;
    int edge_count;
    int boundary_edge_count;
    int non_manifold_edge_count;
    float total_area;
    float total_volume;
    int genus_estimate;
};

MeshStats compute_mesh_stats(const Mesh& mesh);

}  // namespace smidr
