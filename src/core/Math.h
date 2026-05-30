#pragma once

#include <array>
#include <cmath>


namespace smidr {

struct Vec3 {
    float x = 0.f, y = 0.f, z = 0.f;

    Vec3() = default;
    constexpr Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(Vec3 o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(Vec3 o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vec3 operator*(Vec3 o) const { return {x * o.x, y * o.y, z * o.z}; }
    Vec3 operator-() const { return {-x, -y, -z}; }
    Vec3& operator+=(Vec3 o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(Vec3 o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
    Vec3 operator/(float s) const { return {x / s, y / s, z / s}; }

    float dot(Vec3 o) const { return x * o.x + y * o.y + z * o.z; }
    Vec3  cross(Vec3 o) const {
        return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x};
    }
    float length() const { return std::sqrt(dot(*this)); }
    Vec3  normalized() const {
        float l = length();
        return l > 1e-8f ? Vec3{x / l, y / l, z / l} : Vec3{0, 0, 0};
    }
};

inline Vec3 operator*(float s, Vec3 v) { return v * s; }

struct Mat4 {
    std::array<float, 16> m{};

    Mat4() { m.fill(0.f); m[0] = m[5] = m[10] = m[15] = 1.f; }

    static Mat4 identity() { return {}; }

    static Mat4 translate(Vec3 t) {
        Mat4 r;
        r.m[12] = t.x; r.m[13] = t.y; r.m[14] = t.z;
        return r;
    }

    static Mat4 scale(Vec3 s) {
        Mat4 r;
        r.m[0] = s.x; r.m[5] = s.y; r.m[10] = s.z;
        return r;
    }

    static Mat4 rotate_x(float rad) {
        Mat4 r;
        float c = std::cos(rad), s = std::sin(rad);
        r.m[5] = c; r.m[6] = s; r.m[9] = -s; r.m[10] = c;
        return r;
    }

    static Mat4 rotate_y(float rad) {
        Mat4 r;
        float c = std::cos(rad), s = std::sin(rad);
        r.m[0] = c; r.m[2] = -s; r.m[8] = s; r.m[10] = c;
        return r;
    }

    static Mat4 rotate_z(float rad) {
        Mat4 r;
        float c = std::cos(rad), s = std::sin(rad);
        r.m[0] = c; r.m[1] = s; r.m[4] = -s; r.m[5] = c;
        return r;
    }

    static Mat4 perspective(float fov_rad, float aspect, float near, float far) {
        Mat4 r;
        r.m.fill(0.f);
        float f = 1.f / std::tan(fov_rad * 0.5f);
        r.m[0] = f / aspect;
        r.m[5] = f;
        r.m[10] = (far + near) / (near - far);
        r.m[11] = -1.f;
        r.m[14] = (2.f * far * near) / (near - far);
        return r;
    }

    static Mat4 look_at(Vec3 eye, Vec3 center, Vec3 up) {
        Vec3 f = (center - eye).normalized();
        Vec3 s = f.cross(up).normalized();
        Vec3 u = s.cross(f);
        Mat4 r;
        r.m[0] = s.x;  r.m[4] = s.y;  r.m[8]  = s.z;
        r.m[1] = u.x;  r.m[5] = u.y;  r.m[9]  = u.z;
        r.m[2] = -f.x; r.m[6] = -f.y; r.m[10] = -f.z;
        r.m[12] = -s.dot(eye);
        r.m[13] = -u.dot(eye);
        r.m[14] = f.dot(eye);
        return r;
    }

    Mat4 operator*(const Mat4& b) const {
        Mat4 r;
        r.m.fill(0.f);
        for (int col = 0; col < 4; ++col)
            for (int row = 0; row < 4; ++row)
                for (int k = 0; k < 4; ++k)
                    r.m[col * 4 + row] += m[k * 4 + row] * b.m[col * 4 + k];
        return r;
    }

    Vec3 transform_point(Vec3 p) const {
        float w = m[3] * p.x + m[7] * p.y + m[11] * p.z + m[15];
        return {
            (m[0] * p.x + m[4] * p.y + m[8]  * p.z + m[12]) / w,
            (m[1] * p.x + m[5] * p.y + m[9]  * p.z + m[13]) / w,
            (m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14]) / w
        };
    }

    Vec3 transform_normal(Vec3 n) const {
        return Vec3{
            m[0] * n.x + m[4] * n.y + m[8]  * n.z,
            m[1] * n.x + m[5] * n.y + m[9]  * n.z,
            m[2] * n.x + m[6] * n.y + m[10] * n.z
        }.normalized();
    }

    const float* data() const { return m.data(); }
};

struct AABB {
    Vec3 min_pt{1e30f, 1e30f, 1e30f};
    Vec3 max_pt{-1e30f, -1e30f, -1e30f};

    void expand(Vec3 p) {
        min_pt.x = std::min(min_pt.x, p.x);
        min_pt.y = std::min(min_pt.y, p.y);
        min_pt.z = std::min(min_pt.z, p.z);
        max_pt.x = std::max(max_pt.x, p.x);
        max_pt.y = std::max(max_pt.y, p.y);
        max_pt.z = std::max(max_pt.z, p.z);
    }

    void merge(const AABB& o) { expand(o.min_pt); expand(o.max_pt); }

    Vec3 center() const { return (min_pt + max_pt) * 0.5f; }
    Vec3 extent() const { return (max_pt - min_pt) * 0.5f; }
};

constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 2.f * kPi;
constexpr float kDegToRad = kPi / 180.f;

}  // namespace smidr
