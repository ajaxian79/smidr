#pragma once

#include "render/GLHeaders.h"
#include "core/Math.h"

#include <string>

namespace smidr {

class Shader {
public:
    Shader() = default;
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& o) noexcept;
    Shader& operator=(Shader&& o) noexcept;

    bool compile(const char* vert_src, const char* frag_src);
    void use() const;

    void set_mat4(const char* name, const Mat4& m) const;
    void set_vec3(const char* name, Vec3 v) const;
    void set_float(const char* name, float v) const;
    void set_int(const char* name, int v) const;

    GLuint id() const { return program_; }

private:
    GLuint program_ = 0;
};

}  // namespace smidr
