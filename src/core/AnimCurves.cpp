#include "core/AnimCurves.h"

#include <algorithm>
#include <cmath>

namespace smidr {

void AnimCurve::add_key(CurveKey k) { keys.push_back(k); sort_keys(); }
void AnimCurve::remove_key(int idx) {
    if (idx >= 0 && idx < static_cast<int>(keys.size()))
        keys.erase(keys.begin() + idx);
}
void AnimCurve::sort_keys() {
    std::sort(keys.begin(), keys.end(),
              [](const CurveKey& a, const CurveKey& b) { return a.time < b.time; });
}

float AnimCurve::bezier_interp(const CurveKey& a, const CurveKey& b, float u) const {
    float dt = b.time - a.time;
    float p0 = a.value;
    float p1 = a.value + a.out_tangent * dt / 3.f;
    float p2 = b.value - b.in_tangent * dt / 3.f;
    float p3 = b.value;
    float v = 1.f - u;
    return v*v*v*p0 + 3*v*v*u*p1 + 3*v*u*u*p2 + u*u*u*p3;
}

float AnimCurve::hermite_interp(const CurveKey& a, const CurveKey& b, float u) const {
    float dt = b.time - a.time;
    float p0 = a.value, p1 = b.value;
    float m0 = a.out_tangent * dt, m1 = b.in_tangent * dt;
    float h00 = 2*u*u*u - 3*u*u + 1;
    float h10 = u*u*u - 2*u*u + u;
    float h01 = -2*u*u*u + 3*u*u;
    float h11 = u*u*u - u*u;
    return h00*p0 + h10*m0 + h01*p1 + h11*m1;
}

float AnimCurve::sample(float t) const {
    if (keys.empty()) return 0.f;
    if (t <= keys.front().time) return keys.front().value;
    if (t >= keys.back().time) return keys.back().value;
    for (size_t i = 0; i + 1 < keys.size(); ++i) {
        if (t >= keys[i].time && t <= keys[i+1].time) {
            const auto& a = keys[i];
            const auto& b = keys[i+1];
            if (a.interp == CurveInterp::Step) return a.value;
            float dt = b.time - a.time;
            float u = dt > 1e-8f ? (t - a.time) / dt : 0.f;
            switch (a.interp) {
                case CurveInterp::Linear: return a.value + (b.value - a.value) * u;
                case CurveInterp::Bezier: return bezier_interp(a, b, u);
                case CurveInterp::Hermite: return hermite_interp(a, b, u);
                case CurveInterp::CatmullRom: {
                    float u2 = u * u, u3 = u2 * u;
                    return 0.5f * ((2.f * a.value) + (-a.value + b.value) * u
                                   + (2*a.value - 2*b.value) * u2
                                   + (-a.value + b.value) * u3);
                }
                default: return a.value;
            }
        }
    }
    return keys.back().value;
}

int Skeleton::add_bone(const std::string& name, Vec3 head, Vec3 tail, int parent) {
    Bone b;
    b.name = name; b.head = head; b.tail = tail; b.parent = parent;
    bones.push_back(b);
    return static_cast<int>(bones.size()) - 1;
}

Mat4 Skeleton::world_transform(int bone_idx) const {
    if (bone_idx < 0 || bone_idx >= static_cast<int>(bones.size())) return Mat4::identity();
    const auto& b = bones[bone_idx];
    Mat4 local = Mat4::translate(b.head)
               * Mat4::rotate_z(b.rotation.z * kDegToRad)
               * Mat4::rotate_y(b.rotation.y * kDegToRad)
               * Mat4::rotate_x(b.rotation.x * kDegToRad);
    if (b.parent >= 0) return world_transform(b.parent) * local;
    return local;
}

void Skeleton::update_from_curves(const std::vector<AnimCurve>& curves, float time) {
    size_t per_bone = 3;
    for (size_t i = 0; i < bones.size(); ++i) {
        size_t base = i * per_bone;
        if (base + 2 < curves.size()) {
            bones[i].rotation = {curves[base].sample(time),
                                  curves[base+1].sample(time),
                                  curves[base+2].sample(time)};
        }
    }
}

void IKChain::solve_ccd() {
    if (!skeleton || end_effector < 0 || root < 0) return;
    if (end_effector >= static_cast<int>(skeleton->bones.size())) return;

    std::vector<int> chain;
    int cur = end_effector;
    while (cur >= 0) {
        chain.push_back(cur);
        if (cur == root) break;
        cur = skeleton->bones[cur].parent;
    }

    for (int iter = 0; iter < max_iterations; ++iter) {
        Mat4 ee_xform = skeleton->world_transform(end_effector);
        Vec3 ee_pos{ee_xform.m[12], ee_xform.m[13], ee_xform.m[14]};
        Vec3 diff = target_position - ee_pos;
        if (diff.length() < tolerance) break;

        for (size_t i = 1; i < chain.size(); ++i) {
            int b = chain[i];
            Mat4 b_xform = skeleton->world_transform(b);
            Vec3 b_pos{b_xform.m[12], b_xform.m[13], b_xform.m[14]};

            Mat4 ee_now = skeleton->world_transform(end_effector);
            Vec3 ee_now_pos{ee_now.m[12], ee_now.m[13], ee_now.m[14]};

            Vec3 to_ee = (ee_now_pos - b_pos).normalized();
            Vec3 to_target = (target_position - b_pos).normalized();
            Vec3 axis = to_ee.cross(to_target);
            float dot = to_ee.dot(to_target);
            dot = std::max(-1.f, std::min(1.f, dot));
            float angle = std::acos(dot) * 180.f / kPi;

            if (axis.length() > 1e-6f && std::abs(angle) > 0.01f) {
                axis = axis.normalized();
                skeleton->bones[b].rotation.x += axis.x * angle * 0.5f;
                skeleton->bones[b].rotation.y += axis.y * angle * 0.5f;
                skeleton->bones[b].rotation.z += axis.z * angle * 0.5f;
            }
        }
    }
}

void IKChain::solve_fabrik() {
    if (!skeleton || end_effector < 0 || root < 0) return;
    std::vector<int> chain;
    int cur = end_effector;
    while (cur >= 0) { chain.push_back(cur); if (cur == root) break; cur = skeleton->bones[cur].parent; }
    if (chain.size() < 2) return;

    std::vector<Vec3> positions(chain.size());
    std::vector<float> lengths(chain.size() - 1);
    for (size_t i = 0; i < chain.size(); ++i) {
        Mat4 xf = skeleton->world_transform(chain[i]);
        positions[i] = {xf.m[12], xf.m[13], xf.m[14]};
    }
    for (size_t i = 0; i + 1 < chain.size(); ++i)
        lengths[i] = (positions[i+1] - positions[i]).length();

    Vec3 root_pos = positions.back();
    for (int iter = 0; iter < max_iterations; ++iter) {
        positions[0] = target_position;
        for (size_t i = 1; i < chain.size(); ++i) {
            Vec3 d = (positions[i] - positions[i-1]).normalized();
            positions[i] = positions[i-1] + d * lengths[i-1];
        }
        positions.back() = root_pos;
        for (int i = static_cast<int>(chain.size()) - 2; i >= 0; --i) {
            Vec3 d = (positions[i] - positions[i+1]).normalized();
            positions[i] = positions[i+1] + d * lengths[i];
        }
        Vec3 ee_diff = positions[0] - target_position;
        if (ee_diff.length() < tolerance) break;
    }

    for (size_t i = 0; i < chain.size(); ++i) {
        skeleton->bones[chain[i]].head = positions[i];
        if (i + 1 < chain.size())
            skeleton->bones[chain[i]].tail = positions[i+1];
    }
}

void SkinnedMesh::deform(const Skeleton& pose, std::vector<Vec3>& out) const {
    out.resize(rest_positions.size());
    for (size_t i = 0; i < rest_positions.size(); ++i) {
        Vec3 result{0, 0, 0};
        const auto& vs = vertex_weights[i];
        for (auto& w : vs.weights) {
            if (w.bone_idx < 0 || w.bone_idx >= static_cast<int>(pose.bones.size())) continue;
            Mat4 world = pose.world_transform(w.bone_idx);
            Mat4 skin = world;
            if (w.bone_idx < static_cast<int>(bind_pose_inverse.size())) {
                skin = world * bind_pose_inverse[w.bone_idx];
            }
            result += skin.transform_point(rest_positions[i]) * w.weight;
        }
        out[i] = result;
    }
}

}  // namespace smidr
