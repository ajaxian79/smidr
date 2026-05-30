#pragma once

#include "core/Math.h"

#include <cstdint>
#include <string>
#include <vector>

namespace smidr {

enum class CurveInterp { Step, Linear, Bezier, Hermite, CatmullRom };

struct CurveKey {
    float time = 0.f;
    float value = 0.f;
    CurveInterp interp = CurveInterp::Linear;
    float in_tangent = 0.f;
    float out_tangent = 0.f;
};

class AnimCurve {
public:
    std::vector<CurveKey> keys;

    float sample(float t) const;
    void  add_key(CurveKey k);
    void  sort_keys();
    void  remove_key(int idx);
    int   key_count() const { return static_cast<int>(keys.size()); }

private:
    float bezier_interp(const CurveKey& a, const CurveKey& b, float u) const;
    float hermite_interp(const CurveKey& a, const CurveKey& b, float u) const;
};

struct Bone {
    std::string name;
    int  parent = -1;
    Vec3 head{0, 0, 0};
    Vec3 tail{0, 1, 0};
    Vec3 rotation{0, 0, 0};
    float length() const { return (tail - head).length(); }
};

class Skeleton {
public:
    std::vector<Bone> bones;

    int add_bone(const std::string& name, Vec3 head, Vec3 tail, int parent = -1);
    Mat4 world_transform(int bone_idx) const;
    void update_from_curves(const std::vector<AnimCurve>& curves, float time);
};

class IKChain {
public:
    Skeleton* skeleton = nullptr;
    int  end_effector = -1;
    int  root = -1;
    Vec3 target_position;
    int  max_iterations = 16;
    float tolerance = 1e-3f;

    void solve_ccd();
    void solve_fabrik();
};

struct SkinWeight {
    int   bone_idx;
    float weight;
};

struct VertexSkin {
    std::vector<SkinWeight> weights;
};

class SkinnedMesh {
public:
    std::vector<Vec3>       rest_positions;
    std::vector<VertexSkin> vertex_weights;
    std::vector<Mat4>       bind_pose_inverse;

    void deform(const Skeleton& current_pose, std::vector<Vec3>& out_positions) const;
};

}  // namespace smidr
