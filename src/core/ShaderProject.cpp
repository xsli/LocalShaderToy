#include "ShaderProject.h"
#include <sstream>
#include <iostream>
#include <regex>

namespace shadertoy {

// 默认的 Shadertoy shader
static const char* DEFAULT_SHADER_CODE = R"(
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;

    // Time varying pixel color
    vec3 col = 0.5 + 0.5*cos(iTime+uv.xyx+vec3(0,2,4));

    // Output to screen
    fragColor = vec4(col,1.0);
}
)";

// PassType 转换
std::string ShaderPass::passTypeToString(PassType type) {
    switch (type) {
        case PassType::Image: return "image";
        case PassType::BufferA: return "buffer_a";
        case PassType::BufferB: return "buffer_b";
        case PassType::BufferC: return "buffer_c";
        case PassType::BufferD: return "buffer_d";
        case PassType::Common: return "common";
        case PassType::Sound: return "sound";
        default: return "image";
    }
}

PassType ShaderPass::stringToPassType(const std::string& str) {
    if (str == "image") return PassType::Image;
    if (str == "buffer_a" || str == "Buffer A") return PassType::BufferA;
    if (str == "buffer_b" || str == "Buffer B") return PassType::BufferB;
    if (str == "buffer_c" || str == "Buffer C") return PassType::BufferC;
    if (str == "buffer_d" || str == "Buffer D") return PassType::BufferD;
    if (str == "common") return PassType::Common;
    if (str == "sound") return PassType::Sound;
    return PassType::Image;
}

// ShaderProject 构造函数
ShaderProject::ShaderProject() {
    // 创建默认的 Image pass
    ShaderPass imagePass;
    imagePass.type = PassType::Image;
    imagePass.name = "Image";
    imagePass.code = DEFAULT_SHADER_CODE;
    passes.push_back(imagePass);
}

// 从简单代码创建项目
ShaderProject ShaderProject::fromCode(const std::string& code, const std::string& name) {
    ShaderProject project;
    project.name = name;
    project.passes[0].code = code;
    return project;
}

// 获取 Image pass
ShaderPass* ShaderProject::getPass(PassType type) {
    for (auto& pass : passes) {
        if (pass.type == type) return &pass;
    }
    return nullptr;
}

const ShaderPass* ShaderProject::getPass(PassType type) const {
    for (const auto& pass : passes) {
        if (pass.type == type) return &pass;
    }
    return nullptr;
}

// 获取和设置 Image 代码
std::string ShaderProject::getImageCode() const {
    const ShaderPass* pass = getPass(PassType::Image);
    return pass ? pass->code : "";
}

void ShaderProject::setImageCode(const std::string& code) {
    ShaderPass* pass = getPass(PassType::Image);
    if (pass) {
        pass->code = code;
        modified = true;
    }
}

// 检查是否有多Pass
bool ShaderProject::hasMultiplePasses() const {
    int count = 0;
    for (const auto& pass : passes) {
        if (pass.type != PassType::Common && pass.enabled) {
            count++;
        }
    }
    return count > 1;
}

// 获取Buffer passes
std::vector<ShaderPass*> ShaderProject::getBufferPasses() {
    std::vector<ShaderPass*> buffers;
    for (auto& pass : passes) {
        if (pass.enabled && 
            (pass.type == PassType::BufferA || 
             pass.type == PassType::BufferB ||
             pass.type == PassType::BufferC || 
             pass.type == PassType::BufferD)) {
            buffers.push_back(&pass);
        }
    }
    return buffers;
}

// JSON 序列化
void to_json(nlohmann::json& j, const SamplerConfig& s) {
    j = nlohmann::json{
        {"filter", s.filter},
        {"wrap", s.wrap},
        {"vflip", s.vflip}
    };
}

void from_json(const nlohmann::json& j, SamplerConfig& s) {
    if (j.contains("filter")) s.filter = j["filter"].get<std::string>();
    if (j.contains("wrap")) s.wrap = j["wrap"].get<std::string>();
    if (j.contains("vflip")) s.vflip = j["vflip"].get<bool>();
}

static std::string channelTypeToString(ChannelType type) {
    switch (type) {
        case ChannelType::None: return "none";
        case ChannelType::Texture: return "texture";
        case ChannelType::Cubemap: return "cubemap";
        case ChannelType::Buffer: return "buffer";
        case ChannelType::Keyboard: return "keyboard";
        case ChannelType::Audio: return "audio";
        case ChannelType::Video: return "video";
        case ChannelType::Webcam: return "webcam";
        default: return "none";
    }
}

