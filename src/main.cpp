/**
 * Local Shadertoy - A local OpenGL implementation of Shadertoy
 * 
 * Main entry point
 */

#include "core/Application.h"
#include "core/ShaderEngine.h"
#include "core/UniformManager.h"
#include "core/ProjectManager.h"
#include "transpiler/GLSLTranspiler.h"
#include "renderer/Renderer.h"
#include "renderer/TextureManager.h"
#include "ui/UIManager.h"
#include "ui/ShaderEditor.h"
#include "utils/FileDialog.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <TextEditor.h>

#include <iostream>
#include <string>

using namespace shadertoy;

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
};

static AppState* g_state = nullptr;

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
    
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

int main(int argc, char* argv[]) {
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
    
    // 初始化编辑器
    state.editor.SetText(state.projectManager.getProject().getImageCode());
    
    // 编译初始shader
    compileCurrentShader(state, state.projectManager.getProject().getImageCode());
    
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