#pragma once

namespace shadertoy {

class Timer {
public:
    Timer();
    
    void start();
    void stop();
    void reset();
    void pause();
    void resume();
    
    float getElapsedSeconds() const;
    float getDeltaTime() const;
    
    bool isPaused() const { return m_paused; }

private:
    double m_startTime = 0.0;
    double m_pausedTime = 0.0;
    double m_lastFrameTime = 0.0;
    double m_deltaTime = 0.0;
    bool m_paused = false;
    bool m_running = false;
};

} // namespace shadertoy
