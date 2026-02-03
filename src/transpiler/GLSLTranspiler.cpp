#include "GLSLTranspiler.h"

#include <regex>
#include <sstream>

namespace shadertoy {

std::string GLSLTranspiler::getUniformDeclarations() {
    return R"(
// Shadertoy uniform declarations
uniform vec3 iResolution;           // viewport resolution (in pixels)
uniform float iTime;                // shader playback time (in seconds)
uniform float iTimeDelta;           // render time (in seconds)
uniform int iFrame;                 // shader playback frame
uniform vec4 iMouse;                // mouse pixel coords. xy: current, zw: click
uniform vec4 iDate;                 // (year, month, day, time in seconds)
uniform float iSampleRate;          // sound sample rate (i.e., 44100)
uniform vec3 iChannelResolution[4]; // channel resolution (in pixels)
uniform float iChannelTime[4];      // channel playback time (in seconds)
uniform sampler2D iChannel0;        // input channel 0
uniform sampler2D iChannel1;        // input channel 1
uniform sampler2D iChannel2;        // input channel 2
uniform sampler2D iChannel3;        // input channel 3
)";
}

std::string GLSLTranspiler::getDefaultVertexShader() {
    return R"(#version 430 core
layout (location = 0) in vec2 aPos;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";
}

std::string GLSLTranspiler::removePrecision(const std::string& code) {
    // 移除 precision 声明
    std::regex precisionRegex(R"(precision\s+(lowp|mediump|highp)\s+\w+\s*;)");
    return std::regex_replace(code, precisionRegex, "");
}

std::string GLSLTranspiler::replaceWebGLFunctions(const std::string& code) {
    std::string result = code;
    
    // texture2D -> texture
    result = std::regex_replace(result, std::regex(R"(\btexture2D\s*\()"), "texture(");
    
    // textureCube -> texture
    result = std::regex_replace(result, std::regex(R"(\btextureCube\s*\()"), "texture(");
    
    // texture2DLod -> textureLod
    result = std::regex_replace(result, std::regex(R"(\btexture2DLod\s*\()"), "textureLod(");
    
    return result;
}

std::string GLSLTranspiler::transpile(const std::string& shadertoyCode) {
    std::stringstream ss;
    
    // 1. 添加版本声明
    ss << "#version 430 core\n";
    ss << "out vec4 FragColor;\n\n";
    
    // 2. 添加 uniform 声明
    ss << getUniformDeclarations();
    ss << "\n";
    
    // 3. 处理 Shadertoy 代码
    std::string processedCode = shadertoyCode;
    
    // 移除已有的版本声明
    processedCode = std::regex_replace(processedCode, std::regex(R"(#version\s+\d+(\s+\w+)?\s*)"), "");
    
    // 移除 precision 声明
    processedCode = removePrecision(processedCode);
    
    // 替换 WebGL 函数
    processedCode = replaceWebGLFunctions(processedCode);
    
    ss << processedCode;
    ss << "\n\n";
    
    // 4. 添加 main 函数包装
    ss << R"(
void main() {
    mainImage(FragColor, gl_FragCoord.xy);
}
)";
    
    return ss.str();
}

} // namespace shadertoy
