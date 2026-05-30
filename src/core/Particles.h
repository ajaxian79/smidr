#pragma once

#include "core/Math.h"

#include <cstdint>
#include <functional>
#include <vector>

namespace smidr {

struct ParticleInstance {
    Vec3  position;
    Vec3  velocity;
    Vec3  color;
    float age = 0.f;
    float lifetime = 5.f;
    float size = 0.1f;
    float mass = 1.f;
    bool  alive = true;
};

struct Emitter {
    Vec3  position{0, 0, 0};
    Vec3  velocity_base{0, 1, 0};
    float velocity_variance = 0.5f;
    float spawn_rate = 50.f;
    float particle_lifetime = 3.f;
    float particle_size = 0.05f;
    Vec3  color{1.f, 0.8f, 0.3f};
    float accumulator = 0.f;
};

using ForceField = std::function<Vec3(Vec3)>;

namespace force {
inline ForceField gravity(Vec3 g = {0, -9.8f, 0}) {
    return [g](Vec3) { return g; };
}
inline ForceField wind(Vec3 dir) {
    return [dir](Vec3) { return dir; };
}
inline ForceField vortex(Vec3 center, Vec3 axis, float strength) {
    return [center, axis, strength](Vec3 p) {
        Vec3 r = p - center;
        Vec3 a = axis.normalized();
        Vec3 tangent = a.cross(r);
        return tangent * strength;
    };
}
inline ForceField attractor(Vec3 center, float strength) {
    return [center, strength](Vec3 p) {
        Vec3 d = center - p;
        float len = d.length();
        if (len < 1e-3f) return Vec3{0, 0, 0};
        return d * (strength / (len * len + 0.1f));
    };
}
}

class ParticleSystem {
public:
    std::vector<ParticleInstance> particles;
    std::vector<Emitter>          emitters;
    std::vector<ForceField>       forces;
    int max_particles = 10000;

    void update(float dt);
    void emit_burst(Vec3 pos, int count, Vec3 base_vel, float variance);
    void clear();

private:
    uint32_t rng_state = 12345u;
    float rand01();
};

}  // namespace smidr
