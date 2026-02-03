#include "Timer.h"

#include <GLFW/glfw3.h>

namespace shadertoy {

Timer::Timer() {
    reset();
}

void Timer::start() {
    if (!m_running) {
        m_startTime = glfwGetTime();
        m_lastFrameTime = m_startTime;
        m_running = true;
        m_paused = false;
    }
}

void Timer::stop() {
    m_running = false;
    m_paused = false;
}

void Timer::reset() {
    m_startTime = glfwGetTime();
    m_lastFrameTime = m_startTime;
    m_pausedTime = 0.0;
    m_deltaTime = 0.0;
}

void Timer::pause() {
    if (m_running && !m_paused) {
        m_pausedTime = glfwGetTime();
        m_paused = true;
    }
}

void Timer::resume() {
    if (m_running && m_paused) {
        double pauseDuration = glfwGetTime() - m_pausedTime;
        m_startTime += pauseDuration;
        m_lastFrameTime += pauseDuration;
        m_paused = false;
    }
}

float Timer::getElapsedSeconds() const {
    if (!m_running) return 0.0f;
    
    double currentTime = m_paused ? m_pausedTime : glfwGetTime();
    return static_cast<float>(currentTime - m_startTime);
}

float Timer::getDeltaTime() const {
    return static_cast<float>(m_deltaTime);
}

} // namespace shadertoy
