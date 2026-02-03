#include "Renderer.h"

namespace shadertoy {

// 全屏四边形顶点数据
static const float quadVertices[] = {
    // positions
    -1.0f,  1.0f,
    -1.0f, -1.0f,
     1.0f, -1.0f,
    -1.0f,  1.0f,
     1.0f, -1.0f,
     1.0f,  1.0f
};

Renderer::Renderer() = default;

Renderer::~Renderer() {
    cleanup();
}

bool Renderer::init() {
    if (m_initialized) return true;

    // 创建 VAO 和 VBO
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // 位置属性
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    glBindVertexArray(0);

    m_initialized = true;
    return true;
}

void Renderer::render(GLuint program) {
    if (!m_initialized) return;

    glUseProgram(program);
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Renderer::renderFullscreenQuad() {
    if (!m_initialized) return;

    // 使用一个空 VAO 来渲染全屏三角形（由顶点着色器生成）
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

void Renderer::cleanup() {
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if (m_vbo) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    m_initialized = false;
}

} // namespace shadertoy
