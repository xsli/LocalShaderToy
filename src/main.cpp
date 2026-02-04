/**
 * Local Shadertoy - A local OpenGL implementation of Shadertoy
 * 
 * Main entry point
 * Supports both Editor mode and Windows Screensaver mode (.scr)
 */

#include "core/Application.h"
#include "core/ShaderEngine.h"
#include "core/UniformManager.h"
#include "core/ProjectManager.h"
#include "core/ScreensaverMode.h"
#include "transpiler/GLSLTranspiler.h"
#include "renderer/Renderer.h"
#include "renderer/TextureManager.h"
#include "ui/UIManager.h"
#include "ui/ShaderEditor.h"
#include "utils/FileDialog.h"
#include "renderer/BufferManager.h"
#include "renderer/MultiPassRenderer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <imgui.h>
#include <imgui_internal.h>  // 需要 ImGuiTabItemFlags_NoCloseButton
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <TextEditor.h>

#include <iostream>
#include <string>
#include <cmath>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <ctime>

using namespace shadertoy;

// 编辑器专用字体
static ImFont* g_editorFont = nullptr;

// 创建增强的 GLSL 语言定义，支持完整的 GLSL ES 3.0 和 Shadertoy 特性
TextEditor::LanguageDefinition getEnhancedGLSLDefinition() {
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
        "sampler2DRect", "samplerBuffer", "sampler2DMS", "sampler2DMSArray",
        "isampler1D", "isampler2D", "isampler3D", "isamplerCube",
        "usampler1D", "usampler2D", "usampler3D", "usamplerCube",
        // 图像类型
        "image1D", "image2D", "image3D", "imageCube",
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
        "texture2D", "texture2DProj", "texture2DLod", "textureCube",
        // 导数函数
        "dFdx", "dFdy", "dFdxFine", "dFdyFine", "dFdxCoarse", "dFdyCoarse", "fwidth",
        // 其他
        "floatBitsToInt", "floatBitsToUint", "intBitsToFloat", "uintBitsToFloat",
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
        nullptr
    };
    
    // 添加关键字
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
    
    return lang;
}

// 运行模式
static ScreensaverRunMode g_runMode = ScreensaverRunMode::Editor;
static HWND g_previewHwnd = nullptr;
static ScreensaverConfig g_scrConfig;

// 屏保退出检测
static double g_initialMouseX = 0.0;
static double g_initialMouseY = 0.0;
static bool g_mouseInitialized = false;
static const double MOUSE_MOVE_THRESHOLD = 10.0;

// 随机播放状态
static float g_randomTimer = 0.0f;
static int g_currentRandomIndex = -1;

// Multi-pass 编辑器状态
struct PassEditorState {
    ShaderPassType type = ShaderPassType::Image;
    TextEditor editor;
    std::array<int, 4> channels = {-1, -1, -1, -1};  // iChannel 绑定
    bool enabled = true;
    bool needsCompile = false;
    
    PassEditorState() = default;
    PassEditorState(ShaderPassType t) : type(t) {
        // 设置增强的 GLSL 语法高亮
        auto lang = getEnhancedGLSLDefinition();
        editor.SetLanguageDefinition(lang);
        editor.SetTabSize(4);
    }
};

// 全局状态
struct AppState {
    ProjectManager projectManager;
    ShaderEngine shaderEngine;         // 向后兼容的单 Pass 引擎
    MultiPassRenderer multiPassRenderer; // 新的多 Pass 渲染器
    UniformManager uniformManager;
    Renderer renderer;
    GLSLTranspiler transpiler;
    
    // Multi-pass 编辑器 (保留单一 editor 用于向后兼容)
    TextEditor editor;  // 当前激活的编辑器引用（指向 passEditors 中的一个）
    std::vector<PassEditorState> passEditors;  // 每个 Pass 的编辑器
    int activePassIndex = 0;                    // 当前激活的 Tab 索引
    
    bool showEditor = true;
    bool showControls = true;
    bool needsRecompile = false;
    std::string lastError;
    float fps = 0.0f;
    int frameCount = 0;
    float fpsTimer = 0.0f;
    
    // iChannel 绑定 (-1 = None, 0+ = 内置纹理索引)
    // 向后兼容：这是 Image pass 的 channel 绑定
    int channelBindings[4] = {-1, -1, -1, -1};
    
    // 屏保配置管理
    bool showProfileManager = false;     // 显示配置管理弹窗
    bool showSaveProfileDialog = false;  // 显示保存配置对话框
    char newProfileName[128] = "";       // 新配置名称输入
    
    // Buffer 调试模式
    // -1 = 关闭, 0 = Buffer A, 1 = Buffer B, 2 = Buffer C, 3 = Buffer D
    int debugBufferIndex = -1;
    
    // 初始化默认 Pass（至少有 Image）
    void initDefaultPasses() {
        if (passEditors.empty()) {
            passEditors.emplace_back(ShaderPassType::Image);
        }
    }
    
    // 获取当前激活的编辑器
    PassEditorState* getActivePassEditor() {
        if (activePassIndex >= 0 && activePassIndex < static_cast<int>(passEditors.size())) {
            return &passEditors[static_cast<size_t>(activePassIndex)];
        }
        return passEditors.empty() ? nullptr : &passEditors[0];
    }
    
    // 根据类型获取 Pass
    PassEditorState* getPassEditor(ShaderPassType type) {
        for (auto& p : passEditors) {
            if (p.type == type) return &p;
        }
        return nullptr;
    }
    
    // 添加 Pass
    PassEditorState* addPass(ShaderPassType type) {
        // 检查是否已存在
        if (getPassEditor(type)) return getPassEditor(type);
        passEditors.emplace_back(type);
        return &passEditors.back();
    }
    
    // 移除 Pass (Image 不可删)
    bool removePass(ShaderPassType type) {
        if (type == ShaderPassType::Image) return false;
        for (auto it = passEditors.begin(); it != passEditors.end(); ++it) {
            if (it->type == type) {
                passEditors.erase(it);
                // 调整激活索引
                if (activePassIndex >= static_cast<int>(passEditors.size())) {
                    activePassIndex = static_cast<int>(passEditors.size()) - 1;
                }
                return true;
            }
        }
        return false;
    }
    
    // 检查是否有指定类型的 Pass
    bool hasPass(ShaderPassType type) const {
        for (const auto& p : passEditors) {
            if (p.type == type) return true;
        }
        return false;
    }
    
    // 获取 Common 代码（用于合并到其他 Pass）
    std::string getCommonCode() const {
        for (const auto& p : passEditors) {
            if (p.type == ShaderPassType::Common) {
                return p.editor.GetText();
            }
        }
        return "";
    }
    
    // 同步 channelBindings 到/从 Image pass
    void syncChannelBindings() {
        auto* imagePass = getPassEditor(ShaderPassType::Image);
        if (imagePass) {
            for (int i = 0; i < 4; i++) {
                channelBindings[i] = imagePass->channels[i];
            }
        }
    }
    
    // 将编辑器内容同步到 Profile (用于保存)
    void syncToProfile(ScreensaverProfile& profile) const {
        profile.passes.clear();
        
        for (const auto& passEditor : passEditors) {
            PassConfig passConfig;
            passConfig.type = passEditor.type;
            passConfig.code = passEditor.editor.GetText();
            passConfig.enabled = true;
            passConfig.channels = passEditor.channels;
            profile.passes.push_back(passConfig);
        }
        
        // 同时更新旧格式字段（向后兼容）
        profile.syncToLegacy();
    }
};

static AppState* g_state = nullptr;

// Forward declarations
int runEditorMode();
int runScreensaverMode();
int runConfigureMode();
int runPreviewMode();

