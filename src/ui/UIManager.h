#pragma once

struct GLFWwindow;

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

private:
    bool m_initialized = false;
};

} // namespace shadertoy