static ChannelType stringToChannelType(const std::string& str) {
    if (str == "texture") return ChannelType::Texture;
    if (str == "cubemap") return ChannelType::Cubemap;
    if (str == "buffer") return ChannelType::Buffer;
    if (str == "keyboard") return ChannelType::Keyboard;
    if (str == "audio") return ChannelType::Audio;
    if (str == "video") return ChannelType::Video;
    if (str == "webcam") return ChannelType::Webcam;
    return ChannelType::None;
}

void to_json(nlohmann::json& j, const ChannelConfig& c) {
    j = nlohmann::json{
        {"type", channelTypeToString(c.type)},
        {"source", c.source},
        {"bufferId", c.bufferId},
        {"sampler", c.sampler}
    };
}

void from_json(const nlohmann::json& j, ChannelConfig& c) {
    if (j.contains("type")) c.type = stringToChannelType(j["type"].get<std::string>());
    if (j.contains("source")) c.source = j["source"].get<std::string>();
    if (j.contains("bufferId")) c.bufferId = j["bufferId"].get<int>();
    if (j.contains("sampler")) c.sampler = j["sampler"].get<SamplerConfig>();
}

void to_json(nlohmann::json& j, const ShaderPass& p) {
    j = nlohmann::json{
        {"type", ShaderPass::passTypeToString(p.type)},
        {"name", p.name},
        {"code", p.code},
        {"enabled", p.enabled}
    };
    
    // 只保存非空的输入通道
    nlohmann::json inputs = nlohmann::json::array();
    for (const auto& input : p.inputs) {
        inputs.push_back(input);
    }
    j["inputs"] = inputs;
}

void from_json(const nlohmann::json& j, ShaderPass& p) {
    if (j.contains("type")) p.type = ShaderPass::stringToPassType(j["type"].get<std::string>());
    if (j.contains("name")) p.name = j["name"].get<std::string>();
    if (j.contains("code")) p.code = j["code"].get<std::string>();
    if (j.contains("enabled")) p.enabled = j["enabled"].get<bool>();
    
    if (j.contains("inputs") && j["inputs"].is_array()) {
        size_t i = 0;
        for (const auto& input : j["inputs"]) {
            if (i < 4) {
                p.inputs[i] = input.get<ChannelConfig>();
                i++;
            }
        }
    }
}

void to_json(nlohmann::json& j, const ShaderProject& proj) {
    j = nlohmann::json{
        {"version", "1.0"},
        {"name", proj.name},
        {"description", proj.description},
        {"author", proj.author},
        {"license", proj.license},
        {"tags", proj.tags},
        {"passes", proj.passes},
        {"commonCode", proj.commonCode},
        {"startTime", proj.startTime},
        {"autoPlay", proj.autoPlay}
    };
}

void from_json(const nlohmann::json& j, ShaderProject& proj) {
    if (j.contains("name")) proj.name = j["name"].get<std::string>();
    if (j.contains("description")) proj.description = j["description"].get<std::string>();
    if (j.contains("author")) proj.author = j["author"].get<std::string>();
    if (j.contains("license")) proj.license = j["license"].get<std::string>();
    if (j.contains("tags")) proj.tags = j["tags"].get<std::vector<std::string>>();
    if (j.contains("passes")) {
        proj.passes.clear();
        for (const auto& passJson : j["passes"]) {
            proj.passes.push_back(passJson.get<ShaderPass>());
        }
    }
    if (j.contains("commonCode")) proj.commonCode = j["commonCode"].get<std::string>();
    if (j.contains("startTime")) proj.startTime = j["startTime"].get<float>();
    if (j.contains("autoPlay")) proj.autoPlay = j["autoPlay"].get<bool>();
}

std::string ShaderProject::toJson() const {
    nlohmann::json j = *this;
    return j.dump(2);
}

ShaderProject ShaderProject::fromJson(const std::string& jsonStr) {
    try {
        nlohmann::json j = nlohmann::json::parse(jsonStr);
        
        // 检查是否是 Shadertoy API 格式
        if (j.contains("Shader") || j.contains("ver")) {
            return fromShadertoyJson(jsonStr);
        }
        
        ShaderProject proj;
        proj.passes.clear();  // 清除默认pass
        from_json(j, proj);
        return proj;
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse JSON: " << e.what() << std::endl;
        return ShaderProject();
    }
}

