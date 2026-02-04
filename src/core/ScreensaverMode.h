#pragma once

#include <string>
#include <vector>
#include <array>
#include <windows.h>

namespace shadertoy {

// 屏幕保护程序运行模式
enum class ScreensaverRunMode {
    Editor,         // 正常编辑器模式 (无参数)
    Screensaver,    // 屏保模式 (/s)
    Configure,      // 配置模式 (/c)
    Preview         // 预览模式 (/p hwnd)
};

// Pass 类型枚举
enum class ShaderPassType {
    Image = 0,      // 主输出 (必选)
    Common,         // 共享代码
    BufferA,        // Buffer A
    BufferB,        // Buffer B
    BufferC,        // Buffer C
    BufferD         // Buffer D
};

// Channel 绑定类型常量
namespace ChannelBind {
    constexpr int None = -1;
    // 0-99: 纹理索引
    constexpr int BufferA = 100;
    constexpr int BufferB = 101;
    constexpr int BufferC = 102;
    constexpr int BufferD = 103;
    
    inline bool isBuffer(int binding) { return binding >= 100 && binding <= 103; }
    inline int bufferIndex(int binding) { return binding - 100; } // 0=A, 1=B, 2=C, 3=D
}

// 单个 Pass 的配置
struct PassConfig {
    ShaderPassType type = ShaderPassType::Image;
    std::string code;                                   // shader 代码
    std::array<int, 4> channels = {-1, -1, -1, -1};     // iChannel 绑定
    bool enabled = true;                                // 是否启用
    
    PassConfig() = default;
    PassConfig(ShaderPassType t) : type(t) {}
    PassConfig(ShaderPassType t, const std::string& c) : type(t), code(c) {}
    
    // 判断是否有有效代码
    bool hasCode() const { return !code.empty(); }
    
    // 获取类型名称
    static const char* getTypeName(ShaderPassType type) {
        switch (type) {
            case ShaderPassType::Image: return "Image";
            case ShaderPassType::Common: return "Common";
            case ShaderPassType::BufferA: return "Buffer A";
            case ShaderPassType::BufferB: return "Buffer B";
            case ShaderPassType::BufferC: return "Buffer C";
            case ShaderPassType::BufferD: return "Buffer D";
            default: return "Unknown";
        }
    }
    
    const char* getTypeName() const { return getTypeName(type); }
};

// 单个屏保配置档案 (支持 Multi-pass)
struct ScreensaverProfile {
    std::string name;               // 配置名称
    float timeScale = 1.0f;         // 时间缩放
    bool includeInRandom = true;    // 是否参与随机播放
    
    // Multi-pass 配置
    std::vector<PassConfig> passes; // 所有 Pass（至少包含 Image）
    
    // === 向后兼容字段 (加载旧配置时使用) ===
    std::string shaderCode;                     // 旧格式: 单一 shader → Image pass
    int channelBindings[4] = {-1, -1, -1, -1};  // 旧格式: iChannel → Image channels
    
    // 默认构造 - 确保有 Image pass
    ScreensaverProfile() {
        passes.push_back(PassConfig(ShaderPassType::Image));
    }
    
    ScreensaverProfile(const std::string& n, const std::string& code) 
        : name(n), shaderCode(code) {
        PassConfig imagePass(ShaderPassType::Image, code);
        passes.push_back(imagePass);
    }
    
    // 迁移旧格式到新格式
    void migrateFromLegacy() {
        if (!shaderCode.empty()) {
            PassConfig* imagePass = getPass(ShaderPassType::Image);
            if (imagePass && imagePass->code.empty()) {
                imagePass->code = shaderCode;
                for (int i = 0; i < 4; i++) {
                    imagePass->channels[i] = channelBindings[i];
                }
            } else if (!imagePass) {
                PassConfig newImage(ShaderPassType::Image, shaderCode);
                for (int i = 0; i < 4; i++) {
                    newImage.channels[i] = channelBindings[i];
                }
                passes.insert(passes.begin(), newImage);
            }
        }
        // 确保至少有 Image pass
        if (passes.empty()) {
            passes.push_back(PassConfig(ShaderPassType::Image));
        }
    }
    
    // 同步新格式到旧字段（向后兼容保存）
    void syncToLegacy() {
        const PassConfig* imagePass = getPass(ShaderPassType::Image);
        if (imagePass) {
            shaderCode = imagePass->code;
            for (int i = 0; i < 4; i++) {
                channelBindings[i] = imagePass->channels[i];
            }
        }
    }
    
    // 获取指定类型的 Pass
    PassConfig* getPass(ShaderPassType type) {
        for (auto& p : passes) {
            if (p.type == type) return &p;
        }
        return nullptr;
    }
    
