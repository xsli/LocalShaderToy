#pragma once

#include <string>

namespace shadertoy {

class GLSLTranspiler {
public:
    // 将 Shadertoy GLSL 转换为 OpenGL Core GLSL
    static std::string transpile(const std::string& shadertoyCode);
    
    // 获取默认顶点着色器
    static std::string getDefaultVertexShader();
    
    // 获取 uniform 声明
    static std::string getUniformDeclarations();

private:
    // 移除 precision 声明
    static std::string removePrecision(const std::string& code);
    
    // 替换 WebGL 函数为 OpenGL 函数
    static std::string replaceWebGLFunctions(const std::string& code);
};

} // namespace shadertoy
