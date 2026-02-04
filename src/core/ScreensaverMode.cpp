#include "ScreensaverMode.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <shlobj.h>

namespace shadertoy {

std::vector<BuiltinShader> ScreensaverMode::s_builtinShaders;
bool ScreensaverMode::s_initialized = false;

// 辅助函数：转换为小写
static std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

// 辅助函数：去除前后空格
static std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    size_t end = str.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    return str.substr(start, end - start + 1);
}

ScreensaverRunMode ScreensaverMode::parseCommandLine(int argc, char* argv[], HWND& previewHwnd) {
    previewHwnd = nullptr;
    
    if (argc < 2) {
        return ScreensaverRunMode::Editor;
    }
    
    std::string arg = toLower(argv[1]);
    
    // 处理 /s 或 -s
    if (arg == "/s" || arg == "-s") {
        return ScreensaverRunMode::Screensaver;
    }
    
    // 处理 /c 或 /c:hwnd
    if (arg.substr(0, 2) == "/c" || arg.substr(0, 2) == "-c") {
        return ScreensaverRunMode::Configure;
    }
    
    // 处理 /p hwnd
    if (arg == "/p" || arg == "-p") {
        if (argc > 2) {
            previewHwnd = (HWND)(uintptr_t)std::stoull(argv[2]);
        }
        return ScreensaverRunMode::Preview;
    }
    
    return ScreensaverRunMode::Editor;
}

ScreensaverRunMode ScreensaverMode::parseCommandLine(const std::string& cmdLine, HWND& previewHwnd) {
    previewHwnd = nullptr;
    
    std::string cmd = toLower(trim(cmdLine));
    
    if (cmd.empty()) {
        return ScreensaverRunMode::Editor;
    }
    
    // 处理 /s
    if (cmd == "/s" || cmd == "-s" || cmd.find("/s") != std::string::npos) {
        return ScreensaverRunMode::Screensaver;
    }
    
    // 处理 /c
    if (cmd.substr(0, 2) == "/c" || cmd.substr(0, 2) == "-c") {
        return ScreensaverRunMode::Configure;
    }
    
    // 处理 /p hwnd
    if (cmd.substr(0, 2) == "/p" || cmd.substr(0, 2) == "-p") {
        // 尝试解析 hwnd
        size_t pos = cmd.find_first_of(" \t:");
        if (pos != std::string::npos) {
            std::string hwndStr = trim(cmd.substr(pos + 1));
            if (!hwndStr.empty()) {
                try {
                    previewHwnd = (HWND)(uintptr_t)std::stoull(hwndStr);
                } catch (...) {
                    previewHwnd = nullptr;
                }
            }
        }
        return ScreensaverRunMode::Preview;
    }
    
    return ScreensaverRunMode::Editor;
}

std::string ScreensaverMode::getConfigPath() {
    char appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, appDataPath))) {
        std::string configDir = std::string(appDataPath) + "\\LocalShadertoy";
        CreateDirectoryA(configDir.c_str(), nullptr);
        return configDir + "\\config.json";
    }
    return "config.json";
}

