#include "render/Shader.h"

#include <cstdio>
#include <utility>

namespace smidr {

Shader::~Shader() {
    if (program_) glDeleteProgram(program_);
}

Shader::Shader(Shader&& o) noexcept : program_(std::exchange(o.program_, 0)) {}

Shader& Shader::operator=(Shader&& o) noexcept {
    if (this != &o) {
        if (program_) glDeleteProgram(program_);
        program_ = std::exchange(o.program_, 0);
    }
    return *this;
}

static GLuint compile_stage(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, sizeof log, nullptr, log);
        std::fprintf(stderr, "shader compile error: %s\n", log);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

bool Shader::compile(const char* vert_src, const char* frag_src) {
    GLuint vs = compile_stage(GL_VERTEX_SHADER, vert_src);
    GLuint fs = compile_stage(GL_FRAGMENT_SHADER, frag_src);
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return false;
    }

    program_ = glCreateProgram();
    glAttachShader(program_, vs);
    glAttachShader(program_, fs);
    glLinkProgram(program_);

    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = 0;
    glGetProgramiv(program_, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(program_, sizeof log, nullptr, log);
        std::fprintf(stderr, "shader link error: %s\n", log);
        glDeleteProgram(program_);
        program_ = 0;
        return false;
    }
    return true;
}

void Shader::use() const {
    glUseProgram(program_);
}

void Shader::set_mat4(const char* name, const Mat4& m) const {
    glUniformMatrix4fv(glGetUniformLocation(program_, name), 1, GL_FALSE, m.data());
}

void Shader::set_vec3(const char* name, Vec3 v) const {
    glUniform3f(glGetUniformLocation(program_, name), v.x, v.y, v.z);
}

void Shader::set_float(const char* name, float v) const {
    glUniform1f(glGetUniformLocation(program_, name), v);
}

void Shader::set_int(const char* name, int v) const {
    glUniform1i(glGetUniformLocation(program_, name), v);
}

}  // namespace smidr
