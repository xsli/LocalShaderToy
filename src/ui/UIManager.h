#pragma once

struct GLFWwindow;
struct ImFont;

namespace shadertoy {

class UIManager {
public:
    UIManager() = default;
    ~UIManager();

    bool init(GLFWwindow* window);
    void beginFrame();
    void endFrame();
    void shutdown();

    void setDarkTheme();
    
    // 获取编辑器专用字体（Consolas）
    ImFont* getEditorFont() const { return m_editorFont; }

private:
    bool m_initialized = false;
    ImFont* m_editorFont = nullptr;  // 编辑器使用的等宽字体
};

} // namespace shadertoy