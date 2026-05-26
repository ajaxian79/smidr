#pragma once

#include "core/Math.h"
#include "core/Primitive.h"

namespace smidr {

struct AnalysisResult {
    float volume       = 0.f;
    float surface_area  = 0.f;
    float mass          = 0.f;
    Vec3  centroid{0, 0, 0};
    float Ixx = 0.f, Iyy = 0.f, Izz = 0.f;
    float Ixy = 0.f, Ixz = 0.f, Iyz = 0.f;
    int   triangle_count = 0;
    int   vertex_count   = 0;
    bool  valid = false;
};

AnalysisResult analyze_mesh(const Mesh& mesh, float density = 1.f);

float compute_volume(const Mesh& mesh);
float compute_surface_area(const Mesh& mesh);
Vec3  compute_centroid(const Mesh& mesh);

struct MomentsOfInertia {
    float Ixx, Iyy, Izz;
    float Ixy, Ixz, Iyz;
};
MomentsOfInertia compute_moments(const Mesh& mesh, Vec3 centroid, float density = 1.f);

bool point_inside_closed_mesh(Vec3 p, const Mesh& mesh);

AABB compute_tight_bounds(const Mesh& mesh);

}  // namespace smidr
