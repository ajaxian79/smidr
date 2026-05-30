#pragma once

#include "core/Math.h"

#include <cstdint>
#include <string>
#include <vector>

namespace smidr {

class App;

struct SceneLight {
    enum Kind { Point, Directional, Area };
    Kind  kind = Point;
    Vec3  position{5, 8, 6};
    Vec3  direction{0, -1, 0};
    Vec3  color{1, 1, 1};
    float intensity = 1.f;
    float radius = 0.f;
    bool  enabled = true;
    std::string name = "Light";
};

class LightManager {
public:
    LightManager();
    std::vector<SceneLight>& lights() { return lights_; }
    const std::vector<SceneLight>& lights() const { return lights_; }
    int add(const SceneLight& l);
    void remove(int idx);
    void clear();

private:
    std::vector<SceneLight> lights_;
};

struct Keyframe {
    float time;
    Vec3  position;
    Vec3  rotation;
    Vec3  scale;
};

struct AnimationTrack {
    uint32_t  node_id = 0;
    std::vector<Keyframe> keys;

    void sample(float t, Vec3& out_pos, Vec3& out_rot, Vec3& out_scale) const;
};

class Timeline {
public:
    float current_time = 0.f;
    float duration = 5.f;
    float fps = 30.f;
    bool  playing = false;

    std::vector<AnimationTrack> tracks;

    void play() { playing = true; }
    void stop() { playing = false; current_time = 0.f; }
    void pause() { playing = false; }
    void advance(float dt);

    AnimationTrack* find_track(uint32_t node_id);
    void add_keyframe(uint32_t node_id, float t, Vec3 pos, Vec3 rot, Vec3 scale);

    void apply_to_scene(App& app) const;
};

LightManager& app_lights();
Timeline& app_timeline();

void draw_light_panel(App& app);
void draw_timeline_panel(App& app);

}  // namespace smidr