bool ScreensaverMode::loadConfig(ScreensaverConfig& config) {
    try {
        std::ifstream file(getConfigPath());
        if (!file.is_open()) {
            return false;
        }
        
        nlohmann::json j;
        file >> j;
        
        // 新版本：读取 profiles 数组
        if (j.contains("profiles") && j["profiles"].is_array()) {
            config.profiles.clear();
            for (const auto& pj : j["profiles"]) {
                ScreensaverProfile profile;
                if (pj.contains("name")) profile.name = pj["name"].get<std::string>();
                if (pj.contains("shaderCode")) profile.shaderCode = pj["shaderCode"].get<std::string>();
                if (pj.contains("timeScale")) profile.timeScale = pj["timeScale"].get<float>();
                if (pj.contains("channelBindings")) {
                    for (int i = 0; i < 4 && i < static_cast<int>(pj["channelBindings"].size()); i++) {
                        profile.channelBindings[i] = pj["channelBindings"][i].get<int>();
                    }
                }
                config.profiles.push_back(profile);
            }
            if (j.contains("activeProfileIndex")) {
                config.activeProfileIndex = j["activeProfileIndex"].get<int>();
            }
        }
        // 兼容旧版本配置：迁移为单个 profile
        else if (j.contains("shaderCode") || j.contains("useBuiltinShader")) {
            config.profiles.clear();
            
            // 读取旧字段
            if (j.contains("shaderPath")) config.shaderPath = j["shaderPath"].get<std::string>();
            if (j.contains("shaderCode")) config.shaderCode = j["shaderCode"].get<std::string>();
            if (j.contains("selectedBuiltinIndex")) config.selectedBuiltinIndex = j["selectedBuiltinIndex"].get<int>();
            if (j.contains("useBuiltinShader")) config.useBuiltinShader = j["useBuiltinShader"].get<bool>();
            if (j.contains("timeScale")) config.timeScale = j["timeScale"].get<float>();
            if (j.contains("showFPS")) config.showFPS = j["showFPS"].get<bool>();
            if (j.contains("channelBindings")) {
                auto& bindings = j["channelBindings"];
                for (int i = 0; i < 4 && i < static_cast<int>(bindings.size()); i++) {
                    config.channelBindings[i] = bindings[i].get<int>();
                }
            }
            
            // 迁移为 profile
            ScreensaverProfile profile;
            if (config.useBuiltinShader) {
                const auto& builtins = getBuiltinShaders();
                if (config.selectedBuiltinIndex >= 0 && 
                    config.selectedBuiltinIndex < static_cast<int>(builtins.size())) {
                    profile.name = builtins[static_cast<size_t>(config.selectedBuiltinIndex)].name;
                    profile.shaderCode = builtins[static_cast<size_t>(config.selectedBuiltinIndex)].code;
                } else {
                    profile.name = "Default";
                    profile.shaderCode = builtins.empty() ? "" : builtins[0].code;
                }
            } else {
                profile.name = "Custom Shader";
                profile.shaderCode = config.shaderCode;
            }
            profile.timeScale = config.timeScale;
            for (int i = 0; i < 4; i++) {
                profile.channelBindings[i] = config.channelBindings[i];
            }
            config.profiles.push_back(profile);
            config.activeProfileIndex = 0;
        }
        
        return true;
    } catch (...) {
        return false;
    }
}

bool ScreensaverMode::saveConfig(const ScreensaverConfig& config) {
    try {
        nlohmann::json j;
        
        // 保存 profiles 数组
        j["profiles"] = nlohmann::json::array();
        for (const auto& profile : config.profiles) {
            nlohmann::json pj;
            pj["name"] = profile.name;
            pj["shaderCode"] = profile.shaderCode;
            pj["timeScale"] = profile.timeScale;
            pj["channelBindings"] = {
                profile.channelBindings[0],
                profile.channelBindings[1],
                profile.channelBindings[2],
                profile.channelBindings[3]
            };
            j["profiles"].push_back(pj);
        }
        j["activeProfileIndex"] = config.activeProfileIndex;
        
        std::ofstream file(getConfigPath());
        if (!file.is_open()) {
            return false;
        }
        
        file << j.dump(2);
        return true;
    } catch (...) {
        return false;
    }
}

