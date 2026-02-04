#pragma once

#include <string>
#include <functional>

struct GLFWwindow;

namespace shadertoy {

struct AppConfig {
    int width = 1280;
    int height = 720;
    std::string title = "Local Shadertoy";
    bool vsync = true;
    bool fullscreen = false;
    bool decorated = true;  // 是否显示窗口边框
    int glMajorVersion = 4;
    int glMinorVersion = 3;
};

class Application {
public:
    Application(const AppConfig& config = AppConfig{});
    ~Application();

    // 禁止拷贝
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    // 初始化
    bool init();
    
    // 主循环
    void run();
    
    // 关闭
    void shutdown();

    // 回调设置
    using UpdateCallback = std::function<void(float deltaTime)>;
    using RenderCallback = std::function<void()>;
    using ResizeCallback = std::function<void(int width, int height)>;
    
    void setUpdateCallback(UpdateCallback callback) { m_updateCallback = callback; }
    void setRenderCallback(RenderCallback callback) { m_renderCallback = callback; }
    void setResizeCallback(ResizeCallback callback) { m_resizeCallback = callback; }

    // 获取器
    GLFWwindow* getWindow() const { return m_window; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    float getTime() const;
    float getDeltaTime() const { return m_deltaTime; }
    int getFrame() const { return m_frame; }
    
    // 鼠标状态
    struct MouseState {
        float x = 0.0f;
        float y = 0.0f;
        float clickX = 0.0f;
        float clickY = 0.0f;
        bool leftPressed = false;
        bool rightPressed = false;
    };
    const MouseState& getMouseState() const { return m_mouseState; }

    // 状态控制
    bool isRunning() const { return m_running; }
    void requestClose() { m_running = false; }
    
    // 暂停控制
    bool isPaused() const { return m_paused; }
    void setPaused(bool paused) { m_paused = paused; }
    void togglePause() { m_paused = !m_paused; }
    
    // 重置时间
    void resetTime();

private:
    // GLFW 回调
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    
    void handleInput();
    void updateMouseState();

private:
    AppConfig m_config;
    GLFWwindow* m_window = nullptr;
    
    int m_width = 0;
    int m_height = 0;
    
    bool m_running = false;
    bool m_paused = false;
    
    float m_deltaTime = 0.0f;
    float m_lastFrameTime = 0.0f;
    float m_pausedTime = 0.0f;
    float m_pauseStartTime = 0.0f;
    int m_frame = 0;
    
    MouseState m_mouseState;
    
    UpdateCallback m_updateCallback;
    RenderCallback m_renderCallback;
    ResizeCallback m_resizeCallback;
};

} // namespace shadertoy
