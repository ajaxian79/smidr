#pragma once

#include "core/Math.h"

#include <vector>

namespace smidr {

struct SPHParticle {
    Vec3 position;
    Vec3 velocity;
    Vec3 force;
    float density = 0.f;
    float pressure = 0.f;
    float mass = 1.f;
};

class SPHFluid {
public:
    std::vector<SPHParticle> particles;
    float smoothing_radius = 0.4f;
    float rest_density = 1000.f;
    float gas_constant = 50.f;
    float viscosity = 0.5f;
    float surface_tension = 0.1f;
    Vec3 gravity{0, -9.8f, 0};
    AABB container{{-3, 0, -3}, {3, 6, 3}};
    float damping = 0.5f;

    void step(float dt);
    void spawn_box(Vec3 min_pt, Vec3 max_pt, float spacing);
    void clear() { particles.clear(); }

private:
    void compute_density_and_pressure();
    void compute_forces();
    void integrate(float dt);
    void enforce_boundary();
};

}  // namespace smidr
