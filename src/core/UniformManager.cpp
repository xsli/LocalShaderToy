#include "UniformManager.h"
#include "Application.h"

#include <ctime>

namespace shadertoy {

void UniformManager::updateFromApp(const Application& app) {
    m_uniforms.iResolution = glm::vec3(
        static_cast<float>(app.getWidth()),
        static_cast<float>(app.getHeight()),
        1.0f
    );
    
    m_uniforms.iTime = app.getTime();
    m_uniforms.iTimeDelta = app.getDeltaTime();
    m_uniforms.iFrame = app.getFrame();
    
    const auto& mouse = app.getMouseState();
    m_uniforms.iMouse = glm::vec4(
        mouse.leftPressed ? mouse.x : 0.0f,
        mouse.leftPressed ? mouse.y : 0.0f,
        mouse.clickX,
        mouse.clickY
    );
    
    // 获取当前日期时间
    std::time_t now = std::time(nullptr);
    std::tm* tm = std::localtime(&now);
    m_uniforms.iDate = glm::vec4(
        static_cast<float>(tm->tm_year + 1900),
        static_cast<float>(tm->tm_mon),
        static_cast<float>(tm->tm_mday),
        static_cast<float>(tm->tm_hour * 3600 + tm->tm_min * 60 + tm->tm_sec)
    );
    
    m_uniforms.iSampleRate = 44100.0f;
}

void UniformManager::applyToProgram(GLuint program) const {
    glUseProgram(program);
    
    GLint loc;
    
    loc = glGetUniformLocation(program, "iResolution");
    if (loc >= 0) glUniform3fv(loc, 1, &m_uniforms.iResolution[0]);
    
    loc = glGetUniformLocation(program, "iTime");
    if (loc >= 0) glUniform1f(loc, m_uniforms.iTime);
    
    loc = glGetUniformLocation(program, "iTimeDelta");
    if (loc >= 0) glUniform1f(loc, m_uniforms.iTimeDelta);
    
    loc = glGetUniformLocation(program, "iFrame");
    if (loc >= 0) glUniform1i(loc, m_uniforms.iFrame);
    
    loc = glGetUniformLocation(program, "iMouse");
    if (loc >= 0) glUniform4fv(loc, 1, &m_uniforms.iMouse[0]);
    
    loc = glGetUniformLocation(program, "iDate");
    if (loc >= 0) glUniform4fv(loc, 1, &m_uniforms.iDate[0]);
    
    loc = glGetUniformLocation(program, "iSampleRate");
    if (loc >= 0) glUniform1f(loc, m_uniforms.iSampleRate);
    
    loc = glGetUniformLocation(program, "iChannelResolution");
    if (loc >= 0) glUniform3fv(loc, 4, &m_uniforms.iChannelResolution[0][0]);
    
    loc = glGetUniformLocation(program, "iChannelTime");
    if (loc >= 0) glUniform1fv(loc, 4, m_uniforms.iChannelTime);
    
    // 纹理采样器
    for (int i = 0; i < 4; i++) {
        char name[16];
        snprintf(name, sizeof(name), "iChannel%d", i);
        loc = glGetUniformLocation(program, name);
        if (loc >= 0) glUniform1i(loc, i);
    }
}

void UniformManager::setChannelResolution(int channel, const glm::vec3& resolution) {
    if (channel >= 0 && channel < 4) {
        m_uniforms.iChannelResolution[channel] = resolution;
    }
}

void UniformManager::setChannelTime(int channel, float time) {
    if (channel >= 0 && channel < 4) {
        m_uniforms.iChannelTime[channel] = time;
    }
}

} // namespace shadertoy
