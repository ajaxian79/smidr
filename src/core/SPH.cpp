#include "core/SPH.h"

#include <algorithm>
#include <cmath>

namespace smidr {

static float poly6_kernel(float r2, float h) {
    if (r2 >= h * h) return 0.f;
    float diff = h * h - r2;
    return 315.f / (64.f * kPi * std::pow(h, 9.f)) * diff * diff * diff;
}

static float spiky_grad(float r, float h) {
    if (r >= h || r < 1e-6f) return 0.f;
    float diff = h - r;
    return -45.f / (kPi * std::pow(h, 6.f)) * diff * diff;
}

static float viscosity_laplacian(float r, float h) {
    if (r >= h) return 0.f;
    return 45.f / (kPi * std::pow(h, 6.f)) * (h - r);
}

void SPHFluid::spawn_box(Vec3 mn, Vec3 mx, float spacing) {
    for (float z = mn.z; z <= mx.z; z += spacing)
        for (float y = mn.y; y <= mx.y; y += spacing)
            for (float x = mn.x; x <= mx.x; x += spacing) {
                SPHParticle p;
                p.position = {x, y, z};
                p.velocity = {0, 0, 0};
                particles.push_back(p);
            }
}

void SPHFluid::compute_density_and_pressure() {
    float h2 = smoothing_radius * smoothing_radius;
    for (auto& p : particles) {
        p.density = 0.f;
        for (auto& other : particles) {
            Vec3 d = other.position - p.position;
            float r2 = d.dot(d);
            if (r2 < h2) {
                p.density += other.mass * poly6_kernel(r2, smoothing_radius);
            }
        }
        p.density = std::max(p.density, rest_density);
        p.pressure = gas_constant * (p.density - rest_density);
    }
}

void SPHFluid::compute_forces() {
    for (auto& p : particles) {
        Vec3 pressure_force{0, 0, 0};
        Vec3 viscosity_force{0, 0, 0};

        for (auto& other : particles) {
            if (&other == &p) continue;
            Vec3 d = p.position - other.position;
            float r = d.length();
            if (r >= smoothing_radius || r < 1e-6f) continue;
            Vec3 dir = d * (1.f / r);

            float p_avg = (p.pressure + other.pressure) * 0.5f;
            pressure_force += dir * (other.mass * p_avg / other.density * spiky_grad(r, smoothing_radius));

            Vec3 v_diff = other.velocity - p.velocity;
            viscosity_force += v_diff * (other.mass / other.density
                                          * viscosity_laplacian(r, smoothing_radius));
        }
        viscosity_force *= viscosity;

        p.force = pressure_force + viscosity_force + gravity * p.density;
    }
}

void SPHFluid::integrate(float dt) {
    for (auto& p : particles) {
        Vec3 a = p.force * (1.f / std::max(p.density, 1e-6f));
        p.velocity += a * dt;
        p.position += p.velocity * dt;
    }
}

void SPHFluid::enforce_boundary() {
    for (auto& p : particles) {
        if (p.position.x < container.min_pt.x) {
            p.position.x = container.min_pt.x; p.velocity.x *= -damping;
        } else if (p.position.x > container.max_pt.x) {
            p.position.x = container.max_pt.x; p.velocity.x *= -damping;
        }
        if (p.position.y < container.min_pt.y) {
            p.position.y = container.min_pt.y; p.velocity.y *= -damping;
        } else if (p.position.y > container.max_pt.y) {
            p.position.y = container.max_pt.y; p.velocity.y *= -damping;
        }
        if (p.position.z < container.min_pt.z) {
            p.position.z = container.min_pt.z; p.velocity.z *= -damping;
        } else if (p.position.z > container.max_pt.z) {
            p.position.z = container.max_pt.z; p.velocity.z *= -damping;
        }
    }
}

void SPHFluid::step(float dt) {
    compute_density_and_pressure();
    compute_forces();
    integrate(dt);
    enforce_boundary();
}

}  // namespace smidr
