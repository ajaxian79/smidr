#include "app/LightsAndAnim.h"
#include "app/App.h"
#include "gui/ui.h"

#include <algorithm>
#include <cstdio>
#include <imgui.h>

namespace smidr {

LightManager::LightManager() {
    SceneLight key;
    key.kind = SceneLight::Area;
    key.position = {5, 8, 6};
    key.color = {1, 1, 1};
    key.intensity = 0.8f;
    key.radius = 0.5f;
    key.name = "Key";
    lights_.push_back(key);

    SceneLight fill;
    fill.kind = SceneLight::Point;
    fill.position = {-3, 4, -2};
    fill.color = {0.6f, 0.7f, 0.8f};
    fill.intensity = 0.4f;
    fill.name = "Fill";
    lights_.push_back(fill);
}

int LightManager::add(const SceneLight& l) {
    lights_.push_back(l);
    return static_cast<int>(lights_.size()) - 1;
}

void LightManager::remove(int idx) {
    if (idx >= 0 && idx < static_cast<int>(lights_.size()))
        lights_.erase(lights_.begin() + idx);
}

void LightManager::clear() { lights_.clear(); }

LightManager& app_lights() {
    static LightManager mgr;
    return mgr;
}

void AnimationTrack::sample(float t, Vec3& out_pos, Vec3& out_rot, Vec3& out_scale) const {
    if (keys.empty()) { out_pos = {}; out_rot = {}; out_scale = {1,1,1}; return; }
    if (t <= keys.front().time) {
        out_pos = keys.front().position; out_rot = keys.front().rotation; out_scale = keys.front().scale; return;
    }
    if (t >= keys.back().time) {
        out_pos = keys.back().position; out_rot = keys.back().rotation; out_scale = keys.back().scale; return;
    }
    for (size_t i = 0; i + 1 < keys.size(); ++i) {
        if (t >= keys[i].time && t <= keys[i+1].time) {
            float dur = keys[i+1].time - keys[i].time;
            float u = dur > 1e-6f ? (t - keys[i].time) / dur : 0.f;
            out_pos = keys[i].position + (keys[i+1].position - keys[i].position) * u;
            out_rot = keys[i].rotation + (keys[i+1].rotation - keys[i].rotation) * u;
            out_scale = keys[i].scale + (keys[i+1].scale - keys[i].scale) * u;
            return;
        }
    }
}

AnimationTrack* Timeline::find_track(uint32_t node_id) {
    for (auto& t : tracks)
        if (t.node_id == node_id) return &t;
    return nullptr;
}

void Timeline::add_keyframe(uint32_t node_id, float t, Vec3 p, Vec3 r, Vec3 s) {
    auto* track = find_track(node_id);
    if (!track) {
        AnimationTrack tr;
        tr.node_id = node_id;
        tracks.push_back(tr);
        track = &tracks.back();
    }
    track->keys.push_back({t, p, r, s});
    std::sort(track->keys.begin(), track->keys.end(),
              [](const Keyframe& a, const Keyframe& b) { return a.time < b.time; });
}

void Timeline::advance(float dt) {
    if (!playing) return;
    current_time += dt;
    if (current_time > duration) current_time = 0.f;
}

void Timeline::apply_to_scene(App& app) const {
    for (auto& track : tracks) {
        auto* node = app.document().scene().find(track.node_id);
        if (!node) continue;
        Vec3 p, r, s;
        track.sample(current_time, p, r, s);
        if (!track.keys.empty()) {
            node->position = p;
            node->rotation = r;
            node->scale_vec = s;
        }
    }
}

Timeline& app_timeline() {
    static Timeline tl;
    return tl;
}

void draw_light_panel(App& app) {
    ui::SectionHeader("Lights");
    auto& lights = app_lights().lights();
    for (size_t i = 0; i < lights.size(); ++i) {
        auto& l = lights[i];
        ImGui::PushID(static_cast<int>(i));
        char hdr[128];
        std::snprintf(hdr, sizeof hdr, "%s##L%zu", l.name.c_str(), i);
        if (ImGui::TreeNode(hdr)) {
            ImGui::Checkbox("Enabled", &l.enabled);
            ImGui::Combo("Kind", reinterpret_cast<int*>(&l.kind),
                          "Point\0Directional\0Area\0\0");
            ImGui::DragFloat3("Position", &l.position.x, 0.1f);
            ImGui::DragFloat3("Direction", &l.direction.x, 0.05f);
            ImGui::ColorEdit3("Color", &l.color.x);
            ImGui::DragFloat("Intensity", &l.intensity, 0.05f, 0.f, 10.f);
            if (l.kind == SceneLight::Area)
                ImGui::DragFloat("Radius", &l.radius, 0.05f, 0.f, 5.f);
            if (ImGui::Button("Remove")) {
                app_lights().remove(static_cast<int>(i));
                ImGui::TreePop();
                ImGui::PopID();
                return;
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    if (ImGui::Button("+ Add Light")) {
        SceneLight l;
        l.name = "Light." + std::to_string(lights.size());
        app_lights().add(l);
    }
    (void)app;
}

void draw_timeline_panel(App& app) {
    ui::SectionHeader("Timeline");
    auto& tl = app_timeline();

    ImGui::Text("%.2fs / %.2fs", tl.current_time, tl.duration);
    ImGui::SameLine();
    if (ImGui::Button(tl.playing ? "Pause" : "Play")) tl.playing = !tl.playing;
    ImGui::SameLine();
    if (ImGui::Button("Stop")) tl.stop();
    ImGui::SameLine();
    if (ImGui::Button("Reset")) tl.current_time = 0.f;

    ImGui::SliderFloat("Time", &tl.current_time, 0.f, tl.duration);
    ImGui::DragFloat("Duration", &tl.duration, 0.5f, 1.f, 60.f);
    ImGui::DragFloat("FPS", &tl.fps, 1.f, 1.f, 120.f);

    if (ImGui::Button("Add keyframe for selected")) {
        auto sel = app.document().scene().selected_id();
        if (sel) {
            auto* n = app.document().scene().find(*sel);
            if (n) tl.add_keyframe(n->id, tl.current_time,
                                    n->position, n->rotation, n->scale_vec);
        }
    }

    ImGui::Text("Tracks: %zu", tl.tracks.size());
    for (auto& track : tl.tracks) {
        ImGui::BulletText("node %u: %zu keys", track.node_id, track.keys.size());
    }
}

}  // namespace smidr
