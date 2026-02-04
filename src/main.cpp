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

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <TextEditor.h>

#include <iostream>
#include <string>
#include <cmath>
#include <fstream>
#include <sstream>

using namespace shadertoy;

// 运行模式
static ScreensaverRunMode g_runMode = ScreensaverRunMode::Editor;
static HWND g_previewHwnd = nullptr;
static ScreensaverConfig g_scrConfig;

// 屏保退出检测
static double g_initialMouseX = 0.0;
static double g_initialMouseY = 0.0;
static bool g_mouseInitialized = false;
static const double MOUSE_MOVE_THRESHOLD = 10.0;

// 全局状态
struct AppState {
    ProjectManager projectManager;
    ShaderEngine shaderEngine;
    UniformManager uniformManager;
    Renderer renderer;
    GLSLTranspiler transpiler;
    TextEditor editor;
    
    bool showEditor = true;
    bool showControls = true;
    bool showInfo = true;
    bool showTextures = true;  // 纹理面板
    bool needsRecompile = false;
    std::string lastError;
    float fps = 0.0f;
    int frameCount = 0;
    float fpsTimer = 0.0f;
    
    // iChannel 绑定 (-1 = None, 0+ = 内置纹理索引)
    int channelBindings[4] = {-1, -1, -1, -1};
    
    // 屏保配置管理
    bool showProfileManager = false;     // 显示配置管理弹窗
    bool showSaveProfileDialog = false;  // 显示保存配置对话框
    char newProfileName[128] = "";       // 新配置名称输入
};

static AppState* g_state = nullptr;

// Forward declarations
int runEditorMode();
int runScreensaverMode();
int runConfigureMode();
int runPreviewMode();

// 编译shader
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