// 从 Shadertoy API JSON 格式加载
ShaderProject ShaderProject::fromShadertoyJson(const std::string& jsonStr) {
    ShaderProject proj;
    proj.passes.clear();
    
    try {
        nlohmann::json j = nlohmann::json::parse(jsonStr);
        
        // 处理 Shadertoy API 格式 {"Shader": {...}}
        nlohmann::json shaderData;
        if (j.contains("Shader")) {
            shaderData = j["Shader"];
        } else {
            shaderData = j;
        }
        
        // 元数据
        if (shaderData.contains("info")) {
            const auto& info = shaderData["info"];
            if (info.contains("name")) proj.name = info["name"].get<std::string>();
            if (info.contains("description")) proj.description = info["description"].get<std::string>();
            if (info.contains("username")) proj.author = info["username"].get<std::string>();
            if (info.contains("tags")) proj.tags = info["tags"].get<std::vector<std::string>>();
        }
        
        // Render passes
        if (shaderData.contains("renderpass")) {
            for (const auto& rp : shaderData["renderpass"]) {
                ShaderPass pass;
                
                // 确定pass类型
                std::string type = rp.value("type", "image");
                std::string name = rp.value("name", "");
                
                if (type == "image") {
                    pass.type = PassType::Image;
                    pass.name = "Image";
                } else if (type == "buffer") {
                    // 根据name判断是哪个Buffer
                    if (name == "Buffer A") pass.type = PassType::BufferA;
                    else if (name == "Buffer B") pass.type = PassType::BufferB;
                    else if (name == "Buffer C") pass.type = PassType::BufferC;
                    else if (name == "Buffer D") pass.type = PassType::BufferD;
                    else pass.type = PassType::BufferA;
                    pass.name = name;
                } else if (type == "common") {
                    pass.type = PassType::Common;
                    pass.name = "Common";
                } else if (type == "sound") {
                    pass.type = PassType::Sound;
                    pass.name = "Sound";
                }
                
                // Shader代码
                if (rp.contains("code")) {
                    pass.code = rp["code"].get<std::string>();
                }
                
                // 输入通道
                if (rp.contains("inputs")) {
                    for (const auto& input : rp["inputs"]) {
                        int channel = input.value("channel", 0);
                        if (channel >= 0 && channel < 4) {
                            ChannelConfig& cfg = pass.inputs[channel];
                            
                            std::string ctype = input.value("ctype", "");
                            if (ctype == "texture") {
                                cfg.type = ChannelType::Texture;
                                if (input.contains("src")) {
                                    cfg.source = input["src"].get<std::string>();
                                }
                            } else if (ctype == "buffer") {
                                cfg.type = ChannelType::Buffer;
                                if (input.contains("id")) {
                                    // Shadertoy buffer id format
                                    int id = input["id"].get<int>();
                                    cfg.bufferId = id;
                                }
                            } else if (ctype == "keyboard") {
                                cfg.type = ChannelType::Keyboard;
                            } else if (ctype == "music" || ctype == "musicstream") {
                                cfg.type = ChannelType::Audio;
                            } else if (ctype == "cubemap") {
                                cfg.type = ChannelType::Cubemap;
                                if (input.contains("src")) {
                                    cfg.source = input["src"].get<std::string>();
                                }
                            }
                            
                            // 采样器配置
                            if (input.contains("sampler")) {
                                const auto& sampler = input["sampler"];
                                if (sampler.contains("filter")) cfg.sampler.filter = sampler["filter"].get<std::string>();
                                if (sampler.contains("wrap")) cfg.sampler.wrap = sampler["wrap"].get<std::string>();
                                if (sampler.contains("vflip")) {
                                    if (sampler["vflip"].is_boolean()) {
                                        cfg.sampler.vflip = sampler["vflip"].get<bool>();
                                    } else if (sampler["vflip"].is_string()) {
                                        cfg.sampler.vflip = (sampler["vflip"].get<std::string>() == "true");
                                    }
                                }
                            }
                        }
                    }
                }
                
                proj.passes.push_back(pass);
            }
        }
        
        // 如果没有pass，创建默认的
        if (proj.passes.empty()) {
            proj = ShaderProject();
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse Shadertoy JSON: " << e.what() << std::endl;
        proj = ShaderProject();
    }
    
    return proj;
}

} // namespace shadertoy
