#include "core/RigidBody.h"

#include <algorithm>
#include <cmath>

namespace smidr {

int PhysicsWorld::add_body(const RigidBody& b) {
    bodies.push_back(b);
    return static_cast<int>(bodies.size()) - 1;
}

void PhysicsWorld::clear() { bodies.clear(); }

void PhysicsWorld::integrate(float dt) {
    for (auto& b : bodies) {
        if (b.is_static || !b.is_active) continue;
        b.velocity += gravity * dt;
        b.position += b.velocity * dt;
        b.rotation += b.angular_velocity * dt;
    }
}

bool PhysicsWorld::sphere_sphere(RigidBody& a, RigidBody& b) {
    Vec3 d = b.position - a.position;
    float dist = d.length();
    float min_dist = a.radius + b.radius;
    if (dist >= min_dist || dist < 1e-6f) return false;
    Vec3 n = d * (1.f / dist);
    float overlap = min_dist - dist;
    if (!a.is_static && !b.is_static) {
        a.position -= n * (overlap * 0.5f);
        b.position += n * (overlap * 0.5f);
    } else if (!a.is_static) {
        a.position -= n * overlap;
    } else if (!b.is_static) {
        b.position += n * overlap;
    }
    Vec3 rel = b.velocity - a.velocity;
    float vn = rel.dot(n);
    if (vn > 0) return true;
    float e = std::min(a.restitution, b.restitution);
    float inv_a = a.is_static ? 0 : 1.f / a.mass;
    float inv_b = b.is_static ? 0 : 1.f / b.mass;
    float j = -(1.f + e) * vn / (inv_a + inv_b);
    Vec3 impulse = n * j;
    if (!a.is_static) a.velocity -= impulse * inv_a;
    if (!b.is_static) b.velocity += impulse * inv_b;
    return true;
}

bool PhysicsWorld::sphere_plane(RigidBody& s, RigidBody& p) {
    Vec3 plane_normal{0, 1, 0};
    float plane_d = p.position.y;
    float dist = s.position.y - plane_d;
    if (dist > s.radius) return false;
    float overlap = s.radius - dist;
    s.position.y += overlap;
    float vn = s.velocity.dot(plane_normal);
    if (vn < 0) {
        s.velocity -= plane_normal * (vn * (1.f + s.restitution));
        Vec3 tangent_vel{s.velocity.x, 0, s.velocity.z};
        s.velocity.x = tangent_vel.x * (1.f - s.friction);
        s.velocity.z = tangent_vel.z * (1.f - s.friction);
    }
    return true;
}

void PhysicsWorld::resolve_collisions() {
    for (size_t i = 0; i < bodies.size(); ++i) {
        for (size_t j = i + 1; j < bodies.size(); ++j) {
            if (!bodies[i].is_active || !bodies[j].is_active) continue;
            if (bodies[i].shape == ColliderShape::Sphere && bodies[j].shape == ColliderShape::Sphere) {
                sphere_sphere(bodies[i], bodies[j]);
            } else if (bodies[i].shape == ColliderShape::Sphere && bodies[j].shape == ColliderShape::Plane) {
                sphere_plane(bodies[i], bodies[j]);
            } else if (bodies[i].shape == ColliderShape::Plane && bodies[j].shape == ColliderShape::Sphere) {
                sphere_plane(bodies[j], bodies[i]);
            }
        }
    }
}

void PhysicsWorld::step(float dt) {
    float sub_dt = (dt * time_scale) / static_cast<float>(substeps);
    for (int s = 0; s < substeps; ++s) {
        integrate(sub_dt);
        resolve_collisions();
    }
}

}  // namespace smidr
