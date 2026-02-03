#pragma once

#include <glad/glad.h>

namespace shadertoy {

class Framebuffer {
public:
    Framebuffer() = default;
    ~Framebuffer();

    bool create(int width, int height);
    void bind();
    void unbind();
    void resize(int width, int height);
    void cleanup();

    GLuint getTexture() const { return m_texture; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

private:
    GLuint m_fbo = 0;
    GLuint m_texture = 0;
    int m_width = 0;
    int m_height = 0;
};

} // namespace shadertoy
