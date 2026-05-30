#include "core/Particles.h"

#include <algorithm>
#include <cmath>

namespace smidr {

float ParticleSystem::rand01() {
    rng_state = rng_state * 1664525u + 1013904223u;
    return static_cast<float>(rng_state & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
}

void ParticleSystem::update(float dt) {
    for (auto& e : emitters) {
        e.accumulator += dt * e.spawn_rate;
        int spawn = static_cast<int>(e.accumulator);
        e.accumulator -= static_cast<float>(spawn);
        for (int i = 0; i < spawn && static_cast<int>(particles.size()) < max_particles; ++i) {
            ParticleInstance p;
            p.position = e.position;
            p.velocity = e.velocity_base
                       + Vec3{(rand01() - 0.5f) * e.velocity_variance,
                              (rand01() - 0.5f) * e.velocity_variance,
                              (rand01() - 0.5f) * e.velocity_variance};
            p.color = e.color;
            p.size = e.particle_size;
            p.lifetime = e.particle_lifetime;
            p.age = 0.f;
            p.alive = true;
            particles.push_back(p);
        }
    }

    for (auto& p : particles) {
        if (!p.alive) continue;
        Vec3 force_sum{0, 0, 0};
        for (auto& f : forces) force_sum += f(p.position);
        Vec3 accel = force_sum * (1.f / p.mass);
        p.velocity += accel * dt;
        p.position += p.velocity * dt;
        p.age += dt;
        if (p.age >= p.lifetime) p.alive = false;
    }

    particles.erase(
        std::remove_if(particles.begin(), particles.end(),
                        [](const ParticleInstance& p) { return !p.alive; }),
        particles.end());
}

void ParticleSystem::emit_burst(Vec3 pos, int count, Vec3 base_vel, float variance) {
    for (int i = 0; i < count && static_cast<int>(particles.size()) < max_particles; ++i) {
        ParticleInstance p;
        p.position = pos;
        p.velocity = base_vel
                   + Vec3{(rand01() - 0.5f) * variance,
                          (rand01() - 0.5f) * variance,
                          (rand01() - 0.5f) * variance};
        p.color = {1.f, 0.8f, 0.3f};
        p.size = 0.1f;
        p.lifetime = 3.f;
        particles.push_back(p);
    }
}

void ParticleSystem::clear() {
    particles.clear();
    emitters.clear();
    forces.clear();
}

}  // namespace smidr
