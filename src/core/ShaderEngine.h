#pragma once

#include <glad/glad.h>
#include <string>
#include <vector>

namespace shadertoy {

class ShaderEngine {
public:
    ShaderEngine() = default;
    ~ShaderEngine();

    // 从转译后的fragment shader代码编译
    bool compileShader(const std::string& fragmentSource, std::string& errorOut);
    
    // 使用当前shader程序
    void use();
    
    // 检查是否有效
    bool isValid() const { return m_program != 0; }
    
    // 获取程序ID
    GLuint getProgram() const { return m_program; }
    
    // 基础编译函数
    bool compileShaderSource(GLenum type, const std::string& source, GLuint& shaderOut, std::string& errorOut);
    bool linkProgram(GLuint vertexShader, GLuint fragmentShader, GLuint& programOut, std::string& errorOut);
    bool createProgram(const std::string& vertexSource, const std::string& fragmentSource, 
                       GLuint& programOut, std::string& errorOut);
    
    // 删除着色器/程序
    void deleteShader(GLuint shader);
    void deleteProgram(GLuint program);

private:
    GLuint m_program = 0;
    std::string m_lastError;
    
    // 默认顶点着色器
    static const char* getDefaultVertexShader();
};

} // namespace shadertoy