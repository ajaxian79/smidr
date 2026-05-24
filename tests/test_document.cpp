#include "core/Document.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>

static int failures = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { std::fprintf(stderr, "  FAIL: %s\n", msg); ++failures; } \
} while(0)

int test_document() {
    std::printf("--- test_document ---\n");
    failures = 0;

    // New document
    {
        smidr::Document doc;
        doc.new_document("Test");
        CHECK(doc.name() == "Test", "doc name");
        CHECK(!doc.dirty(), "not dirty on new");
    }

    // Save and load round-trip
    {
        auto tmp = std::filesystem::temp_directory_path() / "smidr_test.smidr";

        smidr::Document doc;
        doc.new_document("RoundTrip");
        doc.scene().add_primitive("Sphere.001",
            std::make_unique<smidr::Sphere>(2.5f));
        doc.scene().add_primitive("Box.001",
            std::make_unique<smidr::Box>(smidr::Vec3{1, 2, 3}));

        auto* sphere_node = doc.scene().find(doc.scene().root_ids()[0]);
        sphere_node->position = smidr::Vec3{1, 2, 3};
        sphere_node->color    = smidr::Vec3{0.8f, 0.2f, 0.1f};

        CHECK(doc.save(tmp), "save succeeded");

        smidr::Document loaded;
        CHECK(loaded.load(tmp), "load succeeded");
        CHECK(loaded.name() == "RoundTrip", "loaded name");
        CHECK(loaded.scene().root_ids().size() == 2, "loaded 2 roots");

        auto* s = loaded.scene().find(loaded.scene().root_ids()[0]);
        CHECK(s != nullptr, "found sphere node");
        CHECK(s->name == "Sphere.001", "sphere name preserved");
        CHECK(std::abs(s->position.x - 1.f) < 1e-6f, "position preserved");

        std::filesystem::remove(tmp);
    }

    std::printf("  %d failures\n", failures);
    return failures;
}
