#include "render/Grid.h"

#include <vector>

namespace smidr {

static const char* kGridVert = R"(
#version 150
in vec3 aPos;
in vec3 aCol;
out vec3 vCol;
uniform mat4 uVP;
void main() {
    gl_Position = uVP * vec4(aPos, 1.0);
    vCol = aCol;
}
)";

static const char* kGridFrag = R"(
#version 150
in vec3 vCol;
out vec4 FragColor;
void main() {
    FragColor = vec4(vCol, 0.35);
}
)";

void Grid::init() {
    if (ready_) return;
    if (!shader_.compile(kGridVert, kGridFrag)) return;

    struct V { float x, y, z, r, g, b; };
    std::vector<V> verts;

    constexpr int   N    = 20;
    constexpr float span = 20.f;
    constexpr float step = span / N;

    for (int i = -N; i <= N; ++i) {
        float t = static_cast<float>(i) * step;
        float r = 0.25f, g = 0.25f, b = 0.28f;
        if (i == 0) { r = 0.4f; g = 0.4f; b = 0.45f; }
        verts.push_back({t, 0, -span, r, g, b});
        verts.push_back({t, 0,  span, r, g, b});
        verts.push_back({-span, 0, t, r, g, b});
        verts.push_back({ span, 0, t, r, g, b});
    }

    float axis_len = span * 0.5f;
    verts.push_back({0, 0, 0, 0.85f, 0.2f, 0.2f});
    verts.push_back({axis_len, 0, 0, 0.85f, 0.2f, 0.2f});
    verts.push_back({0, 0, 0, 0.2f, 0.85f, 0.2f});
    verts.push_back({0, axis_len, 0, 0.2f, 0.85f, 0.2f});
    verts.push_back({0, 0, 0, 0.2f, 0.2f, 0.85f});
    verts.push_back({0, 0, axis_len, 0.2f, 0.2f, 0.85f});

    vertex_count_ = static_cast<int>(verts.size());

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(verts.size() * sizeof(V)),
                 verts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(V), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(V),
                          reinterpret_cast<void*>(3 * sizeof(float)));
    glBindVertexArray(0);

    ready_ = true;
}

void Grid::draw(const Mat4& view, const Mat4& proj) {
    if (!ready_) return;
    shader_.use();
    Mat4 vp = proj * view;
    shader_.set_mat4("uVP", vp);
    glBindVertexArray(vao_);
    glDrawArrays(GL_LINES, 0, vertex_count_);
    glBindVertexArray(0);
}

void Grid::cleanup() {
    if (vao_) { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
    if (vbo_) { glDeleteBuffers(1, &vbo_); vbo_ = 0; }
    ready_ = false;
}

}  // namespace smidr
