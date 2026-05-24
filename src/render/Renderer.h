#pragma once

#include "render/GLHeaders.h"
#include "render/Shader.h"
#include "render/Camera.h"
#include "render/Grid.h"
#include "core/MeshGenerator.h"

#include <vector>

namespace smidr {

class Renderer {
public:
    void init();
    void resize(int w, int h);
    void render(const Camera& cam, const MeshGenerator& meshes);
    void cleanup();

    GLuint texture() const { return color_tex_; }
    int width() const { return width_; }
    int height() const { return height_; }

    bool wireframe = false;

private:
    void upload_mesh(const SceneMeshEntry& entry);

    Shader  solid_shader_;
    Grid    grid_;

    GLuint  fbo_       = 0;
    GLuint  color_tex_ = 0;
    GLuint  depth_rbo_ = 0;
    int     width_     = 1;
    int     height_    = 1;

    struct GPUMesh {
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint ebo = 0;
        int    index_count = 0;
    };
    std::vector<GPUMesh> gpu_meshes_;
    void clear_gpu_meshes();
};

}  // namespace smidr
