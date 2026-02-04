#pragma once

#include <string>
#include <vector>
#include <windows.h>

namespace shadertoy {

// 屏幕保护程序运行模式
enum class ScreensaverRunMode {
    Editor,         // 正常编辑器模式 (无参数)
    Screensaver,    // 屏保模式 (/s)
    Configure,      // 配置模式 (/c)
    Preview         // 预览模式 (/p hwnd)
};

// 单个屏保配置档案
struct ScreensaverProfile {
    std::string name;               // 配置名称
    std::string shaderCode;         // shader 代码
    float timeScale = 1.0f;         // 时间缩放
    int channelBindings[4] = {-1, -1, -1, -1};  // iChannel 绑定
    
    // 默认构造
    ScreensaverProfile() = default;
    ScreensaverProfile(const std::string& n, const std::string& code) 
        : name(n), shaderCode(code) {}
};

// 屏保配置（包含多个 Profile）
struct ScreensaverConfig {
    std::vector<ScreensaverProfile> profiles;   // 配置档案列表
    int activeProfileIndex = 0;                  // 当前激活的配置索引
    
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