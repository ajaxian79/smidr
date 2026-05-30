#pragma once

#include "core/Math.h"

#include <vector>

namespace smidr {

enum class ColliderShape { Sphere, Box, Plane };

struct RigidBody {
    Vec3 position{0, 0, 0};
    Vec3 velocity{0, 0, 0};
    Vec3 angular_velocity{0, 0, 0};
    Vec3 rotation{0, 0, 0};
    float mass = 1.f;
    float restitution = 0.5f;
    float friction = 0.3f;
    bool  is_static = false;
    bool  is_active = true;

    ColliderShape shape = ColliderShape::Sphere;
    Vec3  half_extents{0.5f, 0.5f, 0.5f};
    float radius = 0.5f;
};

class PhysicsWorld {
public:
    std::vector<RigidBody> bodies;
    Vec3 gravity{0, -9.8f, 0};
    int  substeps = 4;
    float time_scale = 1.f;

    void step(float dt);
    int  add_body(const RigidBody& b);
    void clear();

private:
    void integrate(float dt);
    void resolve_collisions();
    bool sphere_sphere(RigidBody& a, RigidBody& b);
    bool sphere_plane(RigidBody& s, RigidBody& p);
};

}  // namespace smidr