// 加载 Profile 到 Multi-pass 编辑器
// 单 Pass profile 的代码会直接加载到 Image Tab
// 多 Pass profile 会创建相应的 Tab 并填充代码
void loadProfileToEditors(AppState& state, const ScreensaverProfile& profile) {
    // 1. 清除所有非 Image 的 Pass
    state.passEditors.erase(
        std::remove_if(state.passEditors.begin(), state.passEditors.end(),
            [](const PassEditorState& p) { return p.type != ShaderPassType::Image; }),
        state.passEditors.end()
    );
    
    // 确保至少有 Image pass
    state.initDefaultPasses();
    
    // 2. 检查是否有多 Pass 数据
    bool hasMultiPass = false;
    for (const auto& passConfig : profile.passes) {
        if (!passConfig.code.empty()) {
            hasMultiPass = true;
            break;
        }
    }
    
    if (hasMultiPass) {
        // 多 Pass profile - 创建各个 Pass Tab
        for (const auto& passConfig : profile.passes) {
            if (passConfig.code.empty()) continue;
            
            PassEditorState* passEditor = state.getPassEditor(passConfig.type);
            if (!passEditor) {
                passEditor = state.addPass(passConfig.type);
            }
            
            if (passEditor) {
                passEditor->editor.SetText(passConfig.code);
                passEditor->channels = passConfig.channels;
            }
        }
    } else {
        // 单 Pass profile - 代码放入 Image Tab
        auto* imagePass = state.getPassEditor(ShaderPassType::Image);
        if (imagePass) {
            imagePass->editor.SetText(profile.shaderCode);
            for (int i = 0; i < 4; i++) {
                imagePass->channels[i] = profile.channelBindings[i];
            }
        }
    }
    
    // 3. 同步 channelBindings（向后兼容）
    state.syncChannelBindings();
    
    // 4. 触发重编译
    state.needsRecompile = true;
    
    std::cout << "Loaded profile: " << profile.name 
              << (hasMultiPass ? " (multi-pass)" : " (single-pass)") << std::endl;
}

// 编译shader (向后兼容的单 Pass 编译)
bool compileCurrentShader(AppState& state, const std::string& code) {
    // 转译代码
    std::string transpiledCode = state.transpiler.transpile(code);
    
    // 编译
    std::string error;
    bool success = state.shaderEngine.compileShader(transpiledCode, error);
    
    if (!success) {
        state.lastError = error;
        std::cerr << "Shader compilation error:\n" << error << std::endl;
    } else {
        state.lastError.clear();
        std::cout << "Shader compiled successfully!" << std::endl;
    }
    
    return success;
}

// 多 Pass 编译 - 编译所有启用的 Pass
bool compileAllPasses(AppState& state, int width, int height) {
    // 初始化 MultiPassRenderer（如果尚未初始化）
    if (state.multiPassRenderer.getWidth() != width || 
        state.multiPassRenderer.getHeight() != height) {
        state.multiPassRenderer.init(width, height);
    }
    
    // 首先设置 Common 代码
    std::string commonCode = state.getCommonCode();
    state.multiPassRenderer.setCommonCode(commonCode);
    
    // 调试输出
    if (!commonCode.empty()) {
        std::cout << "Common code found (" << commonCode.length() << " chars)" << std::endl;
    } else {
        std::cout << "No Common code (Common tab not added or empty)" << std::endl;
    }
    
    bool allSuccess = true;
    state.lastError.clear();
    
    // 编译每个 Pass
    for (auto& passState : state.passEditors) {
        // Common 不单独编译
        if (passState.type == ShaderPassType::Common) {
            continue;
        }
        
        std::string code = passState.editor.GetText();
        
        // 如果代码为空，禁用该 pass
        if (code.empty() || code.find_first_not_of(" \t\n\r") == std::string::npos) {
            state.multiPassRenderer.disablePass(passState.type);
            passState.enabled = false;
            continue;
        }
        
        // 编译 pass
        bool success = state.multiPassRenderer.compilePass(
            passState.type, 
            code, 
            passState.channels
        );
        
        if (success) {
            passState.enabled = true;
            std::cout << "Pass " << PassConfig::getTypeName(passState.type) 
                      << " compiled successfully" << std::endl;
        } else {
            passState.enabled = false;
            allSuccess = false;
            
            // 收集错误
            std::string passError = state.multiPassRenderer.getPassError(passState.type);
            if (!passError.empty()) {
                if (!state.lastError.empty()) {
                    state.lastError += "\n\n";
                }
                state.lastError += passError;
            }
        }
    }
    
    // 同时编译向后兼容的单 Pass (Image)
    auto* imagePass = state.getPassEditor(ShaderPassType::Image);
    if (imagePass && !imagePass->editor.GetText().empty()) {
        std::string fullCode = commonCode.empty() ? 
            imagePass->editor.GetText() : 
            commonCode + "\n\n" + imagePass->editor.GetText();
        compileCurrentShader(state, fullCode);
    }
    
    return allSuccess;
}

// 初始化ImGui
void initImGui(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    
    // 加载 Consolas 字体用于代码编辑器 (Windows 系统自带)
    const char* fontPath = "C:\\Windows\\Fonts\\consola.ttf";
    g_editorFont = io.Fonts->AddFontFromFileTTF(fontPath, 16.0f);
    if (!g_editorFont) {
        // 如果加载失败，回退到默认字体
        g_editorFont = io.Fonts->AddFontDefault();
        std::cout << "Warning: Could not load Consolas font, using default" << std::endl;
    } else {
        std::cout << "Loaded Consolas font for code editor" << std::endl;
    }
    
    // 添加默认字体作为 UI 字体（作为第二个字体）
    io.Fonts->AddFontDefault();
    
    ImGui::StyleColorsDark();
    
    // 自定义样式
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 3.0f;
    style.ScrollbarRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    
    // 颜色主题
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.12f, 0.95f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.17f, 1.0f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(0.25f, 0.25f, 0.28f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.35f, 0.4f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.45f, 0.45f, 0.5f, 1.0f);
    
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 430");
}

