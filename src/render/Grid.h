#pragma once

#include "render/GLHeaders.h"
#include "render/Shader.h"

namespace smidr {

class Grid {
public:
    void init();
    void draw(const Mat4& view, const Mat4& proj);
    void cleanup();

private:
    Shader shader_;
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    int    vertex_count_ = 0;
    bool   ready_ = false;
};

}  // namespace smidr
