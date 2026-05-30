#pragma once

#include "core/Math.h"
#include "core/Primitive.h"

#include <vector>

namespace smidr {

struct MassPoint {
    Vec3 position;
    Vec3 prev_position;
    Vec3 velocity{0, 0, 0};
    float inv_mass = 1.f;
};

struct DistanceConstraint {
    int a, b;
    float rest_length;
    float stiffness = 1.f;
};

class SoftBody {
public:
    std::vector<MassPoint>          points;
    std::vector<DistanceConstraint> constraints;
    Vec3  gravity{0, -9.8f, 0};
    int   solver_iterations = 5;
    float damping = 0.98f;

    void step(float dt);
    void pin(int index);
    void unpin(int index);

    static SoftBody from_mesh(const Mesh& mesh, float stiffness = 0.9f);
    Mesh to_mesh(const Mesh& reference) const;
};

}  // namespace smidr
