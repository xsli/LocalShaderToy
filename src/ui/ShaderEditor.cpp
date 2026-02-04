#include "ShaderEditor.h"

#include <imgui.h>
#include <TextEditor.h>

namespace shadertoy {

static TextEditor s_editor;
static ImFont* s_editorFont = nullptr;

// 创建增强的 GLSL 语言定义，支持完整的 GLSL ES 3.0 和 Shadertoy 特性
void ShaderEditor::setupEnhancedGLSL() {
    auto lang = TextEditor::LanguageDefinition::GLSL();
    
    // ========== GLSL 关键字 ==========
    static const char* const glslKeywords[] = {
        // 控制流
        "if", "else", "for", "while", "do", "switch", "case", "default",
        "break", "continue", "return", "discard",
        // 存储限定符
        "const", "in", "out", "inout", "uniform", "varying", "attribute",
        "centroid", "flat", "smooth", "noperspective",
        "layout", "shared", "coherent", "volatile", "restrict", "readonly", "writeonly",
        "buffer", "patch", "sample", "subroutine",
        // 精度限定符
        "precision", "highp", "mediump", "lowp",
        // 结构
        "struct", "void", "true", "false",
        // 版本/扩展
        "invariant",
        nullptr
    };
    
    // ========== GLSL 类型 ==========
    static const char* const glslTypes[] = {
        // 标量类型
        "bool", "int", "uint", "float", "double",
        // 向量类型
        "vec2", "vec3", "vec4",
        "ivec2", "ivec3", "ivec4",
        "uvec2", "uvec3", "uvec4",
        "bvec2", "bvec3", "bvec4",
        "dvec2", "dvec3", "dvec4",
        // 矩阵类型
        "mat2", "mat3", "mat4",
        "mat2x2", "mat2x3", "mat2x4",
        "mat3x2", "mat3x3", "mat3x4",
        "mat4x2", "mat4x3", "mat4x4",
        "dmat2", "dmat3", "dmat4",
        // 采样器类型
        "sampler1D", "sampler2D", "sampler3D", "samplerCube",
        "sampler1DShadow", "sampler2DShadow", "samplerCubeShadow",
        "sampler1DArray", "sampler2DArray", "samplerCubeArray",
        "sampler1DArrayShadow", "sampler2DArrayShadow", "samplerCubeArrayShadow",
        "sampler2DRect", "sampler2DRectShadow",
        "samplerBuffer", "sampler2DMS", "sampler2DMSArray",
        "isampler1D", "isampler2D", "isampler3D", "isamplerCube",
        "isampler1DArray", "isampler2DArray", "isamplerCubeArray",
        "isampler2DRect", "isamplerBuffer", "isampler2DMS", "isampler2DMSArray",
        "usampler1D", "usampler2D", "usampler3D", "usamplerCube",
        "usampler1DArray", "usampler2DArray", "usamplerCubeArray",
        "usampler2DRect", "usamplerBuffer", "usampler2DMS", "usampler2DMSArray",
        // 图像类型
        "image1D", "image2D", "image3D", "imageCube",
        "image1DArray", "image2DArray", "imageCubeArray",
        "image2DRect", "imageBuffer", "image2DMS", "image2DMSArray",
        nullptr
    };
    
    // ========== GLSL 内置函数 ==========
    static const char* const glslBuiltins[] = {
        // 三角函数
        "sin", "cos", "tan", "asin", "acos", "atan", "sinh", "cosh", "tanh",
        "asinh", "acosh", "atanh", "radians", "degrees",
        // 指数函数
        "pow", "exp", "log", "exp2", "log2", "sqrt", "inversesqrt",
        // 常用函数
        "abs", "sign", "floor", "ceil", "trunc", "round", "roundEven",
        "fract", "mod", "modf", "min", "max", "clamp", "mix", "step",
        "smoothstep", "isnan", "isinf", "fma",
        // 几何函数
        "length", "distance", "dot", "cross", "normalize", "faceforward",
        "reflect", "refract",
        // 矩阵函数
        "matrixCompMult", "outerProduct", "transpose", "determinant", "inverse",
        // 向量关系函数
        "lessThan", "lessThanEqual", "greaterThan", "greaterThanEqual",
        "equal", "notEqual", "any", "all", "not",
        // 纹理采样函数
        "texture", "textureProj", "textureLod", "textureOffset",
        "texelFetch", "texelFetchOffset", "textureGrad", "textureGather",
        "textureSize", "textureQueryLod", "textureQueryLevels",
        "texture2D", "texture2DProj", "texture2DLod", "texture2DProjLod",
        "textureCube", "textureCubeLod",
        // 噪声函数（旧版）
        "noise1", "noise2", "noise3", "noise4",
        // 导数函数
        "dFdx", "dFdy", "dFdxFine", "dFdyFine", "dFdxCoarse", "dFdyCoarse", "fwidth",
        // 其他
        "floatBitsToInt", "floatBitsToUint", "intBitsToFloat", "uintBitsToFloat",
        "packSnorm2x16", "packUnorm2x16", "packSnorm4x8", "packUnorm4x8",
        "unpackSnorm2x16", "unpackUnorm2x16", "unpackSnorm4x8", "unpackUnorm4x8",
        "packDouble2x32", "unpackDouble2x32", "packHalf2x16", "unpackHalf2x16",
        nullptr
    };
    
    // ========== Shadertoy 特有标识符 ==========
    static const char* const shadertoyIdentifiers[] = {
        // Shadertoy uniforms
        "iResolution", "iTime", "iTimeDelta", "iFrame", "iChannelTime",
        "iChannelResolution", "iMouse", "iChannel0", "iChannel1", 
        "iChannel2", "iChannel3", "iDate", "iSampleRate",
        // 常用入参
        "fragCoord", "fragColor", "gl_FragCoord", "gl_FragColor",
        // 常用别名
        "uv", "st", "coord", "p", "pos", "col", "color", "time",
        nullptr
    };
    
    // 添加关键字（覆盖默认 C 风格关键字）
    for (auto k = glslKeywords; *k != nullptr; ++k) {
        lang.mKeywords.insert(*k);
    }
    
    // 添加类型作为关键字（使用关键字颜色）
    for (auto t = glslTypes; *t != nullptr; ++t) {
        lang.mKeywords.insert(*t);
    }
    
    // 添加内置函数作为标识符（特殊颜色）
    TextEditor::Identifier id;
    id.mDeclaration = "GLSL built-in function";
    for (auto f = glslBuiltins; *f != nullptr; ++f) {
        lang.mIdentifiers.insert(std::make_pair(std::string(*f), id));
    }
    
    // 添加 Shadertoy 标识符
    id.mDeclaration = "Shadertoy uniform/variable";
    for (auto s = shadertoyIdentifiers; *s != nullptr; ++s) {
        lang.mIdentifiers.insert(std::make_pair(std::string(*s), id));
    }
    
    // 预处理器指令
    lang.mPreprocChar = '#';
    
    s_editor.SetLanguageDefinition(lang);
}

void ShaderEditor::init() {
    setupEnhancedGLSL();
    s_editor.SetShowWhitespaces(false);
    s_editor.SetTabSize(4);
}

void ShaderEditor::render() {
    ImGui::Begin("Shader Editor");
    
    if (ImGui::Button("Compile (F5)") || ImGui::IsKeyPressed(ImGuiKey_F5)) {
        if (m_compileCallback) {
            m_compileCallback(getText());
        }
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        // TODO: Reset to default shader
    }
    
    // 使用专用字体渲染编辑器
    if (s_editorFont) {
        ImGui::PushFont(s_editorFont);
    }
    
    s_editor.Render("Editor");
    
    if (s_editorFont) {
        ImGui::PopFont();
    }
    
    ImGui::End();
}

void ShaderEditor::setText(const std::string& text) {
    m_text = text;
    s_editor.SetText(text);
}

std::string ShaderEditor::getText() const {
    return s_editor.GetText();
}

void ShaderEditor::setErrorMarkers(const std::string& errors) {
    // TODO: Parse error string and set markers
    TextEditor::ErrorMarkers markers;
    // Simple parsing for line numbers
    // markers[lineNumber] = "Error message";
    s_editor.SetErrorMarkers(markers);
}

void ShaderEditor::clearErrorMarkers() {
    TextEditor::ErrorMarkers markers;
    s_editor.SetErrorMarkers(markers);
}

void setEditorFont(ImFont* font) {
    s_editorFont = font;
}

} // namespace shadertoy