void ScreensaverMode::initBuiltinShaders() {
    if (s_initialized) return;
    s_initialized = true;
    
    // 1. 经典等离子效果
    s_builtinShaders.push_back({
        "Plasma",
        "Classic plasma effect with flowing colors",
        R"(
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = fragCoord / iResolution.xy;
    vec2 p = uv * 8.0 - 4.0;
    float t = iTime * 0.5;
    
    float v = sin(p.x + t);
    v += sin((p.y + t) * 0.5);
    v += sin((p.x + p.y + t) * 0.5);
    v += sin(sqrt(p.x*p.x + p.y*p.y) + t);
    
    vec3 col = vec3(
        sin(v * 3.14159),
        sin(v * 3.14159 + 2.094),
        sin(v * 3.14159 + 4.188)
    ) * 0.5 + 0.5;
    
    fragColor = vec4(col, 1.0);
}
)"
    });
    
    // 2. 旋转彩虹隧道
    s_builtinShaders.push_back({
        "Rainbow Tunnel",
        "Hypnotic rainbow spiral tunnel",
        R"(
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;
    float a = atan(uv.y, uv.x);
    float r = length(uv);
    
    float t = iTime;
    float spiral = a * 3.0 + log(r) * 10.0 - t * 2.0;
    
    vec3 col = 0.5 + 0.5 * cos(spiral + vec3(0, 2.094, 4.188));
    col *= smoothstep(0.0, 0.02, r);
    col *= 1.0 - smoothstep(0.8, 1.5, r);
    
    fragColor = vec4(col, 1.0);
}
)"
    });
    
    // 3. 星空飞行
    s_builtinShaders.push_back({
        "Starfield",
        "Flying through a starfield",
        R"(
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;
    vec3 col = vec3(0.0);
    
    float t = iTime * 0.3;
    
    for (int layer = 0; layer < 4; layer++) {
        float depth = float(layer) * 0.25 + 0.25;
        float scale = 1.0 / depth;
        vec2 offset = vec2(t * depth, 0.0);
        
        vec2 grid = floor((uv + offset) * scale * 20.0);
        vec2 local = fract((uv + offset) * scale * 20.0) - 0.5;
        
        float rnd = hash(grid);
        if (rnd > 0.85) {
            float star = 0.03 / (length(local) + 0.01);
            star *= smoothstep(1.0, 0.0, length(local) * 2.0);
            float flicker = 0.8 + 0.2 * sin(t * 10.0 + rnd * 100.0);
            col += star * flicker * vec3(0.9, 0.95, 1.0) * depth;
        }
    }
    
    fragColor = vec4(col, 1.0);
}
)"
    });
    
    // 4. 波浪海洋
    s_builtinShaders.push_back({
        "Ocean Waves",
        "Calm ocean waves at sunset",
        R"(
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = fragCoord / iResolution.xy;
    float t = iTime * 0.3;
    
    // Sky gradient
    vec3 skyTop = vec3(0.1, 0.15, 0.4);
    vec3 skyBottom = vec3(0.8, 0.5, 0.3);
    vec3 sky = mix(skyBottom, skyTop, uv.y);
    
    // Sun
    vec2 sunPos = vec2(0.5, 0.3);
    float sun = 0.05 / (length(uv - sunPos) + 0.01);
    sun = min(sun, 1.0);
    sky += sun * vec3(1.0, 0.8, 0.4);
    
    // Ocean
    float horizon = 0.35;
    float wave = 0.0;
    for (int i = 0; i < 5; i++) {
        float fi = float(i);
        wave += sin(uv.x * (10.0 + fi * 5.0) + t * (1.0 + fi * 0.3)) * 0.01 / (fi + 1.0);
    }
    
    float oceanMask = smoothstep(horizon + wave, horizon + wave + 0.01, uv.y);
    vec3 oceanColor = vec3(0.0, 0.2, 0.4);
    
    // Reflection
    float reflection = sun * 0.5 * (1.0 - uv.y);
    oceanColor += reflection * vec3(1.0, 0.6, 0.3);
    
    vec3 col = mix(oceanColor, sky, oceanMask);
    
    fragColor = vec4(col, 1.0);
}
)"
    });
    
    // 5. 分形曼德勃罗
    s_builtinShaders.push_back({
        "Mandelbrot Zoom",
        "Animated Mandelbrot fractal zoom",
        R"(
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;
    
    float zoom = pow(1.5, mod(iTime * 0.5, 20.0));
    vec2 center = vec2(-0.745, 0.186);
    vec2 c = uv / zoom + center;
    
    vec2 z = vec2(0.0);
    float iter = 0.0;
    const float maxIter = 100.0;
    
    for (float i = 0.0; i < maxIter; i++) {
        z = vec2(z.x*z.x - z.y*z.y, 2.0*z.x*z.y) + c;
        if (dot(z, z) > 4.0) {
            iter = i;
            break;
        }
        iter = i;
    }
    
    float t = iter / maxIter;
    vec3 col = 0.5 + 0.5 * cos(3.0 + t * 6.28 + vec3(0, 0.6, 1.0));
    if (iter >= maxIter - 1.0) col = vec3(0.0);
    
    fragColor = vec4(col, 1.0);
}
)"
    });
    
    // 6. 流光粒子
    s_builtinShaders.push_back({
        "Flow Particles",
        "Flowing particle streams",
        R"(
float hash21(vec2 p) {
    return fract(sin(dot(p, vec2(41.1, 289.7))) * 43758.5453);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = fragCoord / iResolution.xy;
    vec3 col = vec3(0.02, 0.02, 0.05);
    
    float t = iTime;
    
    for (int i = 0; i < 50; i++) {
        float fi = float(i);
        float seed = hash21(vec2(fi, 0.0));
        
        vec2 pos;
        pos.x = fract(seed + t * 0.1 * (0.5 + seed));
        pos.y = fract(seed * 7.0 + sin(t * 0.5 + seed * 10.0) * 0.3 + 0.5);
        
        float d = length(uv - pos);
        float glow = 0.003 / (d + 0.001);
        
        vec3 particleCol = 0.5 + 0.5 * cos(seed * 6.28 + vec3(0, 2.0, 4.0));
        col += glow * particleCol * 0.3;
    }
    
    fragColor = vec4(col, 1.0);
}
)"
    });
    
    // 7. 几何万花筒
    s_builtinShaders.push_back({
        "Kaleidoscope",
        "Geometric kaleidoscope patterns",
        R"(
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;
    float t = iTime * 0.3;
    
    // Kaleidoscope fold
    float a = atan(uv.y, uv.x);
    float segments = 6.0;
    a = mod(a, 3.14159 * 2.0 / segments);
    a = abs(a - 3.14159 / segments);
    
    float r = length(uv);
    uv = vec2(cos(a), sin(a)) * r;
    
    // Pattern
    vec3 col = vec3(0.0);
    for (int i = 0; i < 3; i++) {
        float fi = float(i);
        vec2 p = uv * (3.0 + fi);
        p += t * vec2(1.0 + fi * 0.5, 0.5);
        
        float v = sin(p.x) * sin(p.y);
        v = smoothstep(0.0, 0.1, abs(v) - 0.3);
        
        col[i] = v;
    }
    
    col = 0.5 + 0.5 * cos(col * 6.28 + t + vec3(0, 2.0, 4.0));
    
    fragColor = vec4(col, 1.0);
}
)"
    });
    
    // 8. 极光
    s_builtinShaders.push_back({
        "Aurora Borealis",
        "Northern lights effect",
        R"(
float noise(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

float smoothNoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    
    float a = noise(i);
    float b = noise(i + vec2(1.0, 0.0));
    float c = noise(i + vec2(0.0, 1.0));
    float d = noise(i + vec2(1.0, 1.0));
    
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = fragCoord / iResolution.xy;
    float t = iTime * 0.2;
    
    // Dark sky gradient
    vec3 col = mix(vec3(0.0, 0.02, 0.05), vec3(0.0, 0.0, 0.02), uv.y);
    
    // Aurora layers
    for (int i = 0; i < 3; i++) {
        float fi = float(i);
        float y = uv.y + 0.3;
        float wave = 0.0;
        
        for (int j = 0; j < 4; j++) {
            float fj = float(j);
            wave += sin(uv.x * (3.0 + fj) + t * (0.5 + fi * 0.2) + fi) * 0.1 / (fj + 1.0);
        }
        
        float n = smoothNoise(vec2(uv.x * 5.0 + t, fi * 10.0)) * 0.2;
        float aurora = exp(-pow((y - 0.5 - wave - n) * 4.0, 2.0));
        
        vec3 auroraCol = mix(
            vec3(0.0, 1.0, 0.5),
            vec3(0.5, 0.0, 1.0),
            fi / 3.0 + n
        );
        
        col += aurora * auroraCol * 0.4;
    }
    
    // Stars
    float star = noise(uv * 500.0);
    if (star > 0.995) {
        col += vec3(1.0) * (star - 0.995) * 200.0;
    }
    
    fragColor = vec4(col, 1.0);
}
)"
    });
}

const std::vector<BuiltinShader>& ScreensaverMode::getBuiltinShaders() {
    if (!s_initialized) {
        initBuiltinShaders();
    }
    return s_builtinShaders;
}

} // namespace shadertoy