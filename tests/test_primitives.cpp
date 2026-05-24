#include "core/Primitive.h"

#include <cassert>
#include <cmath>
#include <cstdio>

static int failures = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { std::fprintf(stderr, "  FAIL: %s\n", msg); ++failures; } \
} while(0)

int test_primitives() {
    std::printf("--- test_primitives ---\n");
    failures = 0;

    // Sphere
    {
        smidr::Sphere s(2.f);
        CHECK(s.type() == smidr::PrimitiveType::Sphere, "sphere type");
        CHECK(std::abs(s.radius - 2.f) < 1e-6f, "sphere radius");

        auto mesh = s.generate_mesh(8);
        CHECK(!mesh.vertices.empty(), "sphere has vertices");
        CHECK(!mesh.indices.empty(), "sphere has indices");
        CHECK(mesh.indices.size() % 3 == 0, "sphere indices are triangles");

        auto bb = s.local_bounds();
        CHECK(std::abs(bb.min_pt.x - (-2.f)) < 1e-6f, "sphere bounds min");
        CHECK(std::abs(bb.max_pt.x - 2.f) < 1e-6f, "sphere bounds max");

        auto c = s.clone();
        CHECK(c->type() == smidr::PrimitiveType::Sphere, "sphere clone type");
    }

    // Box
    {
        smidr::Box b(smidr::Vec3{1, 2, 3});
        auto mesh = b.generate_mesh();
        CHECK(mesh.vertices.size() == 24, "box has 24 vertices (4 per face)");
        CHECK(mesh.indices.size() == 36, "box has 36 indices (6 per face)");

        auto bb = b.local_bounds();
        CHECK(std::abs(bb.max_pt.y - 2.f) < 1e-6f, "box bounds");
    }

    // Cylinder
    {
        smidr::Cylinder cy(0.5f, 3.f);
        auto mesh = cy.generate_mesh(16);
        CHECK(!mesh.vertices.empty(), "cylinder has vertices");
        CHECK(mesh.indices.size() % 3 == 0, "cylinder indices are triangles");

        auto bb = cy.local_bounds();
        CHECK(std::abs(bb.max_pt.y - 1.5f) < 1e-6f, "cylinder height bounds");
    }

    // Cone
    {
        smidr::Cone co(1.f, 4.f);
        auto mesh = co.generate_mesh(16);
        CHECK(!mesh.vertices.empty(), "cone has vertices");
        CHECK(mesh.indices.size() % 3 == 0, "cone indices are triangles");
    }

    // Torus
    {
        smidr::Torus t(2.f, 0.5f);
        auto mesh = t.generate_mesh(16);
        CHECK(!mesh.vertices.empty(), "torus has vertices");
        CHECK(mesh.indices.size() % 3 == 0, "torus indices are triangles");

        auto bb = t.local_bounds();
        CHECK(std::abs(bb.max_pt.x - 2.5f) < 1e-6f, "torus bounds");
    }

    // Type name round-trip
    {
        for (auto t : {smidr::PrimitiveType::Sphere, smidr::PrimitiveType::Box,
                       smidr::PrimitiveType::Cylinder, smidr::PrimitiveType::Cone,
                       smidr::PrimitiveType::Torus}) {
            auto name = smidr::primitive_type_name(t);
            auto back = smidr::primitive_type_from_name(name);
            CHECK(back == t, "primitive type round-trip");
        }
    }

    std::printf("  %d failures\n", failures);
    return failures;
}
