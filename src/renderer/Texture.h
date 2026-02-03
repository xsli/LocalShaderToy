#pragma once

#include <glad/glad.h>
#include <string>

namespace shadertoy {

class Texture {
public:
    Texture() = default;
    ~Texture();

    bool loadFromFile(const std::string& path);
    void bind(int unit = 0) const;
    void unbind() const;
    void cleanup();

    GLuint getId() const { return m_texture; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

private:
    GLuint m_texture = 0;
    int m_width = 0;
    int m_height = 0;
    int m_channels = 0;
};

} // namespace shadertoy
