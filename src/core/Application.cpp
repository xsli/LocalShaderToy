#include "Application.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>

namespace shadertoy {

Application::Application(const AppConfig& config)
    : m_config(config)
    , m_width(config.width)
    , m_height(config.height)
{
}

Application::~Application() {
    shutdown();
}

bool Application::init() {
    // 初始化 GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // 配置 OpenGL 版本
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, m_config.glMajorVersion);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, m_config.glMinorVersion);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // 创建窗口
    GLFWmonitor* monitor = m_config.fullscreen ? glfwGetPrimaryMonitor() : nullptr;
    m_window = glfwCreateWindow(m_width, m_height, m_config.title.c_str(), monitor, nullptr);
    
    if (!m_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);

    // 设置垂直同步
    glfwSwapInterval(m_config.vsync ? 1 : 0);

    // 加载 OpenGL 函数
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    // 输出 OpenGL 信息
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;

    // 设置回调
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
    glfwSetKeyCallback(m_window, keyCallback);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
    glfwSetCursorPosCallback(m_window, cursorPosCallback);

    // 获取实际 framebuffer 大小
    glfwGetFramebufferSize(m_window, &m_width, &m_height);
    glViewport(0, 0, m_width, m_height);

    m_running = true;
    m_lastFrameTime = static_cast<float>(glfwGetTime());
    
    return true;
}

void Application::run() {
    while (m_running && !glfwWindowShouldClose(m_window)) {
        // 计算 delta time
        float currentTime = static_cast<float>(glfwGetTime());
        m_deltaTime = currentTime - m_lastFrameTime;
        m_lastFrameTime = currentTime;

        // 处理事件
        glfwPollEvents();
        handleInput();
        updateMouseState();

        // 更新逻辑
        if (!m_paused) {
            if (m_updateCallback) {
                m_updateCallback(m_deltaTime);
            }
            m_frame++;
        }

        // 渲染
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (m_renderCallback) {
            m_renderCallback();
        }

        // 交换缓冲区
        glfwSwapBuffers(m_window);
    }
}

void Application::shutdown() {
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
}

float Application::getTime() const {
    if (m_paused) {
        return m_pauseStartTime - m_pausedTime;
    }
    return static_cast<float>(glfwGetTime()) - m_pausedTime;
}

void Application::resetTime() {
    m_pausedTime = static_cast<float>(glfwGetTime());
    m_frame = 0;
}

void Application::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->m_width = width;
        app->m_height = height;
        glViewport(0, 0, width, height);
        
        if (app->m_resizeCallback) {
            app->m_resizeCallback(width, height);
        }
    }
}

void Application::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (!app) return;

    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                app->requestClose();
                break;
            case GLFW_KEY_SPACE:
                if (app->m_paused) {
                    // 恢复
                    app->m_pausedTime += static_cast<float>(glfwGetTime()) - app->m_pauseStartTime;
                } else {
                    // 暂停
                    app->m_pauseStartTime = static_cast<float>(glfwGetTime());
                }
                app->togglePause();
                break;
            case GLFW_KEY_R:
                if (mods & GLFW_MOD_CONTROL) {
                    app->resetTime();
                }
                break;
            case GLFW_KEY_F11:
                // TODO: Toggle fullscreen
                break;
        }
    }
}

void Application::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (!app) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            app->m_mouseState.leftPressed = true;
            double x, y;
            glfwGetCursorPos(window, &x, &y);
            // 转换为 Shadertoy 坐标系 (左下角为原点)
            app->m_mouseState.clickX = static_cast<float>(x);
            app->m_mouseState.clickY = static_cast<float>(app->m_height - y);
        } else if (action == GLFW_RELEASE) {
            app->m_mouseState.leftPressed = false;
            // Shadertoy 约定：释放时 clickX/clickY 变为负值
            app->m_mouseState.clickX = -app->m_mouseState.clickX;
            app->m_mouseState.clickY = -app->m_mouseState.clickY;
        }
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        app->m_mouseState.rightPressed = (action == GLFW_PRESS);
    }
}

void Application::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (!app) return;

    // 转换为 Shadertoy 坐标系 (左下角为原点)
    app->m_mouseState.x = static_cast<float>(xpos);
    app->m_mouseState.y = static_cast<float>(app->m_height - ypos);
}

void Application::handleInput() {
    // 可以添加更多输入处理
}

void Application::updateMouseState() {
    // 鼠标状态已在回调中更新
}

} // namespace shadertoy