// 初始化ImGui
void initImGui(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    
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
            ImGui::MenuItem("Textures", nullptr, &state.showTextures);
            ImGui::MenuItem("Info", nullptr, &state.showInfo);
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
    
    // Shader编辑器窗口
    if (state.showEditor) {
        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(20, 40), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("Shader Editor", &state.showEditor, ImGuiWindowFlags_MenuBar)) {
            if (ImGui::BeginMenuBar()) {
                if (ImGui::Button("Compile (F5)")) {
                    state.needsRecompile = true;
                }
                ImGui::EndMenuBar();
            }
            
            // 设置GLSL语法高亮
            auto lang = TextEditor::LanguageDefinition::GLSL();
            state.editor.SetLanguageDefinition(lang);
            
            // 渲染编辑器
            state.editor.Render("ShaderCode");
            
            // 检查是否需要重新编译 (Ctrl+Enter)
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
    
    // 项目信息
    if (state.showInfo) {
        ImGui::SetNextWindowSize(ImVec2(250, 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(app.getWidth() - 270, 260), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("Project", &state.showInfo)) {
            ImGui::Text("Name: %s", state.projectManager.getProjectName().c_str());
            if (state.projectManager.isModified()) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "(modified)");
            }
        }
        ImGui::End();
    }
    
    // 纹理面板
    if (state.showTextures) {
        ImGui::SetNextWindowSize(ImVec2(300, 350), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(app.getWidth() - 320, 380), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("Textures (iChannel)", &state.showTextures)) {
            auto& texMgr = TextureManager::instance();
            const auto& builtins = texMgr.getBuiltinTextures();
            
            // 为每个 iChannel 创建选择器
            for (int ch = 0; ch < 4; ch++) {
                ImGui::PushID(ch);
                
                ImGui::Text("iChannel%d:", ch);
                ImGui::SameLine();
                
                // 创建下拉选项
                const char* currentName = "None";
                if (state.channelBindings[ch] >= 0 && 
                    state.channelBindings[ch] < static_cast<int>(builtins.size())) {
                    currentName = builtins[static_cast<size_t>(state.channelBindings[ch])].name.c_str();
                }
                
                if (ImGui::BeginCombo("##combo", currentName)) {
                    // None 选项
                    if (ImGui::Selectable("None", state.channelBindings[ch] == -1)) {
                        state.channelBindings[ch] = -1;
                    }
                    
                    // 内置纹理选项
                    for (size_t i = 0; i < builtins.size(); i++) {
                        bool isSelected = (state.channelBindings[ch] == static_cast<int>(i));
                        if (ImGui::Selectable(builtins[i].name.c_str(), isSelected)) {
                            state.channelBindings[ch] = static_cast<int>(i);
                        }
                        
                        // 悬停提示
                        if (ImGui::IsItemHovered()) {
                            ImGui::BeginTooltip();
                            ImGui::Text("%dx%d", builtins[i].width, builtins[i].height);
                            ImGui::Text("Channels: %d", builtins[i].channels);
                            ImGui::EndTooltip();
                        }
                    }
                    ImGui::EndCombo();
                }
                
                // 显示纹理预览
                if (state.channelBindings[ch] >= 0 && 
                    state.channelBindings[ch] < static_cast<int>(builtins.size())) {
                    GLuint texId = builtins[static_cast<size_t>(state.channelBindings[ch])].id;
                    ImGui::Image((ImTextureID)(intptr_t)texId, ImVec2(64, 64));
                }
                
                ImGui::PopID();
                ImGui::Separator();
            }
            
            // 显示已加载纹理数量
            ImGui::Text("Builtin textures: %zu", builtins.size());
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
        
        // 显示当前通道绑定
        ImGui::Text("Texture Bindings:");
        auto& texMgr = TextureManager::instance();
        const auto& builtinTex = texMgr.getBuiltinTextures();
        for (int ch = 0; ch < 4; ch++) {
            const char* texName = "None";
            if (state.channelBindings[ch] >= 0 && 
                state.channelBindings[ch] < static_cast<int>(builtinTex.size())) {
                texName = builtinTex[static_cast<size_t>(state.channelBindings[ch])].name.c_str();
            }
            ImGui::BulletText("iChannel%d: %s", ch, texName);
        }
        
        ImGui::Separator();
        
        if (ImGui::Button("Save", ImVec2(120, 0))) {
            if (strlen(state.newProfileName) > 0) {
                // 加载当前配置
                ScreensaverMode::loadConfig(g_scrConfig);
                
                // 创建新的 profile
                ScreensaverProfile newProfile;
                newProfile.name = state.newProfileName;
                newProfile.shaderCode = state.editor.GetText();
                newProfile.timeScale = 1.0f;
                for (int i = 0; i < 4; i++) {
                    newProfile.channelBindings[i] = state.channelBindings[i];
                }
                
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
        
        // 配置列表
        ImGui::BeginChild("ProfileList", ImVec2(400, 200), true);
        for (int i = 0; i < static_cast<int>(g_scrConfig.profiles.size()); i++) {
            bool isActive = (i == g_scrConfig.activeProfileIndex);
            std::string label = g_scrConfig.profiles[static_cast<size_t>(i)].name;
            if (isActive) {
                label = "[ACTIVE] " + label;
            }
            
            if (ImGui::Selectable(label.c_str(), selectedProfile == i)) {
                selectedProfile = i;
                strncpy_s(renameBuffer, g_scrConfig.profiles[static_cast<size_t>(i)].name.c_str(), sizeof(renameBuffer) - 1);
            }
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
            state.editor.SetText(profile.shaderCode);
            for (int i = 0; i < 4; i++) {
                state.channelBindings[i] = profile.channelBindings[i];
            }
            state.needsRecompile = true;
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
        
        ImGui::Separator();
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            selectedProfile = -1;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
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
    
    // 加载 shader - 优先使用 profiles 系统
    std::string shaderCode;
    float timeScale = 1.0f;
    
    // 获取内置 shaders（确保已初始化）
    const auto& builtins = ScreensaverMode::getBuiltinShaders();
    
    const ScreensaverProfile* activeProfile = g_scrConfig.getActiveProfile();
    if (activeProfile && !activeProfile->shaderCode.empty()) {
        // 使用激活的 profile
        shaderCode = activeProfile->shaderCode;
        timeScale = activeProfile->timeScale;
        for (int i = 0; i < 4; i++) {
            state.channelBindings[i] = activeProfile->channelBindings[i];
        }
    } else if (!g_scrConfig.profiles.empty() && g_scrConfig.activeProfileIndex >= 0) {
        // profiles 存在但可能 shaderCode 为空
        size_t idx = static_cast<size_t>(g_scrConfig.activeProfileIndex);
        if (idx < g_scrConfig.profiles.size()) {
            shaderCode = g_scrConfig.profiles[idx].shaderCode;
            timeScale = g_scrConfig.profiles[idx].timeScale;
            for (int i = 0; i < 4; i++) {
                state.channelBindings[i] = g_scrConfig.profiles[idx].channelBindings[i];
            }
        }
    }
    
    // 如果 shaderCode 仍为空，使用回退逻辑
    if (shaderCode.empty()) {
        if (g_scrConfig.useBuiltinShader && g_scrConfig.selectedBuiltinIndex >= 0 
            && g_scrConfig.selectedBuiltinIndex < static_cast<int>(builtins.size())) {
            shaderCode = builtins[static_cast<size_t>(g_scrConfig.selectedBuiltinIndex)].code;
        } else if (!g_scrConfig.shaderCode.empty()) {
            shaderCode = g_scrConfig.shaderCode;
        } else {
            // 默认使用第一个内置 shader (Plasma)
            if (!builtins.empty()) {
                shaderCode = builtins[0].code;
            }
        }
        
        timeScale = g_scrConfig.timeScale;
        if (timeScale <= 0.0f) timeScale = 1.0f;
        
        for (int i = 0; i < 4; i++) {
            state.channelBindings[i] = g_scrConfig.channelBindings[i];
        }
    }
    
    // 确保有 shader 代码
    if (shaderCode.empty() && !builtins.empty()) {
        shaderCode = builtins[0].code;
    }
    
    // 编译 shader
    compileCurrentShader(state, shaderCode);
    
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
    
    // 渲染回调
    app.setRenderCallback([&state, &app, timeScale]() {
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
        
        // 渲染 shader
        if (state.shaderEngine.isValid()) {
            state.shaderEngine.use();
            
            // 绑定纹理
            auto& texMgr = TextureManager::instance();
            const auto& textures = texMgr.getBuiltinTextures();
            GLuint program = state.shaderEngine.getProgram();
            
            for (int ch = 0; ch < 4; ch++) {
                glActiveTexture(GL_TEXTURE0 + ch);
                if (state.channelBindings[ch] >= 0 && 
                    state.channelBindings[ch] < static_cast<int>(textures.size())) {
                    size_t idx = static_cast<size_t>(state.channelBindings[ch]);
                    glBindTexture(GL_TEXTURE_2D, textures[idx].id);
                } else {
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
                
                std::string channelName = "iChannel" + std::to_string(ch);
                GLint channelLoc = glGetUniformLocation(program, channelName.c_str());
                if (channelLoc >= 0) {
                    glUniform1i(channelLoc, ch);
                }
            }
            
            // 设置 uniforms
            state.uniformManager.setTime(app.getTime() * timeScale);
            state.uniformManager.setResolution(static_cast<float>(app.getWidth()), 
                                                static_cast<float>(app.getHeight()));
            state.uniformManager.setMouse(0, 0, 0, 0);
            state.uniformManager.setFrame(app.getFrame());
            state.uniformManager.setTimeDelta(app.getDeltaTime());
            state.uniformManager.applyUniforms(program);
            
            state.renderer.renderFullscreenQuad();
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
    
    // 初始化编辑器 - 优先使用已保存的 profile
    std::string initialCode;
    if (configLoaded) {
        const ScreensaverProfile* activeProfile = g_scrConfig.getActiveProfile();
        if (activeProfile && !activeProfile->shaderCode.empty()) {
            initialCode = activeProfile->shaderCode;
            // 恢复 channel bindings
            for (int i = 0; i < 4; i++) {
                state.channelBindings[i] = activeProfile->channelBindings[i];
            }
            std::cout << "Loaded screensaver profile: " << activeProfile->name << std::endl;
        }
    }
    
    // 如果没有配置，使用默认代码
    if (initialCode.empty()) {
        initialCode = state.projectManager.getProject().getImageCode();
    }
    
    state.editor.SetText(initialCode);
    
    // 编译初始shader
    compileCurrentShader(state, initialCode);
    
    // 设置更新回调
    app.setUpdateCallback([&state](float deltaTime) {
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
            std::string code = state.editor.GetText();
            state.projectManager.getProject().setImageCode(code);
            compileCurrentShader(state, code);
            state.needsRecompile = false;
        }
    });
    
    // 设置渲染回调
    app.setRenderCallback([&state, &app]() {
        // 清屏
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // 使用shader渲染
        if (state.shaderEngine.isValid()) {
            state.shaderEngine.use();
            
            // 绑定 iChannel 纹理
            auto& texMgr = TextureManager::instance();
            const auto& builtins = texMgr.getBuiltinTextures();
            GLuint program = state.shaderEngine.getProgram();
            
            for (int ch = 0; ch < 4; ch++) {
                glActiveTexture(GL_TEXTURE0 + ch);
                
                if (state.channelBindings[ch] >= 0 && 
                    state.channelBindings[ch] < static_cast<int>(builtins.size())) {
                    size_t idx = static_cast<size_t>(state.channelBindings[ch]);
                    // 绑定选中的纹理
                    glBindTexture(GL_TEXTURE_2D, builtins[idx].id);
                    
                    // 设置 iChannelResolution
                    std::string resName = "iChannelResolution[" + std::to_string(ch) + "]";
                    GLint loc = glGetUniformLocation(program, resName.c_str());
                    if (loc >= 0) {
                        glUniform3f(loc, 
                            static_cast<float>(builtins[idx].width),
                            static_cast<float>(builtins[idx].height),
                            1.0f);
                    }
                } else {
                    // 绑定空纹理
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
                
                // 设置 iChannel uniform
                std::string channelName = "iChannel" + std::to_string(ch);
                GLint channelLoc = glGetUniformLocation(program, channelName.c_str());
                if (channelLoc >= 0) {
                    glUniform1i(channelLoc, ch);
                }
            }
            
            // 设置uniforms
            const auto& mouse = app.getMouseState();
            state.uniformManager.setTime(app.getTime());
            state.uniformManager.setResolution(static_cast<float>(app.getWidth()), 
                                                static_cast<float>(app.getHeight()));
            state.uniformManager.setMouse(mouse.x, mouse.y, 
                                          mouse.leftPressed ? mouse.clickX : 0.0f,
                                          mouse.leftPressed ? mouse.clickY : 0.0f);
            state.uniformManager.setFrame(app.getFrame());
            state.uniformManager.setTimeDelta(app.getDeltaTime());
            
            state.uniformManager.applyUniforms(program);
            
            // 渲染全屏四边形
            state.renderer.renderFullscreenQuad();
        }
        
        // 渲染UI
        renderUI(state, app);
    });
    
    // 设置窗口大小改变回调
    app.setResizeCallback([](int width, int height) {
        glViewport(0, 0, width, height);
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
