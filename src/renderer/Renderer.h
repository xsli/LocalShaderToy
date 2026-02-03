#pragma once

#include <glad/glad.h>

namespace shadertoy {

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool init();
    void render(GLuint program);
    void renderFullscreenQuad();  // 不切换程序，只绘制
    void cleanup();

private:
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    bool m_initialized = false;
};

} // namespace shadertoy
