#pragma once

#include <glad/glad.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace shadertoy {

// 内置纹理类型
enum class BuiltinTextureType {
    // 噪声纹理
    GrayNoise256,       // 256x256 灰度噪声
    GrayNoiseMedium,    // 64x64 灰度噪声
    GrayNoiseSmall,     // 32x32 灰度噪声
    RGBANoise256,       // 256x256 RGBA噪声
    RGBANoiseMedium,    // 64x64 RGBA噪声
    RGBANoiseSmall,     // 32x32 RGBA噪声
    
    // Perlin噪声
    PerlinNoise256,     // 256x256 Perlin噪声
    PerlinNoise512,     // 512x512 Perlin噪声
    
    // 有机噪声 (类似Shadertoy的Organic纹理)
    OrganicNoise,       // 256x256 有机纹理
    
    // 瓷砖/图案
    BlueNoise,          // 蓝噪声 (用于抖动)
    Checkerboard,       // 棋盘格
    
    // 其他
    Black,              // 纯黑
    White,              // 纯白
    UV,                 // UV渐变
    
    Count
};

// 纹理信息
struct TextureInfo {
    std::string name;
    std::string description;
    int width;
    int height;
    int channels;
    GLuint id;
    BuiltinTextureType type;
    bool isTileable;
};

// 纹理管理器
class TextureManager {
public:
    static TextureManager& instance();
    
    // 禁止拷贝
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;
    
    // 初始化所有内置纹理
    bool init();
    
    // 清理资源
    void cleanup();
    
    // 获取内置纹理
    GLuint getBuiltinTexture(BuiltinTextureType type) const;
    const TextureInfo* getTextureInfo(BuiltinTextureType type) const;
    
    // 获取所有内置纹理列表
    const std::vector<TextureInfo>& getBuiltinTextures() const { return m_builtinTextures; }
    
    // 加载用户纹理
    GLuint loadUserTexture(const std::string& path);
    void unloadUserTexture(GLuint id);
    
    // 绑定纹理到指定单元
    void bindTexture(GLuint textureId, int unit) const;
    void unbindTexture(int unit) const;
    
    // 获取纹理名称 (用于UI显示)
    static std::string getTextureName(BuiltinTextureType type);

private:
    TextureManager() = default;
    ~TextureManager();
    
    // 生成各种噪声纹理
    GLuint createGrayNoiseTexture(int width, int height, unsigned int seed = 0);
    GLuint createRGBANoiseTexture(int width, int height, unsigned int seed = 0);
    GLuint createPerlinNoiseTexture(int width, int height, int octaves = 4);
    GLuint createOrganicNoiseTexture(int width, int height);
    GLuint createBlueNoiseTexture(int width, int height);
    GLuint createCheckerboardTexture(int width, int height, int cellSize = 16);
    GLuint createSolidColorTexture(int width, int height, float r, float g, float b, float a);
    GLuint createUVGradientTexture(int width, int height);
    
    // 上传纹理数据到GPU
    GLuint uploadTexture(const unsigned char* data, int width, int height, int channels, bool tileable);
    
private:
    std::vector<TextureInfo> m_builtinTextures;
    std::unordered_map<GLuint, std::string> m_userTextures;
    bool m_initialized = false;
};

} // namespace shadertoy
