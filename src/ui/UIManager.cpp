#include "UIManager.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>

namespace shadertoy {

UIManager::~UIManager() {
    shutdown();
}

bool UIManager::init(GLFWwindow* window) {
    if (m_initialized) return true;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    
    // 加载 Consolas 字体用于代码编辑器
    // Windows 系统字体路径：C:\Windows\Fonts\consola.ttf
    const char* fontPath = "C:\\Windows\\Fonts\\consola.ttf";
    m_editorFont = io.Fonts->AddFontFromFileTTF(fontPath, 16.0f);
    
    if (!m_editorFont) {
        // 如果加载失败，回退到默认字体
        m_editorFont = io.Fonts->AddFontDefault();
    }
    
    // 添加默认字体作为 UI 字体
    io.Fonts->AddFontDefault();
    
    setDarkTheme();
    
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 430");
    
    m_initialized = true;
    return true;
}

void UIManager::beginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UIManager::endFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UIManager::shutdown() {
    if (!m_initialized) return;
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    m_initialized = false;
}

void UIManager::setDarkTheme() {
    ImGui::StyleColorsDark();
    
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 2.0f;
    style.ScrollbarRounding = 2.0f;
    
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.12f, 1.0f);
    colors[ImGuiCol_Header] = ImVec4(0.2f, 0.2f, 0.25f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.3f, 0.3f, 0.35f, 1.0f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 0.25f, 0.3f, 1.0f);
}

} // namespace shadertoy