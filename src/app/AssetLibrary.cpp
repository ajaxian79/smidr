#include "app/AssetLibrary.h"
#include "app/App.h"
#include "core/PrimitivesAdvanced.h"
#include "core/PrimitivesExtra.h"

namespace smidr {

AssetLibrary::AssetLibrary() {
    install_defaults();
}

AssetLibrary& AssetLibrary::instance() {
    static AssetLibrary lib;
    return lib;
}

void AssetLibrary::add_primitive(PrimitivePreset p) { primitives_.push_back(std::move(p)); }
void AssetLibrary::add_material(MaterialPreset m) { materials_.push_back(std::move(m)); }
void AssetLibrary::add_scene(ScenePreset s) { scenes_.push_back(std::move(s)); }

std::vector<PrimitivePreset> AssetLibrary::by_category(const std::string& cat) const {
    std::vector<PrimitivePreset> out;
    for (auto& p : primitives_) if (p.category == cat) out.push_back(p);
    return out;
}

void AssetLibrary::install_defaults() {
    add_primitive({"Unit Sphere", "Basic", [] {
        return std::make_unique<Sphere>(1.f);
    }, "Sphere of radius 1"});
    add_primitive({"Unit Box", "Basic", [] {
        return std::make_unique<Box>();
    }, "Cube with half-extent 0.5"});
    add_primitive({"Tall Cylinder", "Basic", [] {
        return std::make_unique<Cylinder>(0.3f, 4.f);
    }, "Slim tall cylinder"});

    add_primitive({"Bolt M8", "Hardware", [] {
        auto p = std::make_unique<Cylinder>(0.4f, 2.f);
        return p;
    }, "Cylindrical bolt"});
    add_primitive({"Nut Hex", "Hardware", [] {
        auto p = std::make_unique<Arb8>();
        return p;
    }, "Hex nut"});
    add_primitive({"Washer", "Hardware", [] {
        return std::make_unique<Pipe>(0.4f, 0.6f, 0.05f);
    }, "Flat washer"});
    add_primitive({"Standoff", "Hardware", [] {
        return std::make_unique<Cylinder>(0.25f, 2.f);
    }, "Cylindrical standoff"});

    add_primitive({"Gear blank", "Mechanical", [] {
        return std::make_unique<Cylinder>(1.f, 0.3f);
    }, "Disk for gear cutting"});
    add_primitive({"Shaft", "Mechanical", [] {
        return std::make_unique<Cylinder>(0.2f, 5.f);
    }, "Drive shaft"});
    add_primitive({"Bearing race", "Mechanical", [] {
        return std::make_unique<Pipe>(0.7f, 1.f, 0.4f);
    }, "Ball bearing outer race"});

    add_primitive({"Cylinder ø10", "Pipe", [] {
        return std::make_unique<Pipe>(0.4f, 0.5f, 4.f);
    }, "Schedule 40 pipe section"});
    add_primitive({"Pipe elbow", "Pipe", [] {
        return std::make_unique<Torus>(1.f, 0.2f);
    }, "90° pipe bend"});

    add_primitive({"Decoration cone", "Decorative", [] {
        return std::make_unique<Cone>(0.5f, 1.5f);
    }, "Cone"});
    add_primitive({"Star (heart)", "Decorative", [] {
        return std::make_unique<HeartPrimitive>();
    }, "Heart shape"});
    add_primitive({"Pearl", "Decorative", [] {
        return std::make_unique<Ellipsoid>(Vec3{0.5f, 0.4f, 0.5f});
    }, "Lustrous bead"});

    add_material({"Default", "Basic", {"Default", {0.6f, 0.6f, 0.7f}, 0.5f, 0.f, 1.f}});
    add_material({"Red Plastic", "Basic", {"Red Plastic", {0.8f, 0.1f, 0.1f}, 0.5f, 0.f, 1.f}});
    add_material({"Blue Plastic", "Basic", {"Blue Plastic", {0.1f, 0.3f, 0.85f}, 0.5f, 0.f, 1.f}});
    add_material({"White Plastic", "Basic", {"White Plastic", {0.95f, 0.95f, 0.95f}, 0.4f, 0.f, 1.f}});
    add_material({"Black Rubber", "Basic", {"Black Rubber", {0.05f, 0.05f, 0.05f}, 0.9f, 0.f, 1.f}});

    add_material({"Chrome", "Metal", {"Chrome", {0.9f, 0.9f, 0.95f}, 0.1f, 1.f, 1.f}});
    add_material({"Brushed Aluminum", "Metal", {"Brushed Aluminum", {0.85f, 0.85f, 0.85f}, 0.35f, 1.f, 1.f}});
    add_material({"Gold", "Metal", {"Gold", {1.0f, 0.85f, 0.4f}, 0.2f, 1.f, 1.f}});
    add_material({"Copper", "Metal", {"Copper", {0.95f, 0.6f, 0.45f}, 0.25f, 1.f, 1.f}});
    add_material({"Steel", "Metal", {"Steel", {0.6f, 0.6f, 0.6f}, 0.4f, 1.f, 1.f}});

    add_material({"Glass Clear", "Transparent", {"Glass Clear", {0.95f, 0.98f, 1.f}, 0.02f, 0.f, 0.15f}});
    add_material({"Glass Frosted", "Transparent", {"Glass Frosted", {0.92f, 0.94f, 0.97f}, 0.5f, 0.f, 0.4f}});

    add_material({"Wood Oak", "Organic", {"Wood Oak", {0.7f, 0.5f, 0.3f}, 0.6f, 0.f, 1.f}});
    add_material({"Wood Pine", "Organic", {"Wood Pine", {0.85f, 0.7f, 0.5f}, 0.65f, 0.f, 1.f}});

    add_scene({"Workshop layout", "3 tables and tools", [](App& app) {
        for (int i = 0; i < 3; ++i) {
            auto p = std::make_unique<Box>(Vec3{1.f, 0.05f, 0.5f});
            auto id = app.document().scene().add_primitive(
                "Table." + std::to_string(i+1), std::move(p));
            auto* n = app.document().scene().find(id);
            if (n) n->position = {static_cast<float>(i) * 2.5f, 1.f, 0.f};
        }
    }});
    add_scene({"Solar system", "Procedural solar system", [](App& app) {
        const char* names[] = {"Sun", "Mercury", "Venus", "Earth", "Mars"};
        float radii[] = {1.5f, 0.15f, 0.25f, 0.27f, 0.2f};
        float orbits[] = {0.f, 2.5f, 3.5f, 5.f, 7.f};
        for (int i = 0; i < 5; ++i) {
            auto p = std::make_unique<Sphere>(radii[i]);
            auto id = app.document().scene().add_primitive(names[i], std::move(p));
            auto* n = app.document().scene().find(id);
            if (n) n->position = {orbits[i], 0.f, 0.f};
        }
    }});
    add_scene({"Engine block", "5 cylinder + crankshaft mockup", [](App& app) {
        auto block = std::make_unique<Box>(Vec3{2.f, 0.6f, 0.8f});
        auto bid = app.document().scene().add_primitive("Block", std::move(block));
        auto* bn = app.document().scene().find(bid);
        if (bn) bn->color = {0.5f, 0.5f, 0.55f};
        for (int i = 0; i < 5; ++i) {
            auto cyl = std::make_unique<Cylinder>(0.25f, 1.f);
            auto id = app.document().scene().add_primitive(
                "Cyl." + std::to_string(i+1), std::move(cyl));
            auto* n = app.document().scene().find(id);
            if (n) { n->position = {static_cast<float>(i)*0.7f - 1.4f, 1.f, 0.f}; n->color = {0.85f, 0.85f, 0.85f}; }
        }
        auto shaft = std::make_unique<Cylinder>(0.15f, 4.f);
        auto sid = app.document().scene().add_primitive("Crankshaft", std::move(shaft));
        auto* sn = app.document().scene().find(sid);
        if (sn) { sn->position = {0.f, -0.5f, 0.f}; sn->rotation = {0.f, 0.f, 90.f}; sn->color = {0.7f, 0.7f, 0.75f}; }
    }});
}

}  // namespace smidr