    const PassConfig* getPass(ShaderPassType type) const {
        for (const auto& p : passes) {
            if (p.type == type) return &p;
        }
        return nullptr;
    }
    
    // 快捷方法
    PassConfig* getImagePass() { return getPass(ShaderPassType::Image); }
    const PassConfig* getImagePass() const { return getPass(ShaderPassType::Image); }
    PassConfig* getCommonPass() { return getPass(ShaderPassType::Common); }
    const PassConfig* getCommonPass() const { return getPass(ShaderPassType::Common); }
    
    // 添加 Pass（如果不存在）
    PassConfig* addPass(ShaderPassType type) {
        if (auto* existing = getPass(type)) return existing;
        passes.push_back(PassConfig(type));
        return &passes.back();
    }
    
    // 移除 Pass（Image 不能移除）
    bool removePass(ShaderPassType type) {
        if (type == ShaderPassType::Image) return false;
        for (auto it = passes.begin(); it != passes.end(); ++it) {
            if (it->type == type) {
                passes.erase(it);
                return true;
            }
        }
        return false;
    }
    
    // 检查是否有 Multi-pass
    bool hasMultiPass() const {
        return passes.size() > 1;
    }
    
    // 检查是否有任何有效代码（单 Pass 或多 Pass）
    bool hasAnyCode() const {
        // 检查 passes 数组
        for (const auto& p : passes) {
            if (p.hasCode()) return true;
        }
        // 检查旧格式字段
        return !shaderCode.empty();
    }
    
    // 获取所有启用的 Buffer passes（按 A→B→C→D 顺序）
    std::vector<PassConfig*> getEnabledBufferPasses() {
        std::vector<PassConfig*> result;
        for (ShaderPassType t : {ShaderPassType::BufferA, ShaderPassType::BufferB, 
                                  ShaderPassType::BufferC, ShaderPassType::BufferD}) {
            PassConfig* p = getPass(t);
            if (p && p->enabled && p->hasCode()) {
                result.push_back(p);
            }
        }
        return result;
    }
};

// 屏保配置（包含多个 Profile）
struct ScreensaverConfig {
    std::vector<ScreensaverProfile> profiles;   // 配置档案列表
    int activeProfileIndex = 0;                  // 当前激活的配置索引
    
    // 随机播放设置
    bool randomMode = false;                     // 是否启用随机播放模式
    float randomInterval = 30.0f;                // 随机切换间隔（秒）
    
    // 兼容旧配置的字段（已废弃，仅用于迁移）
    std::string shaderPath;         // shader 文件路径
    std::string shaderCode;         // 缓存的 shader 代码
    int selectedBuiltinIndex = 0;   // 内置 shader 索引 (-1 表示使用自定义)
    bool useBuiltinShader = true;   // 是否使用内置 shader
    float timeScale = 1.0f;         // 时间缩放
    bool showFPS = false;           // 是否显示 FPS (调试用)
    int channelBindings[4] = {-1, -1, -1, -1};
    
    // 获取当前激活的 Profile
    ScreensaverProfile* getActiveProfile() {
        if (profiles.empty()) return nullptr;
        if (activeProfileIndex < 0 || activeProfileIndex >= static_cast<int>(profiles.size())) {
            activeProfileIndex = 0;
        }
        return &profiles[static_cast<size_t>(activeProfileIndex)];
    }
    
    const ScreensaverProfile* getActiveProfile() const {
        if (profiles.empty()) return nullptr;
        if (activeProfileIndex < 0 || activeProfileIndex >= static_cast<int>(profiles.size())) {
            return &profiles[0];
        }
        return &profiles[static_cast<size_t>(activeProfileIndex)];
    }
};

// 内置 shader 定义
struct BuiltinShader {
    std::string name;
    std::string description;
    std::string code;
};

class ScreensaverMode {
public:
    // 解析命令行参数
    static ScreensaverRunMode parseCommandLine(int argc, char* argv[], HWND& previewHwnd);
    static ScreensaverRunMode parseCommandLine(const std::string& cmdLine, HWND& previewHwnd);
    
    // 获取配置文件路径
    static std::string getConfigPath();
    
    // 加载/保存配置
    static bool loadConfig(ScreensaverConfig& config);
    static bool saveConfig(const ScreensaverConfig& config);
    
    // 获取内置 shader 列表
    static const std::vector<BuiltinShader>& getBuiltinShaders();
    
    // 初始化内置 shader
    static void initBuiltinShaders();
    
private:
    static std::vector<BuiltinShader> s_builtinShaders;
    static bool s_initialized;
};

} // namespace shadertoy