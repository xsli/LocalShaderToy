#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

namespace shadertoy {

class Application;

// Shadertoy 标准 uniform 结构
struct ShadertoyUniforms {
    glm::vec3 iResolution;          // 视口分辨率 (width, height, 1.0)
    float iTime;                     // 运行时间 (秒)
    float iTimeDelta;                // 帧时间差 (秒)
    int iFrame;                      // 帧计数器
    glm::vec4 iMouse;                // 鼠标 (x, y, clickX, clickY)
    glm::vec4 iDate;                 // 日期 (year, month, day, seconds)
    float iSampleRate;               // 音频采样率
    glm::vec3 iChannelResolution[4]; // 各通道分辨率
    float iChannelTime[4];           // 各通道播放时间
};

class UniformManager {
public:
    UniformManager() = default;
    
    // 从 Application 更新 uniform
    void updateFromApp(const Application& app);
    
    // 应用 uniform 到着色器程序
    void applyToProgram(GLuint program) const;
    void applyUniforms(GLuint program) const { applyToProgram(program); }
    
    // 获取当前 uniform
    const ShadertoyUniforms& getUniforms() const { return m_uniforms; }
    
    // 单独设置各个 uniform
    void setTime(float time) { m_uniforms.iTime = time; }
    void setTimeDelta(float dt) { m_uniforms.iTimeDelta = dt; }
    void setResolution(float w, float h) { m_uniforms.iResolution = glm::vec3(w, h, 1.0f); }
    void setMouse(float x, float y, float clickX, float clickY) { 
        m_uniforms.iMouse = glm::vec4(x, y, clickX, clickY); 
    }
    void setFrame(int frame) { m_uniforms.iFrame = frame; }
    
    // 更新日期时间 (自动获取当前时间)
    void updateDate();
    
    // 设置通道分辨率
    void setChannelResolution(int channel, const glm::vec3& resolution);
    
    // 设置通道时间
    void setChannelTime(int channel, float time);

private:
    ShadertoyUniforms m_uniforms{};
};

} // namespace shadertoy