// 渲染UI
void renderUI(AppState& state, Application& app) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    // 主菜单栏
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New", "Ctrl+N")) {
                state.projectManager.newProject();
                state.editor.SetText(state.projectManager.getProject().getImageCode());
                state.needsRecompile = true;
            }
            if (ImGui::MenuItem("Open...", "Ctrl+O")) {
                std::string path = FileDialog::openFile("Open Shader", FileDialog::projectFilters());
                if (!path.empty()) {
                    if (state.projectManager.loadProject(path)) {
                        state.editor.SetText(state.projectManager.getProject().getImageCode());
                        state.needsRecompile = true;
                    }
                }
            }
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                state.projectManager.getProject().setImageCode(state.editor.GetText());
                if (state.projectManager.getProjectPath().empty()) {
                    std::string path = FileDialog::saveFile("Save Shader", FileDialog::projectFilters(), "", "shader.json");
                    if (!path.empty()) {
                        state.projectManager.saveProject(path);
                    }
                } else {
                    state.projectManager.saveProject();
                }
            }
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
                state.projectManager.getProject().setImageCode(state.editor.GetText());
                std::string path = FileDialog::saveFile("Save Shader As", FileDialog::projectFilters(), "", "shader.json");
                if (!path.empty()) {
                    state.projectManager.saveProjectAs(path);
                }
            }
            ImGui::Separator();
            
            // 屏保配置子菜单
            if (ImGui::BeginMenu("Screensaver")) {
                if (ImGui::MenuItem("Save as Screensaver Profile...")) {
                    state.showSaveProfileDialog = true;
                    // 使用项目名称作为默认配置名
                    std::string projectName = state.projectManager.getProjectName();
                    strncpy_s(state.newProfileName, projectName.c_str(), sizeof(state.newProfileName) - 1);
                }
                if (ImGui::MenuItem("Manage Profiles...")) {
                    // 加载当前配置
                    ScreensaverMode::loadConfig(g_scrConfig);
                    state.showProfileManager = true;
                }
                ImGui::Separator();
                
                // 显示已保存的配置列表
                ScreensaverMode::loadConfig(g_scrConfig);
                if (!g_scrConfig.profiles.empty()) {
                    ImGui::Text("Active Profile:");
                    for (int i = 0; i < static_cast<int>(g_scrConfig.profiles.size()); i++) {
                        bool isActive = (i == g_scrConfig.activeProfileIndex);
                        std::string label = g_scrConfig.profiles[static_cast<size_t>(i)].name;
                        if (isActive) {
                            label = "[*] " + label;
                        }
                        if (ImGui::MenuItem(label.c_str(), nullptr, isActive)) {
                            g_scrConfig.activeProfileIndex = i;
                            ScreensaverMode::saveConfig(g_scrConfig);
                            
                            // 使用统一函数加载 profile 到 Multi-pass 编辑器
                            const auto& profile = g_scrConfig.profiles[static_cast<size_t>(i)];
                            loadProfileToEditors(state, profile);
                            // 加载 Profile 后重置时间
                            app.resetTime();
                            state.multiPassRenderer.getBufferManager().clearAll();
                        }
                    }
                } else {
                    ImGui::TextDisabled("(No saved profiles)");
                }
                
                ImGui::EndMenu();
            }
            
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Esc")) {
                app.requestClose();
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Paste Shadertoy Code", "Ctrl+V")) {
                // 从剪贴板获取
                const char* clipboardText = ImGui::GetClipboardText();
                if (clipboardText) {
                    state.projectManager.loadFromText(clipboardText);
                    state.editor.SetText(state.projectManager.getProject().getImageCode());
                    state.needsRecompile = true;
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Compile", "F5")) {
                state.needsRecompile = true;
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Editor", nullptr, &state.showEditor);
            ImGui::MenuItem("Controls", nullptr, &state.showControls);
            ImGui::Separator();
            
            // Debug Buffer 下拉菜单
            if (ImGui::BeginMenu("Debug Buffer")) {
                int currentDebug = state.multiPassRenderer.getDebugBuffer();
                
                // Off 选项
                if (ImGui::MenuItem("Off", nullptr, currentDebug == -1)) {
                    state.multiPassRenderer.setDebugBuffer(-1);
                }
                
                ImGui::Separator();
                
                // Buffer A/B/C/D 选项
                const char* bufferNames[] = {"Buffer A", "Buffer B", "Buffer C", "Buffer D"};
                for (int i = 0; i < 4; i++) {
                    bool isEnabled = state.multiPassRenderer.getBufferManager().isEnabled(i);
                    if (isEnabled) {
                        if (ImGui::MenuItem(bufferNames[i], nullptr, currentDebug == i)) {
                            state.multiPassRenderer.setDebugBuffer(i);
                        }
                    } else {
                        // 未启用的 Buffer 显示为灰色
                        ImGui::BeginDisabled();
                        ImGui::MenuItem(bufferNames[i], nullptr, false);
                        ImGui::EndDisabled();
                    }
                }
                
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Playback")) {
            if (ImGui::MenuItem(app.isPaused() ? "Play" : "Pause", "Space")) {
                app.togglePause();
            }
            if (ImGui::MenuItem("Reset Time", "R")) {
                app.resetTime();
            }
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
    
    // Shader编辑器窗口 (Multi-pass Tab 界面)
    if (state.showEditor) {
        ImGui::SetNextWindowSize(ImVec2(700, 500), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(20, 40), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("Shader Editor", &state.showEditor, ImGuiWindowFlags_MenuBar)) {
            // 菜单栏
            if (ImGui::BeginMenuBar()) {
                if (ImGui::Button("Compile (F5)")) {
                    state.needsRecompile = true;
                }
                ImGui::EndMenuBar();
            }
            
            // ============================================================
            // Tab Bar - Multi-pass 编辑
            // ============================================================
            
            // 确保至少有 Image pass
            state.initDefaultPasses();
            
            ImGuiTabBarFlags tabBarFlags = ImGuiTabBarFlags_AutoSelectNewTabs | 
                                            ImGuiTabBarFlags_FittingPolicyResizeDown;  // 移除 Reorderable，使用固定顺序
            
            if (ImGui::BeginTabBar("ShaderPasses", tabBarFlags)) {
                // 固定的 Tab 排序顺序：Common -> Buffer A -> B -> C -> D -> Image
                static const ShaderPassType tabOrder[] = {
                    ShaderPassType::Common,
                    ShaderPassType::BufferA,
                    ShaderPassType::BufferB,
                    ShaderPassType::BufferC,
                    ShaderPassType::BufferD,
                    ShaderPassType::Image
                };
                
                // 按固定顺序渲染 Pass Tabs
                for (const auto& orderedType : tabOrder) {
                    // 查找该类型的 Pass 索引
                    int i = -1;
                    for (int j = 0; j < static_cast<int>(state.passEditors.size()); j++) {
                        if (state.passEditors[static_cast<size_t>(j)].type == orderedType) {
                            i = j;
                            break;
                        }
                    }
                    if (i < 0) continue;  // 该类型不存在，跳过
                    
                    PassEditorState& passState = state.passEditors[static_cast<size_t>(i)];
                    
                    // Tab 标签
                    const char* tabName = PassConfig::getTypeName(passState.type);
                    bool isOpen = true;
                    
                    // Image Tab 不可关闭
                    ImGuiTabItemFlags tabFlags = ImGuiTabItemFlags_None;
                    if (passState.type == ShaderPassType::Image) {
                        tabFlags |= ImGuiTabItemFlags_NoCloseButton;
                    }
                    
                    // 设置 Tab 高亮样式
                    bool isCurrentActive = (i == state.activePassIndex);
                    if (isCurrentActive) {
                        // 激活 Tab：明亮的蓝色背景
                        ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.2f, 0.4f, 0.8f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.3f, 0.5f, 0.9f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.4f, 0.6f, 1.0f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                    } else {
                        // 非激活 Tab：灰暗背景
                        ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.2f, 0.2f, 0.25f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.25f, 0.3f, 0.4f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                    }
                    
                    // 渲染 Tab
                    bool tabActive = ImGui::BeginTabItem(tabName, 
                        passState.type != ShaderPassType::Image ? &isOpen : nullptr,
                        tabFlags);
                    
                    ImGui::PopStyleColor(4);
                    
                    // Tab 右键菜单
                    if (ImGui::BeginPopupContextItem()) {
                        if (passState.type != ShaderPassType::Image) {
                            if (ImGui::MenuItem("Remove Pass")) {
                                state.removePass(passState.type);
                                ImGui::EndPopup();
                                if (tabActive) ImGui::EndTabItem();
                                continue;  // 跳过已删除的 tab
                            }
                        }
                        ImGui::EndPopup();
                    }
                    
                    if (tabActive) {
                        state.activePassIndex = i;
                        
                        // 渲染当前 Tab 的编辑器
                        ImGui::PushID(i);
                        
                        // 如果不是 Common，显示 iChannel 绑定
                        if (passState.type != ShaderPassType::Common) {
                            // 紧凑型 iChannel 绑定选择器
                            auto& texMgr = TextureManager::instance();
                            const auto& builtins = texMgr.getBuiltinTextures();
                            
                            for (int ch = 0; ch < 4; ch++) {
                                ImGui::PushID(ch);
                                
                                // 当前绑定名称
                                const char* currentName = "None";
                                if (passState.channels[ch] >= ChannelBind::BufferA) {
                                    // Buffer 绑定
                                    int bufIdx = passState.channels[ch] - ChannelBind::BufferA;
                                    static const char* bufNames[] = {"Buf A", "Buf B", "Buf C", "Buf D"};
                                    if (bufIdx >= 0 && bufIdx < 4) currentName = bufNames[bufIdx];
                                } else if (passState.channels[ch] >= 0 && 
                                           passState.channels[ch] < static_cast<int>(builtins.size())) {
                                    currentName = builtins[static_cast<size_t>(passState.channels[ch])].name.c_str();
                                }
                                
                                // 标签在前，下拉框在后
                                ImGui::Text("iChannel%d:", ch);
                                ImGui::SameLine();
                                ImGui::SetNextItemWidth(80);
                                
                                if (ImGui::BeginCombo("##combo", currentName, ImGuiComboFlags_NoArrowButton)) {
                                    // None 选项
                                    if (ImGui::Selectable("None", passState.channels[ch] == -1)) {
                                        passState.channels[ch] = -1;
                                    }
                                    
                                    // Buffer 选项（如果有其他 Buffer）
                                    ImGui::Separator();
                                    ImGui::TextDisabled("-- Buffers --");
                                    static const char* bufLabels[] = {"Buffer A", "Buffer B", "Buffer C", "Buffer D"};
                                    static const ShaderPassType bufTypes[] = {
                                        ShaderPassType::BufferA, ShaderPassType::BufferB,
                                        ShaderPassType::BufferC, ShaderPassType::BufferD
                                    };
                                    for (int b = 0; b < 4; b++) {
                                        // 显示已添加的 Buffer（包括自身，因为 ping-pong 双缓冲可读取上一帧）
                                        if (state.hasPass(bufTypes[b])) {
                                            int bindVal = ChannelBind::BufferA + b;
                                            bool isSelected = (passState.channels[ch] == bindVal);
                                            // 如果是自身，添加标记说明
                                            std::string label = bufLabels[b];
                                            if (bufTypes[b] == passState.type) {
                                                label += " (self)";
                                            }
                                            if (ImGui::Selectable(label.c_str(), isSelected)) {
                                                passState.channels[ch] = bindVal;
                                            }
                                        }
                                    }
                                    
                                    // 纹理选项
                                    ImGui::Separator();
                                    ImGui::TextDisabled("-- Textures --");
                                    for (size_t ti = 0; ti < builtins.size(); ti++) {
                                        bool isSelected = (passState.channels[ch] == static_cast<int>(ti));
                                        if (ImGui::Selectable(builtins[ti].name.c_str(), isSelected)) {
                                            passState.channels[ch] = static_cast<int>(ti);
                                        }
                                    }
                                    
                                    ImGui::EndCombo();
                                }
                                
                                if (ch < 3) ImGui::SameLine();
                                ImGui::PopID();
                            }
                            
                            ImGui::Separator();
                        }
                        
                        // 使用 Consolas 字体渲染代码编辑器
                        if (g_editorFont) {
                            ImGui::PushFont(g_editorFont);
                        }
                        
                        passState.editor.Render("##ShaderCode");
                        
                        if (g_editorFont) {
                            ImGui::PopFont();
                        }
                        
                        ImGui::PopID();
                        ImGui::EndTabItem();
                    }
                    
                    // 处理 Tab 关闭
                    if (!isOpen && passState.type != ShaderPassType::Image) {
                        state.removePass(passState.type);
                    }
                }
                
                // [+] 添加 Pass 按钮
                if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip)) {
                    ImGui::OpenPopup("AddPassPopup");
                }
                
                // 添加 Pass 弹出菜单
                if (ImGui::BeginPopup("AddPassPopup")) {
                    ImGui::TextDisabled("Add Pass:");
                    ImGui::Separator();
                    
                    if (!state.hasPass(ShaderPassType::Common)) {
                        if (ImGui::MenuItem("Common")) {
                            state.addPass(ShaderPassType::Common);
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Shared code prepended to all passes");
                        }
                    }
                    
                    if (!state.hasPass(ShaderPassType::BufferA)) {
                        if (ImGui::MenuItem("Buffer A")) {
                            state.addPass(ShaderPassType::BufferA);
                        }
                    }
                    if (!state.hasPass(ShaderPassType::BufferB)) {
                        if (ImGui::MenuItem("Buffer B")) {
                            state.addPass(ShaderPassType::BufferB);
                        }
                    }
                    if (!state.hasPass(ShaderPassType::BufferC)) {
                        if (ImGui::MenuItem("Buffer C")) {
                            state.addPass(ShaderPassType::BufferC);
                        }
                    }
                    if (!state.hasPass(ShaderPassType::BufferD)) {
                        if (ImGui::MenuItem("Buffer D")) {
                            state.addPass(ShaderPassType::BufferD);
                        }
                    }
                    
                    ImGui::EndPopup();
                }
                
                ImGui::EndTabBar();
            }
            
            // 同步 Image pass 的 channel 到全局 channelBindings（向后兼容）
            auto* imagePass = state.getPassEditor(ShaderPassType::Image);
            if (imagePass) {
                for (int i = 0; i < 4; i++) {
                    state.channelBindings[i] = imagePass->channels[i];
                }
            }
            
            // 检查是否需要重新编译 (F5)
            if (ImGui::IsKeyPressed(ImGuiKey_F5)) {
                state.needsRecompile = true;
            }
        }
        ImGui::End();
    }
    
    // 控制面板
    if (state.showControls) {
        ImGui::SetNextWindowSize(ImVec2(250, 200), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(app.getWidth() - 270, 40), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("Controls", &state.showControls)) {
            // 播放控制
            if (ImGui::Button(app.isPaused() ? "Play" : "Pause")) {
                app.togglePause();
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset")) {
                app.resetTime();
            }
            
            ImGui::Separator();
            
            // 时间显示
            ImGui::Text("Time: %.2f s", app.getTime());
            ImGui::Text("Frame: %d", app.getFrame());
            ImGui::Text("FPS: %.1f", state.fps);
            
            ImGui::Separator();
            
            // 分辨率
            ImGui::Text("Resolution: %dx%d", app.getWidth(), app.getHeight());
            
            // 鼠标状态
            const auto& mouse = app.getMouseState();
            ImGui::Text("Mouse: (%.0f, %.0f)", mouse.x, mouse.y);
        }
        ImGui::End();
    }
    
    // 错误显示
    if (!state.lastError.empty()) {
        ImGui::SetNextWindowSize(ImVec2(500, 150), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(20, app.getHeight() - 170), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("Errors", nullptr, ImGuiWindowFlags_NoCollapse)) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
            ImGui::TextWrapped("%s", state.lastError.c_str());
            ImGui::PopStyleColor();
        }
        ImGui::End();
    }
    
    
    
    // ========================================================================
    // 保存屏保配置对话框
    // ========================================================================
    if (state.showSaveProfileDialog) {
        ImGui::OpenPopup("Save Screensaver Profile");
        state.showSaveProfileDialog = false;  // 只触发一次
    }
    
    if (ImGui::BeginPopupModal("Save Screensaver Profile", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Save current shader as a screensaver profile:");
        ImGui::Separator();
        
        ImGui::Text("Profile Name:");
        ImGui::SetNextItemWidth(300);
        ImGui::InputText("##profileName", state.newProfileName, sizeof(state.newProfileName));
        
        ImGui::Separator();
        
        // 显示多 Pass 信息
        ImGui::Text("Passes to save:");
        for (const auto& passEditor : state.passEditors) {
            std::string passName;
            switch (passEditor.type) {
                case ShaderPassType::Image:   passName = "Image"; break;
                case ShaderPassType::Common:  passName = "Common"; break;
                case ShaderPassType::BufferA: passName = "Buffer A"; break;
                case ShaderPassType::BufferB: passName = "Buffer B"; break;
                case ShaderPassType::BufferC: passName = "Buffer C"; break;
                case ShaderPassType::BufferD: passName = "Buffer D"; break;
                default: passName = "Unknown"; break;
            }
            size_t codeLen = passEditor.editor.GetText().length();
            ImGui::BulletText("%s (%zu chars)", passName.c_str(), codeLen);
        }
        
        ImGui::Separator();
        
        if (ImGui::Button("Save", ImVec2(120, 0))) {
            if (strlen(state.newProfileName) > 0) {
                // 加载当前配置
                ScreensaverMode::loadConfig(g_scrConfig);
                
                // 创建新的 profile
                ScreensaverProfile newProfile;
                newProfile.name = state.newProfileName;
                newProfile.timeScale = 1.0f;
                
                // 使用新的多 Pass 同步函数
                state.syncToProfile(newProfile);
                
                // 检查是否有同名配置，如果有则覆盖
                bool found = false;
                for (size_t i = 0; i < g_scrConfig.profiles.size(); i++) {
                    if (g_scrConfig.profiles[i].name == newProfile.name) {
                        g_scrConfig.profiles[i] = newProfile;
                        g_scrConfig.activeProfileIndex = static_cast<int>(i);
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    g_scrConfig.profiles.push_back(newProfile);
                    g_scrConfig.activeProfileIndex = static_cast<int>(g_scrConfig.profiles.size()) - 1;
                }
                
                // 保存配置
                ScreensaverMode::saveConfig(g_scrConfig);
                
                std::cout << "Profile saved: " << newProfile.name 
                          << " with " << newProfile.passes.size() << " passes" << std::endl;
                
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
    
    // ========================================================================
    // 配置管理弹窗
    // ========================================================================
    if (state.showProfileManager) {
        ImGui::OpenPopup("Manage Screensaver Profiles");
        state.showProfileManager = false;  // 只触发一次
    }
    
    if (ImGui::BeginPopupModal("Manage Screensaver Profiles", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Saved screensaver profiles:");
        ImGui::Separator();
        
        static int selectedProfile = -1;
        static char renameBuffer[128] = "";
        
        // 配置列表（带随机复选框）
        ImGui::BeginChild("ProfileList", ImVec2(450, 200), true);
        for (int i = 0; i < static_cast<int>(g_scrConfig.profiles.size()); i++) {
            auto& profile = g_scrConfig.profiles[static_cast<size_t>(i)];
            bool isActive = (i == g_scrConfig.activeProfileIndex);
            
            ImGui::PushID(i);
            
            // 参与随机复选框
            bool includeRandom = profile.includeInRandom;
            if (ImGui::Checkbox("##random", &includeRandom)) {
                profile.includeInRandom = includeRandom;
                ScreensaverMode::saveConfig(g_scrConfig);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Include in random playback");
            }
            ImGui::SameLine();
            
            // Profile 名称（可选择）
            std::string label = profile.name;
            if (isActive) {
                label = "[ACTIVE] " + label;
            }
            
            if (ImGui::Selectable(label.c_str(), selectedProfile == i)) {
                selectedProfile = i;
                strncpy_s(renameBuffer, profile.name.c_str(), sizeof(renameBuffer) - 1);
            }
            
            ImGui::PopID();
        }
        ImGui::EndChild();
        
        // 操作按钮
        ImGui::Separator();
        
        bool hasSelection = (selectedProfile >= 0 && selectedProfile < static_cast<int>(g_scrConfig.profiles.size()));
        
        // 设为激活
        if (ImGui::Button("Set Active", ImVec2(100, 0)) && hasSelection) {
            g_scrConfig.activeProfileIndex = selectedProfile;
            ScreensaverMode::saveConfig(g_scrConfig);
        }
        ImGui::SameLine();
        
        // 加载到编辑器
        if (ImGui::Button("Load to Editor", ImVec2(120, 0)) && hasSelection) {
            const auto& profile = g_scrConfig.profiles[static_cast<size_t>(selectedProfile)];
            loadProfileToEditors(state, profile);
            // 加载 Profile 后重置时间和清除 Buffer
            app.resetTime();
            state.multiPassRenderer.getBufferManager().clearAll();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        
        // 删除
        if (ImGui::Button("Delete", ImVec2(80, 0)) && hasSelection) {
            g_scrConfig.profiles.erase(g_scrConfig.profiles.begin() + selectedProfile);
            if (g_scrConfig.activeProfileIndex >= static_cast<int>(g_scrConfig.profiles.size())) {
                g_scrConfig.activeProfileIndex = static_cast<int>(g_scrConfig.profiles.size()) - 1;
            }
            if (g_scrConfig.activeProfileIndex < 0) {
                g_scrConfig.activeProfileIndex = 0;
            }
            ScreensaverMode::saveConfig(g_scrConfig);
            selectedProfile = -1;
        }
        
        // 重命名
        ImGui::Separator();
        if (hasSelection) {
            ImGui::Text("Rename:");
            ImGui::SetNextItemWidth(200);
            ImGui::InputText("##rename", renameBuffer, sizeof(renameBuffer));
            ImGui::SameLine();
            if (ImGui::Button("Apply") && strlen(renameBuffer) > 0) {
                g_scrConfig.profiles[static_cast<size_t>(selectedProfile)].name = renameBuffer;
                ScreensaverMode::saveConfig(g_scrConfig);
            }
        }
        
        // 随机播放设置
        ImGui::Separator();
        ImGui::Text("Random Playback:");
        bool randomMode = g_scrConfig.randomMode;
        if (ImGui::Checkbox("Enable Random Mode", &randomMode)) {
            g_scrConfig.randomMode = randomMode;
            ScreensaverMode::saveConfig(g_scrConfig);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Randomly switch between profiles during screensaver");
        }
        
        float intervalSec = g_scrConfig.randomInterval;
        ImGui::SetNextItemWidth(150);
        if (ImGui::SliderFloat("Switch Interval (sec)", &intervalSec, 10.0f, 300.0f, "%.0f")) {
            g_scrConfig.randomInterval = intervalSec;
            ScreensaverMode::saveConfig(g_scrConfig);
        }
        
        ImGui::Separator();
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            selectedProfile = -1;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
    
    // ========================================================================
    // Debug Buffer 状态指示器（显示当前正在调试哪个 Buffer）
    // ========================================================================
    int debugBufferIdx = state.multiPassRenderer.getDebugBuffer();
    if (debugBufferIdx >= 0 && debugBufferIdx <= 3) {
        const char* bufferNames[] = {"Buffer A", "Buffer B", "Buffer C", "Buffer D"};
        
        // 在视口左上角显示调试状态
        ImVec2 displayPos(10, 30);
        ImDrawList* drawList = ImGui::GetForegroundDrawList();
        
        char debugText[64];
        snprintf(debugText, sizeof(debugText), "[DEBUG: %s]", bufferNames[debugBufferIdx]);
        
        // 背景框
        ImVec2 textSize = ImGui::CalcTextSize(debugText);
        drawList->AddRectFilled(
            ImVec2(displayPos.x - 4, displayPos.y - 2),
            ImVec2(displayPos.x + textSize.x + 4, displayPos.y + textSize.y + 2),
            IM_COL32(0, 0, 0, 180),
            4.0f
        );
        
        // 文本（黄色）
        drawList->AddText(displayPos, IM_COL32(255, 200, 0, 255), debugText);
    }
    
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// ============================================================================
// 屏保模式运行
// ============================================================================
int runScreensaverMode() {
    // 初始化内置 shaders (必须在加载配置前初始化，因为配置迁移可能需要)
    ScreensaverMode::initBuiltinShaders();
    
    // 加载配置
    bool configLoaded = ScreensaverMode::loadConfig(g_scrConfig);
    
    // 调试：输出配置状态
    #ifdef _DEBUG
    if (configLoaded) {
        OutputDebugStringA("Screensaver config loaded successfully\n");
        char buf[256];
        sprintf_s(buf, "  Profiles count: %zu\n", g_scrConfig.profiles.size());
        OutputDebugStringA(buf);
        sprintf_s(buf, "  Active index: %d\n", g_scrConfig.activeProfileIndex);
        OutputDebugStringA(buf);
    } else {
        OutputDebugStringA("No screensaver config found, using defaults\n");
    }
    #endif
    
    // 获取主显示器分辨率
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    
    // 创建独占全屏窗口配置（更好的 VSync 支持）
    AppConfig config;
    config.width = mode->width;
    config.height = mode->height;
    config.title = "Screensaver";
    config.vsync = true;
    config.fullscreen = true;
    config.decorated = true;  // 使用独占全屏模式以获得更好的 VSync
    
    Application app(config);
    if (!app.init()) {
        return -1;
    }
    
    // 隐藏鼠标
    glfwSetInputMode(app.getWindow(), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    
    // 初始化渲染
    AppState state;
    g_state = &state;
    state.renderer.init();
    TextureManager::instance().init();
    
    // 初始化多 Pass 渲染器
    state.multiPassRenderer.init(app.getWidth(), app.getHeight());
    
    // 获取内置 shaders（确保已初始化）
    const auto& builtins = ScreensaverMode::getBuiltinShaders();
    
    // 辅助函数：从 Profile 加载多 Pass 到 MultiPassRenderer
    auto loadProfileToMultiPass = [&state](const ScreensaverProfile& profile) -> bool {
        // 清理旧的 buffer 数据
        state.multiPassRenderer.getBufferManager().clearAll();
        
        // 清除 Common 代码（防止旧的 Common 污染新 Profile）
        state.multiPassRenderer.setCommonCode("");
        
        // 禁用所有 Buffer Pass（防止旧 Pass 残留）
        state.multiPassRenderer.disablePass(ShaderPassType::BufferA);
        state.multiPassRenderer.disablePass(ShaderPassType::BufferB);
        state.multiPassRenderer.disablePass(ShaderPassType::BufferC);
        state.multiPassRenderer.disablePass(ShaderPassType::BufferD);
        state.multiPassRenderer.disablePass(ShaderPassType::Common);
        
        // 检查是否有有效的多 Pass 配置
        // passes 可能包含空的默认 Pass，需要检查是否有实际代码
        bool hasMultiPassCode = false;
        for (const auto& pass : profile.passes) {
            if (!pass.code.empty()) {
                hasMultiPassCode = true;
                break;
            }
        }
        
        if (hasMultiPassCode) {
            // 使用多 Pass 配置
            std::string imageCode;
            std::map<int, std::string> bufferCodes;   // bufferIndex -> code
            std::map<int, std::array<int, 4>> bufferChannels;  // bufferIndex -> channels
            
            std::cout << "[Screensaver] Loading multi-pass profile: " << profile.name << std::endl;
            std::cout << "[Screensaver] Profile has " << profile.passes.size() << " passes" << std::endl;
            
            // 解析所有 Pass
            for (const auto& pass : profile.passes) {
                std::cout << "[Screensaver]   Pass type=" << static_cast<int>(pass.type) 
                          << " code_size=" << pass.code.size() << std::endl;
                if (pass.type == ShaderPassType::Common) {
                    // 设置 Common 代码到 MultiPassRenderer（它会在编译时自动合并）
                    state.multiPassRenderer.setCommonCode(pass.code);
                } else if (pass.type == ShaderPassType::Image) {
                    imageCode = pass.code;
                    // 设置 Image Pass 的 channel 绑定
                    for (int ch = 0; ch < 4; ch++) {
                        state.multiPassRenderer.setChannelBinding(ch, pass.channels[ch]);
                    }
                } else if (pass.type == ShaderPassType::BufferA) {
                    bufferCodes[0] = pass.code;
                    bufferChannels[0] = pass.channels;
                } else if (pass.type == ShaderPassType::BufferB) {
                    bufferCodes[1] = pass.code;
                    bufferChannels[1] = pass.channels;
                } else if (pass.type == ShaderPassType::BufferC) {
                    bufferCodes[2] = pass.code;
                    bufferChannels[2] = pass.channels;
                } else if (pass.type == ShaderPassType::BufferD) {
                    bufferCodes[3] = pass.code;
                    bufferChannels[3] = pass.channels;
                }
            }
            
            // 设置 Buffer 的 channel 绑定
            for (const auto& [bufferIndex, channels] : bufferChannels) {
                for (int ch = 0; ch < 4; ch++) {
                    state.multiPassRenderer.setBufferChannelBinding(bufferIndex, ch, channels[ch]);
                }
            }
            
            // 编译所有 Pass
            bool success = true;
            
            // 编译 Buffer Passes（不需要手动合并 common code，MultiPassRenderer 会自动处理）
            for (const auto& [bufferIndex, code] : bufferCodes) {
                if (!code.empty()) {
                    std::string error;
                    if (!state.multiPassRenderer.compileBufferPass(bufferIndex, code, error)) {
                        std::cerr << "[Screensaver] Failed to compile Buffer " << (char)('A' + bufferIndex) << ": " << error << std::endl;
                        success = false;
                    } else {
                        std::cout << "[Screensaver] Compiled Buffer " << (char)('A' + bufferIndex) << " OK" << std::endl;
                    }
                }
            }
            
            // 编译 Image Pass（不需要手动合并 common code，MultiPassRenderer 会自动处理）
            if (!imageCode.empty()) {
                std::string error;
                if (!state.multiPassRenderer.compileMainPass(imageCode, error)) {
                    std::cerr << "[Screensaver] Failed to compile Image pass: " << error << std::endl;
                    success = false;
                } else {
                    std::cout << "[Screensaver] Compiled Image pass OK" << std::endl;
                }
            } else {
                std::cerr << "[Screensaver] WARNING: Image pass code is empty!" << std::endl;
            }
            
            std::cout << "[Screensaver] hasValidMainPass = " << state.multiPassRenderer.hasValidMainPass() << std::endl;
            
            return success;
        } else {
            // 兼容旧的单 Pass 配置（shaderCode）
            if (!profile.shaderCode.empty()) {
                std::string error;
                if (!state.multiPassRenderer.compileMainPass(profile.shaderCode, error)) {
                    std::cerr << "[Screensaver] Failed to compile shader: " << error << std::endl;
                    return false;
                }
                // 设置 channel 绑定
                for (int ch = 0; ch < 4; ch++) {
                    state.multiPassRenderer.setChannelBinding(ch, profile.channelBindings[ch]);
                }
                return true;
            }
            return false;
        }
    };
    
    // 加载初始 Profile
    float timeScale = 1.0f;
    const ScreensaverProfile* activeProfile = g_scrConfig.getActiveProfile();
    
    if (activeProfile) {
        loadProfileToMultiPass(*activeProfile);
        timeScale = activeProfile->timeScale;
    } else if (!g_scrConfig.profiles.empty() && g_scrConfig.activeProfileIndex >= 0) {
        size_t idx = static_cast<size_t>(g_scrConfig.activeProfileIndex);
        if (idx < g_scrConfig.profiles.size()) {
            loadProfileToMultiPass(g_scrConfig.profiles[idx]);
            timeScale = g_scrConfig.profiles[idx].timeScale;
        }
    }
    
    // 如果没有加载成功，使用回退逻辑
    if (!state.multiPassRenderer.hasValidMainPass()) {
        std::string fallbackCode;
        if (g_scrConfig.useBuiltinShader && g_scrConfig.selectedBuiltinIndex >= 0 
            && g_scrConfig.selectedBuiltinIndex < static_cast<int>(builtins.size())) {
            fallbackCode = builtins[static_cast<size_t>(g_scrConfig.selectedBuiltinIndex)].code;
        } else if (!g_scrConfig.shaderCode.empty()) {
            fallbackCode = g_scrConfig.shaderCode;
        } else if (!builtins.empty()) {
            fallbackCode = builtins[0].code;
        }
        
        if (!fallbackCode.empty()) {
            std::string error;
            state.multiPassRenderer.compileMainPass(fallbackCode, error);
        }
        
        timeScale = g_scrConfig.timeScale;
        if (timeScale <= 0.0f) timeScale = 1.0f;
        
        for (int i = 0; i < 4; i++) {
            state.multiPassRenderer.setChannelBinding(i, g_scrConfig.channelBindings[i]);
        }
    }
    
    // 随机播放状态初始化
    g_randomTimer = 0.0f;
    g_currentRandomIndex = g_scrConfig.activeProfileIndex;
    
    // 构建参与随机的 Profile 索引列表
    std::vector<int> randomCandidates;
    for (int i = 0; i < static_cast<int>(g_scrConfig.profiles.size()); i++) {
        if (g_scrConfig.profiles[static_cast<size_t>(i)].includeInRandom) {
            randomCandidates.push_back(i);
        }
    }
    
    // 如果启用随机模式且有多个候选 profile，才启用随机模式
    bool effectiveRandomMode = g_scrConfig.randomMode && randomCandidates.size() > 1;
    float randomInterval = g_scrConfig.randomInterval;
    if (randomInterval <= 0.0f) randomInterval = 30.0f;
    
    // 初始化随机数种子
    srand(static_cast<unsigned int>(time(nullptr)));
    
    // 随机选择初始 profile（如果启用随机模式）
    if (effectiveRandomMode && !randomCandidates.empty()) {
        int candidateIdx = rand() % static_cast<int>(randomCandidates.size());
        g_currentRandomIndex = randomCandidates[static_cast<size_t>(candidateIdx)];
        const auto& profile = g_scrConfig.profiles[static_cast<size_t>(g_currentRandomIndex)];
        loadProfileToMultiPass(profile);
        timeScale = profile.timeScale;
    }
    
    // 可变的时间缩放（用于随机切换后更新）
    float currentTimeScale = timeScale;
    
    // 设置 update callback（处理随机切换）
    app.setUpdateCallback([&state, &app, &loadProfileToMultiPass, effectiveRandomMode, randomInterval, &currentTimeScale](float deltaTime) {
        // 随机切换逻辑
        if (effectiveRandomMode) {
            g_randomTimer += deltaTime;
            
            if (g_randomTimer >= randomInterval) {
                g_randomTimer = 0.0f;
                
                // 重新构建候选列表（支持运行时配置变化）
                std::vector<int> candidates;
                for (int i = 0; i < static_cast<int>(g_scrConfig.profiles.size()); i++) {
                    if (g_scrConfig.profiles[static_cast<size_t>(i)].includeInRandom) {
                        candidates.push_back(i);
                    }
                }
                
                // 需要至少 2 个候选才能切换
                if (candidates.size() > 1) {
                    // 选择下一个随机 profile（避免选择当前正在播放的）
                    int newIndex;
                    int candidateCount = static_cast<int>(candidates.size());
                    do {
                        int candidateIdx = rand() % candidateCount;
                        newIndex = candidates[static_cast<size_t>(candidateIdx)];
                    } while (newIndex == g_currentRandomIndex && candidateCount > 1);
                    
                    g_currentRandomIndex = newIndex;
                    
                    // 加载新的 profile（支持多 Pass）
                    const auto& profile = g_scrConfig.profiles[static_cast<size_t>(g_currentRandomIndex)];
                    bool loadSuccess = loadProfileToMultiPass(profile);
                    
                    std::cout << "[Screensaver] Random switch to profile [" << newIndex << "] '" 
                              << profile.name << "' - " << (loadSuccess ? "SUCCESS" : "FAILED") << std::endl;
                    
                    // ========== 重置所有系统变量 ==========
                    // 1. 重置时间（iTime 从 0 开始）
                    app.resetTime();
                    
                    // 2. 清除所有 Buffer 内容（防止历史数据影响新 shader）
                    state.multiPassRenderer.getBufferManager().clearAll();
                    
                    std::cout << "[Screensaver] Reset: time, frame, buffers cleared" << std::endl;
                    
                    // 更新时间缩放
                    currentTimeScale = profile.timeScale;
                    if (currentTimeScale <= 0.0f) currentTimeScale = 1.0f;
                }
            }
        }
    });
    
    // 设置退出检测回调
    glfwSetKeyCallback(app.getWindow(), [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        (void)key; (void)scancode; (void)action; (void)mods;
        // 任意按键退出
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    });
    
    glfwSetMouseButtonCallback(app.getWindow(), [](GLFWwindow* window, int button, int action, int mods) {
        (void)button; (void)action; (void)mods;
        // 任意鼠标点击退出
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    });
    
    // 渲染回调 - 使用多 Pass 渲染器
    app.setRenderCallback([&state, &app, &currentTimeScale]() {
        // 检测鼠标移动
        double mouseX, mouseY;
        glfwGetCursorPos(app.getWindow(), &mouseX, &mouseY);
        
        if (!g_mouseInitialized) {
            g_initialMouseX = mouseX;
            g_initialMouseY = mouseY;
            g_mouseInitialized = true;
        } else {
            double dx = mouseX - g_initialMouseX;
            double dy = mouseY - g_initialMouseY;
            if (std::sqrt(dx*dx + dy*dy) > MOUSE_MOVE_THRESHOLD) {
                glfwSetWindowShouldClose(app.getWindow(), GLFW_TRUE);
                return;
            }
        }
        
        // 清屏
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // 检查窗口尺寸变化并调整 buffer 大小
        int width = app.getWidth();
        int height = app.getHeight();
        state.multiPassRenderer.resize(width, height);
        
        // 渲染多 Pass shader
        if (state.multiPassRenderer.hasValidMainPass()) {
            // 设置 uniforms
            state.uniformManager.setTime(app.getTime() * currentTimeScale);
            state.uniformManager.setResolution(static_cast<float>(width), 
                                                static_cast<float>(height));
            state.uniformManager.setMouse(0, 0, 0, 0);
            state.uniformManager.setFrame(app.getFrame());
            state.uniformManager.setTimeDelta(app.getDeltaTime());
            state.uniformManager.updateDate();  // 更新日期时间
            
            // 执行多 Pass 渲染
            state.multiPassRenderer.render(state.uniformManager, state.renderer);
        }
    });
    
    app.run();
    return 0;
}

// ============================================================================
// 配置模式运行 - 直接打开编辑器模式（简化入口）
// ============================================================================
int runConfigureMode() {
    // 配置模式直接启动编辑器模式
    // 用户可以在编辑器中通过 File > Screensaver 菜单管理屏保配置
    return runEditorMode();
}

// ============================================================================
// 预览模式运行 (控制面板小窗口)
// ============================================================================
int runPreviewMode() {
    // 预览模式较复杂，需要在父窗口中渲染
    // 暂时简单返回，或显示静态图像
    return 0;
}

// ============================================================================
// 编辑器模式运行
// ============================================================================
int runEditorMode() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Local Shadertoy v1.0.0" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 创建应用配置
    AppConfig config;
    config.width = 1280;
    config.height = 720;
    config.title = "Local Shadertoy";
    config.vsync = true;
    
    // 创建并初始化应用
    Application app(config);
    
    if (!app.init()) {
        std::cerr << "Failed to initialize application!" << std::endl;
        return -1;
    }
    
    std::cout << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  Space      - Pause/Resume" << std::endl;
    std::cout << "  R          - Reset time" << std::endl;
    std::cout << "  F5         - Compile shader" << std::endl;
    std::cout << "  Escape     - Exit" << std::endl;
    std::cout << std::endl;
    
    // 初始化状态
    AppState state;
    g_state = &state;
    
    // 初始化渲染器
    state.renderer.init();
    
    // 初始化纹理管理器
    if (!TextureManager::instance().init()) {
        std::cerr << "Warning: Failed to initialize some builtin textures" << std::endl;
    } else {
        std::cout << "Texture Manager initialized with " 
                  << TextureManager::instance().getBuiltinTextures().size() 
                  << " builtin textures" << std::endl;
    }
    
    // 初始化ImGui
    initImGui(app.getWindow());
    
    // 加载屏保配置 (如果存在)
    ScreensaverMode::initBuiltinShaders();
    bool configLoaded = ScreensaverMode::loadConfig(g_scrConfig);
    
    // 初始化 Multi-pass 编辑器 - 确保 Image Tab 存在
    state.initDefaultPasses();
    
    // 初始化编辑器 - 优先使用已保存的 profile
    bool profileLoaded = false;
    if (configLoaded) {
        const ScreensaverProfile* activeProfile = g_scrConfig.getActiveProfile();
        // 使用 hasAnyCode() 检查，支持单 Pass 和多 Pass profile
        if (activeProfile && activeProfile->hasAnyCode()) {
            // 使用统一的加载函数，正确处理单/多 Pass
            loadProfileToEditors(state, *activeProfile);
            profileLoaded = true;
            std::cout << "Loaded screensaver profile: " << activeProfile->name << std::endl;
        }
    }
    
    // 如果没有加载 profile，使用默认代码
    if (!profileLoaded) {
        std::string initialCode = state.projectManager.getProject().getImageCode();
        auto* imagePass = state.getPassEditor(ShaderPassType::Image);
        if (imagePass) {
            imagePass->editor.SetText(initialCode);
        }
        // 同时设置旧字段（兼容）
        state.editor.SetText(initialCode);
        // 触发编译
        state.needsRecompile = true;
    }
    
    // 设置更新回调
    app.setUpdateCallback([&state, &app](float deltaTime) {
        // FPS计算
        state.fpsTimer += deltaTime;
        state.frameCount++;
        if (state.fpsTimer >= 1.0f) {
            state.fps = static_cast<float>(state.frameCount) / state.fpsTimer;
            state.frameCount = 0;
            state.fpsTimer = 0.0f;
        }
        
        // 检查是否需要重新编译
        if (state.needsRecompile) {
            // 使用多 Pass 编译
            compileAllPasses(state, app.getWidth(), app.getHeight());
            
            // 重置时间和帧计数器
            app.resetTime();
            
            // 清除 Buffer 内容（避免上一个 shader 的残留）
            state.multiPassRenderer.getBufferManager().clearAll();
            
            // 同步 Image pass 代码到项目
            auto* imagePass = state.getPassEditor(ShaderPassType::Image);
            if (imagePass) {
                state.projectManager.getProject().setImageCode(imagePass->editor.GetText());
            }
            
            state.needsRecompile = false;
        }
    });
    
    // 设置渲染回调 - 使用 Multi-pass 渲染管线
    app.setRenderCallback([&state, &app]() {
        // 清屏
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        auto& texMgr = TextureManager::instance();
        const auto& builtins = texMgr.getBuiltinTextures();
        
        // 检查是否有启用的 Multi-pass 渲染
        bool hasMultiPass = state.multiPassRenderer.isPassEnabled(ShaderPassType::Image) ||
                            state.multiPassRenderer.isPassEnabled(ShaderPassType::BufferA) ||
                            state.multiPassRenderer.isPassEnabled(ShaderPassType::BufferB) ||
                            state.multiPassRenderer.isPassEnabled(ShaderPassType::BufferC) ||
                            state.multiPassRenderer.isPassEnabled(ShaderPassType::BufferD);
        
        if (hasMultiPass) {
            // 使用 Multi-pass 渲染器
            state.multiPassRenderer.render(
                // Uniform 设置回调
                [&state, &app](GLuint program, ShaderPassType passType) {
                    (void)passType;  // 可用于 pass 特定的 uniform
                    
                    const auto& mouse = app.getMouseState();
                    state.uniformManager.setTime(app.getTime());
                    state.uniformManager.setResolution(
                        static_cast<float>(app.getWidth()), 
                        static_cast<float>(app.getHeight()));
                    state.uniformManager.setMouse(
                        mouse.x, mouse.y, 
                        mouse.leftPressed ? mouse.clickX : 0.0f,
                        mouse.leftPressed ? mouse.clickY : 0.0f);
                    state.uniformManager.setFrame(app.getFrame());
                    state.uniformManager.setTimeDelta(app.getDeltaTime());
                    state.uniformManager.updateDate();  // 更新日期时间
                    state.uniformManager.applyUniforms(program);
                },
                // 纹理绑定回调 (用于非 Buffer 类型的纹理)
                [&builtins](GLuint program, int channel, int binding) {
                    glActiveTexture(GL_TEXTURE0 + channel);
                    
                    if (binding >= 0 && binding < static_cast<int>(builtins.size())) {
                        size_t idx = static_cast<size_t>(binding);
                        glBindTexture(GL_TEXTURE_2D, builtins[idx].id);
                        
                        // 设置 iChannelResolution
                        std::string resName = "iChannelResolution[" + std::to_string(channel) + "]";
                        GLint loc = glGetUniformLocation(program, resName.c_str());
                        if (loc >= 0) {
                            glUniform3f(loc, 
                                static_cast<float>(builtins[idx].width),
                                static_cast<float>(builtins[idx].height),
                                1.0f);
                        }
                    } else {
                        glBindTexture(GL_TEXTURE_2D, 0);
                    }
                    
                    // 设置 iChannel uniform
                    std::string channelName = "iChannel" + std::to_string(channel);
                    GLint channelLoc = glGetUniformLocation(program, channelName.c_str());
                    if (channelLoc >= 0) {
                        glUniform1i(channelLoc, channel);
                    }
                },
                // 渲染四边形回调
                [&state]() {
                    state.renderer.renderFullscreenQuad();
                }
            );
        } else if (state.shaderEngine.isValid()) {
            // 回退到旧的单 Pass 渲染（兼容性）
            state.shaderEngine.use();
            GLuint program = state.shaderEngine.getProgram();
            
            for (int ch = 0; ch < 4; ch++) {
                glActiveTexture(GL_TEXTURE0 + ch);
                
                if (state.channelBindings[ch] >= 0 && 
                    state.channelBindings[ch] < static_cast<int>(builtins.size())) {
                    size_t idx = static_cast<size_t>(state.channelBindings[ch]);
                    glBindTexture(GL_TEXTURE_2D, builtins[idx].id);
                    
                    std::string resName = "iChannelResolution[" + std::to_string(ch) + "]";
                    GLint loc = glGetUniformLocation(program, resName.c_str());
                    if (loc >= 0) {
                        glUniform3f(loc, 
                            static_cast<float>(builtins[idx].width),
                            static_cast<float>(builtins[idx].height),
                            1.0f);
                    }
                } else {
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
                
                std::string channelName = "iChannel" + std::to_string(ch);
                GLint channelLoc = glGetUniformLocation(program, channelName.c_str());
                if (channelLoc >= 0) {
                    glUniform1i(channelLoc, ch);
                }
            }
            
            const auto& mouse = app.getMouseState();
            state.uniformManager.setTime(app.getTime());
            state.uniformManager.setResolution(
                static_cast<float>(app.getWidth()), 
                static_cast<float>(app.getHeight()));
            state.uniformManager.setMouse(
                mouse.x, mouse.y, 
                mouse.leftPressed ? mouse.clickX : 0.0f,
                mouse.leftPressed ? mouse.clickY : 0.0f);
            state.uniformManager.setFrame(app.getFrame());
            state.uniformManager.setTimeDelta(app.getDeltaTime());
            state.uniformManager.updateDate();  // 更新日期时间
            state.uniformManager.applyUniforms(program);
            
            state.renderer.renderFullscreenQuad();
        }
        
        // 渲染UI
        renderUI(state, app);
    });
    
    // 设置窗口大小改变回调
    app.setResizeCallback([&state, &app](int width, int height) {
        glViewport(0, 0, width, height);
        
        // 调整多 Pass 渲染器的 FBO 大小
        if (width > 0 && height > 0) {
            state.multiPassRenderer.resize(width, height);
            
            // 分辨率变化时重置时间和清除 Buffer
            app.resetTime();
            state.multiPassRenderer.getBufferManager().clearAll();
        }
    });
    
    // 运行主循环
    app.run();
    
    // 清理ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    std::cout << "Application closed." << std::endl;
    return 0;
}

// ============================================================================
// 主入口点
// ============================================================================
int main(int argc, char* argv[]) {
    // 初始化 GLFW（需要在解析参数前初始化，因为屏保模式需要查询显示器）
    if (!glfwInit()) {
        MessageBoxA(nullptr, "Failed to initialize GLFW!", "Error", MB_OK | MB_ICONERROR);
        return -1;
    }
    
    // 解析命令行参数
    g_runMode = ScreensaverMode::parseCommandLine(argc, argv, g_previewHwnd);
    
    int result = 0;
    
    switch (g_runMode) {
        case ScreensaverRunMode::Screensaver:
            result = runScreensaverMode();
            break;
            
        case ScreensaverRunMode::Configure:
            result = runConfigureMode();
            break;
            
        case ScreensaverRunMode::Preview:
            result = runPreviewMode();
            break;
            
        case ScreensaverRunMode::Editor:
        default:
            result = runEditorMode();
            break;
    }
    
    glfwTerminate();
    return result;
}
