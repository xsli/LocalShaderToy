#pragma once

#include <string>
#include <vector>
#include <array>
#include <map>
#include <nlohmann/json.hpp>

namespace shadertoy {

// 输入通道类型
enum class ChannelType {
    None,
    Texture,        // 2D纹理
    Cubemap,        // 立方体贴图
    Buffer,         // Buffer Pass输出
    Keyboard,       // 键盘输入
    Audio,          // 音频输入
    Video,          // 视频输入
    Webcam          // 摄像头
};

// 纹理采样配置
struct SamplerConfig {
    std::string filter = "mipmap";    // nearest, linear, mipmap
    std::string wrap = "repeat";       // clamp, repeat
    bool vflip = true;                 // 垂直翻转
};

// 输入通道配置
struct ChannelConfig {
    ChannelType type = ChannelType::None;
    std::string source;                // 资源路径或Buffer名称
    int bufferId = -1;                 // Buffer ID (0-3 for Buffer A-D)
    SamplerConfig sampler;
};

// Shader Pass 类型
enum class PassType {
    Image,          // 主Image pass (输出到屏幕)
    BufferA,        // Buffer A
    BufferB,        // Buffer B
    BufferC,        // Buffer C
    BufferD,        // Buffer D
    Common,         // 共享代码 (不是真正的pass)
    Sound           // 音频shader
};

// 单个 Shader Pass
struct ShaderPass {
    PassType type = PassType::Image;
    std::string name;                   // Pass名称
    std::string code;                   // GLSL代码
    std::array<ChannelConfig, 4> inputs; // iChannel0-3
    bool enabled = true;
    
    // 获取类型字符串
    static std::string passTypeToString(PassType type);
    static PassType stringToPassType(const std::string& str);
};

// Shader 项目
struct ShaderProject {
    // 元数据
    std::string name = "Untitled";
    std::string description;
    std::string author;
    std::string license = "CC BY-NC-SA 3.0";
    std::vector<std::string> tags;
    
    // Shader passes
    std::vector<ShaderPass> passes;
    std::string commonCode;             // 共享代码段
    
    // 项目设置
    float startTime = 0.0f;
    bool autoPlay = true;
    
    // 文件路径 (用于保存)
    std::string filePath;
    bool modified = false;
    
    // 默认构造函数 - 创建基本Image pass
    ShaderProject();
    
    // 从 Shadertoy JSON API 格式加载
    static ShaderProject fromShadertoyJson(const std::string& jsonStr);
    
    // 从简单的shader代码创建 (只有Image pass)
    static ShaderProject fromCode(const std::string& code, const std::string& name = "Untitled");
    
    // 导出为 JSON
    std::string toJson() const;
    
    // 从 JSON 加载
    static ShaderProject fromJson(const std::string& jsonStr);
    
    // 获取指定类型的Pass
    ShaderPass* getPass(PassType type);
    const ShaderPass* getPass(PassType type) const;
    
    // 获取Image pass代码 (最常用)
    std::string getImageCode() const;
    void setImageCode(const std::string& code);
    
    // 检查是否有多Pass
    bool hasMultiplePasses() const;
    
    // 获取所有启用的Buffer passes
    std::vector<ShaderPass*> getBufferPasses();
};

// JSON 序列化辅助
void to_json(nlohmann::json& j, const SamplerConfig& s);
void from_json(const nlohmann::json& j, SamplerConfig& s);
void to_json(nlohmann::json& j, const ChannelConfig& c);
void from_json(const nlohmann::json& j, ChannelConfig& c);
void to_json(nlohmann::json& j, const ShaderPass& p);
void from_json(const nlohmann::json& j, ShaderPass& p);
void to_json(nlohmann::json& j, const ShaderProject& proj);
void from_json(const nlohmann::json& j, ShaderProject& proj);

} // namespace shadertoy
