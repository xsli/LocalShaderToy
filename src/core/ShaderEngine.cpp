#include "ShaderEngine.h"
#include <iostream>
#include <algorithm>

namespace shadertoy {

// 默认顶点着色器 - 简单的全屏四边形
const char* ShaderEngine::getDefaultVertexShader() {
    return R"(
#version 430 core

out vec2 fragCoord;

void main() {
    // 全屏三角形
    float x = float((gl_VertexID & 1) << 2) - 1.0;
    float y = float((gl_VertexID & 2) << 1) - 1.0;
    fragCoord = vec2((x + 1.0) * 0.5, (y + 1.0) * 0.5);
    gl_Position = vec4(x, y, 0.0, 1.0);
}
)";
}

ShaderEngine::~ShaderEngine() {
    if (m_program != 0) {
        glDeleteProgram(m_program);
    }
}

bool ShaderEngine::compileShader(const std::string& fragmentSource, std::string& errorOut) {
    GLuint newProgram = 0;
    
    if (!createProgram(getDefaultVertexShader(), fragmentSource, newProgram, errorOut)) {
        return false;
    }
    
    // 删除旧程序
    if (m_program != 0) {
        glDeleteProgram(m_program);
    }
    
    m_program = newProgram;
    return true;
}

void ShaderEngine::use() {
    if (m_program != 0) {
        glUseProgram(m_program);
    }
}

bool ShaderEngine::compileShaderSource(GLenum type, const std::string& source, GLuint& shaderOut, std::string& errorOut) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    
    if (!success) {
        char infoLog[2048];
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
        errorOut = infoLog;
        glDeleteShader(shader);
        return false;
    }

    shaderOut = shader;
    return true;
}

bool ShaderEngine::linkProgram(GLuint vertexShader, GLuint fragmentShader, GLuint& programOut, std::string& errorOut) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    
    if (!success) {
        char infoLog[2048];
        glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
        errorOut = infoLog;
        glDeleteProgram(program);
        return false;
    }

    programOut = program;
    return true;
}

bool ShaderEngine::createProgram(const std::string& vertexSource, const std::string& fragmentSource, 
                                  GLuint& programOut, std::string& errorOut) {
    GLuint vertexShader = 0, fragmentShader = 0;
    
    if (!compileShaderSource(GL_VERTEX_SHADER, vertexSource, vertexShader, errorOut)) {
        errorOut = "Vertex shader error:\n" + errorOut;
        return false;
    }
    
    if (!compileShaderSource(GL_FRAGMENT_SHADER, fragmentSource, fragmentShader, errorOut)) {
        errorOut = "Fragment shader error:\n" + errorOut;
        glDeleteShader(vertexShader);
        return false;
    }
    
    bool success = linkProgram(vertexShader, fragmentShader, programOut, errorOut);
    
    // 着色器已链接，可以删除
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return success;
}

void ShaderEngine::deleteShader(GLuint shader) {
    glDeleteShader(shader);
}

void ShaderEngine::deleteProgram(GLuint program) {
    glDeleteProgram(program);
}

} // namespace shadertoy