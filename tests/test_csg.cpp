#include "core/Scene.h"
#include "core/MeshGenerator.h"

#include <cstdio>

static int failures = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { std::fprintf(stderr, "  FAIL: %s\n", msg); ++failures; } \
} while(0)

int test_csg() {
    std::printf("--- test_csg ---\n");
    failures = 0;

    // Scene add/find/select
    {
        smidr::Scene scene;
        auto id1 = scene.add_primitive("Sphere.001",
            std::make_unique<smidr::Sphere>(1.f));
        auto id2 = scene.add_primitive("Box.001",
            std::make_unique<smidr::Box>());

        CHECK(scene.root_ids().size() == 2, "two roots");
        CHECK(scene.find(id1) != nullptr, "find sphere");
        CHECK(scene.find(id2) != nullptr, "find box");
        CHECK(scene.find(id1)->name == "Sphere.001", "sphere name");

        scene.select(id1);
        CHECK(scene.selected_id().has_value(), "has selection");
        CHECK(*scene.selected_id() == id1, "correct selection");

        scene.clear_selection();
        CHECK(!scene.selected_id().has_value(), "cleared selection");
    }

    // Boolean ops
    {
        smidr::Scene scene;
        auto s = scene.add_primitive("S", std::make_unique<smidr::Sphere>());
        auto b = scene.add_primitive("B", std::make_unique<smidr::Box>());
        CHECK(scene.root_ids().size() == 2, "two roots before boolean");

        auto u = scene.add_boolean("Union", smidr::BooleanOp::Union, s, b);
        CHECK(scene.root_ids().size() == 1, "one root after boolean");
        CHECK(scene.find(u)->children.size() == 2, "boolean has 2 children");
        CHECK(scene.find(s)->parent == u, "sphere parent is union");
    }

    // Remove node
    {
        smidr::Scene scene;
        auto id = scene.add_primitive("X", std::make_unique<smidr::Sphere>());
        CHECK(scene.root_ids().size() == 1, "one root");
        scene.remove_node(id);
        CHECK(scene.root_ids().empty(), "empty after remove");
    }

    // Groups
    {
        smidr::Scene scene;
        auto g = scene.add_group("Group");
        auto s = scene.add_primitive("S", std::make_unique<smidr::Sphere>(), g);
        CHECK(scene.find(g)->children.size() == 1, "group has 1 child");
        CHECK(scene.find(s)->parent == g, "sphere parent is group");
    }

    // MeshGenerator
    {
        smidr::Scene scene;
        scene.add_primitive("S", std::make_unique<smidr::Sphere>());
        scene.add_primitive("B", std::make_unique<smidr::Box>());

        smidr::MeshGenerator gen;
        gen.rebuild(scene);
        CHECK(gen.entries().size() == 2, "2 mesh entries");
        CHECK(!gen.entries()[0].mesh.vertices.empty(), "mesh 0 has verts");
        CHECK(!gen.entries()[1].mesh.vertices.empty(), "mesh 1 has verts");
    }

    std::printf("  %d failures\n", failures);
    return failures;
}
