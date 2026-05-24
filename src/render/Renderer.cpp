#include "render/Renderer.h"

namespace smidr {

static const char* kSolidVert = R"(
#version 150
in vec3 aPos;
in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
uniform mat3 uNormalMat;

out vec3 vWorldPos;
out vec3 vNormal;

void main() {
    vec4 world = uModel * vec4(aPos, 1.0);
    vWorldPos = world.xyz;
    vNormal = normalize(uNormalMat * aNormal);
    gl_Position = uProj * uView * world;
}
)";

static const char* kSolidFrag = R"(
#version 150
in vec3 vWorldPos;
in vec3 vNormal;

uniform vec3  uColor;
uniform float uOpacity;
uniform vec3  uEyePos;
uniform bool  uSelected;

out vec4 FragColor;

void main() {
    vec3 light_dir = normalize(vec3(0.4, 0.8, 0.6));
    vec3 ambient = uColor * 0.15;
    float diff = max(dot(normalize(vNormal), light_dir), 0.0);
    vec3 diffuse = uColor * diff * 0.7;

    vec3 view_dir = normalize(uEyePos - vWorldPos);
    vec3 half_dir = normalize(light_dir + view_dir);
    float spec = pow(max(dot(normalize(vNormal), half_dir), 0.0), 64.0);
    vec3 specular = vec3(0.3) * spec;

    vec3 color = ambient + diffuse + specular;

    if (uSelected) {
        color = mix(color, vec3(0.3, 0.6, 1.0), 0.25);
    }

    FragColor = vec4(color, uOpacity);
}
)";

void Renderer::init() {
    solid_shader_.compile(kSolidVert, kSolidFrag);
    grid_.init();

    glGenFramebuffers(1, &fbo_);
    glGenTextures(1, &color_tex_);
    glGenRenderbuffers(1, &depth_rbo_);
    resize(800, 600);
}

void Renderer::resize(int w, int h) {
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    width_ = w;
    height_ = h;

    glBindTexture(GL_TEXTURE_2D, color_tex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex_, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::render(const Camera& cam, const MeshGenerator& meshes) {
    clear_gpu_meshes();
    for (auto& entry : meshes.entries())
        upload_mesh(entry);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, width_, height_);
    glClearColor(0.05f, 0.05f, 0.07f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float aspect = static_cast<float>(width_) / static_cast<float>(height_);
    Mat4 view = cam.view_matrix();
    Mat4 proj = cam.projection_matrix(aspect);

    grid_.draw(view, proj);

    if (wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    solid_shader_.use();
    solid_shader_.set_mat4("uView", view);
    solid_shader_.set_mat4("uProj", proj);
    solid_shader_.set_vec3("uEyePos", cam.eye());

    for (size_t i = 0; i < gpu_meshes_.size(); ++i) {
        auto& gm = gpu_meshes_[i];
        auto& entry = meshes.entries()[i];

        solid_shader_.set_mat4("uModel", entry.transform);

        float nm[9] = {
            entry.transform.m[0], entry.transform.m[1], entry.transform.m[2],
            entry.transform.m[4], entry.transform.m[5], entry.transform.m[6],
            entry.transform.m[8], entry.transform.m[9], entry.transform.m[10]
        };
        glUniformMatrix3fv(
            glGetUniformLocation(solid_shader_.id(), "uNormalMat"),
            1, GL_FALSE, nm);

        solid_shader_.set_vec3("uColor", entry.color);
        solid_shader_.set_float("uOpacity", entry.opacity);
        solid_shader_.set_int("uSelected", entry.selected ? 1 : 0);

        glBindVertexArray(gm.vao);
        glDrawElements(GL_TRIANGLES, gm.index_count, GL_UNSIGNED_INT, nullptr);
    }

    if (wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::upload_mesh(const SceneMeshEntry& entry) {
    GPUMesh gm;
    glGenVertexArrays(1, &gm.vao);
    glGenBuffers(1, &gm.vbo);
    glGenBuffers(1, &gm.ebo);

    glBindVertexArray(gm.vao);

    glBindBuffer(GL_ARRAY_BUFFER, gm.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(entry.mesh.vertices.size() * sizeof(Vertex)),
                 entry.mesh.vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(sizeof(Vec3)));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gm.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(entry.mesh.indices.size() * sizeof(uint32_t)),
                 entry.mesh.indices.data(), GL_STATIC_DRAW);

    gm.index_count = static_cast<int>(entry.mesh.indices.size());

    glBindVertexArray(0);
    gpu_meshes_.push_back(gm);
}

void Renderer::clear_gpu_meshes() {
    for (auto& gm : gpu_meshes_) {
        if (gm.vao) glDeleteVertexArrays(1, &gm.vao);
        if (gm.vbo) glDeleteBuffers(1, &gm.vbo);
        if (gm.ebo) glDeleteBuffers(1, &gm.ebo);
    }
    gpu_meshes_.clear();
}

void Renderer::cleanup() {
    clear_gpu_meshes();
    grid_.cleanup();
    if (fbo_)       { glDeleteFramebuffers(1, &fbo_); fbo_ = 0; }
    if (color_tex_) { glDeleteTextures(1, &color_tex_); color_tex_ = 0; }
    if (depth_rbo_) { glDeleteRenderbuffers(1, &depth_rbo_); depth_rbo_ = 0; }
}

}  // namespace smidr
