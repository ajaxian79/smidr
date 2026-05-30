#pragma once

#include "core/Primitive.h"

namespace smidr {

// Adaptive tessellation: refines mesh where curvature is high,
// keeps coarse where flat. Iterative split of edges that exceed
// a curvature × length threshold.
Mesh adaptive_subdivide(const Mesh& mesh, float curvature_threshold = 0.5f,
                         int max_iterations = 3);

// Refine along edges that exceed maximum edge length.
Mesh refine_long_edges(const Mesh& mesh, float max_edge_len);

// Collapse edges shorter than the threshold (decimation in flat regions).
Mesh collapse_short_edges(const Mesh& mesh, float min_edge_len);

// Combined: refine high-curvature regions, decimate flat regions
Mesh remesh_adaptive(const Mesh& mesh, float target_edge, float curvature_weight);

}  // namespace smidr
