#include "core/SoftBody.h"
#include "core/MeshOps.h"

#include <set>
#include <utility>

namespace smidr {

void SoftBody::pin(int i) {
    if (i >= 0 && i < static_cast<int>(points.size())) points[i].inv_mass = 0.f;
}

void SoftBody::unpin(int i) {
    if (i >= 0 && i < static_cast<int>(points.size())) points[i].inv_mass = 1.f;
}

void SoftBody::step(float dt) {
    for (auto& p : points) {
        if (p.inv_mass == 0) continue;
        Vec3 next = p.position + (p.position - p.prev_position) * damping + gravity * (dt * dt);
        p.prev_position = p.position;
        p.position = next;
    }

    for (int iter = 0; iter < solver_iterations; ++iter) {
        for (auto& c : constraints) {
            MassPoint& a = points[c.a];
            MassPoint& b = points[c.b];
            float w_sum = a.inv_mass + b.inv_mass;
            if (w_sum == 0) continue;
            Vec3 delta = b.position - a.position;
            float dist = delta.length();
            if (dist < 1e-6f) continue;
            Vec3 dir = delta * (1.f / dist);
            float err = dist - c.rest_length;
            Vec3 correction = dir * (err * c.stiffness / w_sum);
            a.position += correction * a.inv_mass;
            b.position -= correction * b.inv_mass;
        }
    }

    for (auto& p : points) {
        if (p.inv_mass == 0) continue;
        p.velocity = (p.position - p.prev_position) * (1.f / dt);
    }
}

SoftBody SoftBody::from_mesh(const Mesh& mesh, float stiffness) {
    SoftBody sb;
    Mesh welded = weld_vertices(mesh, 1e-5f);
    for (auto& v : welded.vertices) {
        MassPoint p;
        p.position = v.position;
        p.prev_position = v.position;
        sb.points.push_back(p);
    }
    std::set<std::pair<int,int>> edges;
    for (size_t i = 0; i + 2 < welded.indices.size(); i += 3) {
        for (int j = 0; j < 3; ++j) {
            int a = static_cast<int>(welded.indices[i + static_cast<size_t>(j)]);
            int b = static_cast<int>(welded.indices[i + static_cast<size_t>((j+1)%3)]);
            if (a > b) std::swap(a, b);
            edges.insert({a, b});
        }
    }
    for (auto& [a, b] : edges) {
        DistanceConstraint c;
        c.a = a; c.b = b;
        c.rest_length = (welded.vertices[static_cast<size_t>(b)].position - welded.vertices[static_cast<size_t>(a)].position).length();
        c.stiffness = stiffness;
        sb.constraints.push_back(c);
    }
    return sb;
}

Mesh SoftBody::to_mesh(const Mesh& reference) const {
    Mesh out = reference;
    for (size_t i = 0; i < out.vertices.size() && i < points.size(); ++i) {
        out.vertices[i].position = points[i].position;
    }
    return out;
}

}  // namespace smidr
