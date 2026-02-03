#include "TextureManager.h"
#include "NoiseGenerator.h"
#include <iostream>
#include <cmath>

#include <stb_image.h>

namespace shadertoy {

TextureManager& TextureManager::instance() {
    static TextureManager instance;
    return instance;
}

TextureManager::~TextureManager() {
    cleanup();
}

bool TextureManager::init() {
    if (m_initialized) return true;
    
    std::cout << "Initializing TextureManager..." << std::endl;
    
    // 预分配空间
    m_builtinTextures.reserve(static_cast<size_t>(BuiltinTextureType::Count));
    
    // 创建所有内置纹理
    
    // 灰度噪声
    {
        TextureInfo info;
        info.name = "Gray Noise 256";
        info.description = "256x256 grayscale white noise";
        info.width = 256;
        info.height = 256;
        info.channels = 1;
        info.type = BuiltinTextureType::GrayNoise256;
        info.isTileable = true;
        info.id = createGrayNoiseTexture(256, 256, 12345);
        m_builtinTextures.push_back(info);
    }
    
    {
        TextureInfo info;
        info.name = "Gray Noise Medium";
        info.description = "64x64 grayscale white noise";
        info.width = 64;
        info.height = 64;
        info.channels = 1;
        info.type = BuiltinTextureType::GrayNoiseMedium;
        info.isTileable = true;
        info.id = createGrayNoiseTexture(64, 64, 23456);
        m_builtinTextures.push_back(info);
    }
    
    {
        TextureInfo info;
        info.name = "Gray Noise Small";
        info.description = "32x32 grayscale white noise";
        info.width = 32;
        info.height = 32;
        info.channels = 1;
        info.type = BuiltinTextureType::GrayNoiseSmall;
        info.isTileable = true;
        info.id = createGrayNoiseTexture(32, 32, 34567);
        m_builtinTextures.push_back(info);
    }
    
    // RGBA噪声
    {
        TextureInfo info;
        info.name = "RGBA Noise 256";
        info.description = "256x256 RGBA white noise";
        info.width = 256;
        info.height = 256;
        info.channels = 4;
        info.type = BuiltinTextureType::RGBANoise256;
        info.isTileable = true;
        info.id = createRGBANoiseTexture(256, 256, 45678);
        m_builtinTextures.push_back(info);
    }
    
    {
        TextureInfo info;
        info.name = "RGBA Noise Medium";
        info.description = "64x64 RGBA white noise";
        info.width = 64;
        info.height = 64;
        info.channels = 4;
        info.type = BuiltinTextureType::RGBANoiseMedium;
        info.isTileable = true;
        info.id = createRGBANoiseTexture(64, 64, 56789);
        m_builtinTextures.push_back(info);
    }
    
    {
        TextureInfo info;
        info.name = "RGBA Noise Small";
        info.description = "32x32 RGBA white noise";
        info.width = 32;
        info.height = 32;
        info.channels = 4;
        info.type = BuiltinTextureType::RGBANoiseSmall;
        info.isTileable = true;
        info.id = createRGBANoiseTexture(32, 32, 67890);
        m_builtinTextures.push_back(info);
    }
    
    // Perlin噪声
    {
        TextureInfo info;
        info.name = "Perlin Noise 256";
        info.description = "256x256 Perlin noise (4 octaves)";
        info.width = 256;
        info.height = 256;
        info.channels = 1;
        info.type = BuiltinTextureType::PerlinNoise256;
        info.isTileable = true;
        info.id = createPerlinNoiseTexture(256, 256, 4);
        m_builtinTextures.push_back(info);
    }
    
    {
        TextureInfo info;
        info.name = "Perlin Noise 512";
        info.description = "512x512 Perlin noise (6 octaves)";
        info.width = 512;
        info.height = 512;
        info.channels = 1;
        info.type = BuiltinTextureType::PerlinNoise512;
        info.isTileable = true;
        info.id = createPerlinNoiseTexture(512, 512, 6);
        m_builtinTextures.push_back(info);
    }
    
    // 有机噪声
    {
        TextureInfo info;
        info.name = "Organic Noise";
        info.description = "256x256 organic texture (multi-layer noise)";
        info.width = 256;
        info.height = 256;
        info.channels = 4;
        info.type = BuiltinTextureType::OrganicNoise;
        info.isTileable = true;
        info.id = createOrganicNoiseTexture(256, 256);
        m_builtinTextures.push_back(info);
    }
    
    // 蓝噪声
    {
        TextureInfo info;
        info.name = "Blue Noise";
        info.description = "64x64 blue noise (for dithering)";
        info.width = 64;
        info.height = 64;
        info.channels = 1;
        info.type = BuiltinTextureType::BlueNoise;
        info.isTileable = true;
        info.id = createBlueNoiseTexture(64, 64);
        m_builtinTextures.push_back(info);
    }
    
    // 棋盘格
    {
        TextureInfo info;
        info.name = "Checkerboard";
        info.description = "256x256 checkerboard pattern";
        info.width = 256;
        info.height = 256;
        info.channels = 1;
        info.type = BuiltinTextureType::Checkerboard;
        info.isTileable = true;
        info.id = createCheckerboardTexture(256, 256, 16);
        m_builtinTextures.push_back(info);
    }
    
    // 纯黑
    {
        TextureInfo info;
        info.name = "Black";
        info.description = "8x8 solid black";
        info.width = 8;
        info.height = 8;
        info.channels = 4;
        info.type = BuiltinTextureType::Black;
        info.isTileable = true;
        info.id = createSolidColorTexture(8, 8, 0.0f, 0.0f, 0.0f, 1.0f);
        m_builtinTextures.push_back(info);
    }
    
    // 纯白
    {
        TextureInfo info;
        info.name = "White";
        info.description = "8x8 solid white";
        info.width = 8;
        info.height = 8;
        info.channels = 4;
        info.type = BuiltinTextureType::White;
        info.isTileable = true;
        info.id = createSolidColorTexture(8, 8, 1.0f, 1.0f, 1.0f, 1.0f);
        m_builtinTextures.push_back(info);
    }
    
    // UV渐变
    {
        TextureInfo info;
        info.name = "UV Gradient";
        info.description = "256x256 UV coordinate gradient";
        info.width = 256;
        info.height = 256;
        info.channels = 4;
        info.type = BuiltinTextureType::UV;
        info.isTileable = false;
        info.id = createUVGradientTexture(256, 256);
        m_builtinTextures.push_back(info);
    }
    
    std::cout << "TextureManager initialized with " << m_builtinTextures.size() << " builtin textures." << std::endl;
    m_initialized = true;
    return true;
}

void TextureManager::cleanup() {
    for (auto& tex : m_builtinTextures) {
        if (tex.id) {
            glDeleteTextures(1, &tex.id);
        }
    }
    m_builtinTextures.clear();
    
    for (auto& pair : m_userTextures) {
        if (pair.first) {
            glDeleteTextures(1, &pair.first);
        }
    }
    m_userTextures.clear();
    
    m_initialized = false;
}

GLuint TextureManager::getBuiltinTexture(BuiltinTextureType type) const {
    for (const auto& tex : m_builtinTextures) {
        if (tex.type == type) {
            return tex.id;
        }
    }
    return 0;
}

const TextureInfo* TextureManager::getTextureInfo(BuiltinTextureType type) const {
    for (const auto& tex : m_builtinTextures) {
        if (tex.type == type) {
            return &tex;
        }
    }
    return nullptr;
}

GLuint TextureManager::loadUserTexture(const std::string& path) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    
    if (!data) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        return 0;
    }
    
    GLuint textureId = uploadTexture(data, width, height, channels, true);
    stbi_image_free(data);
    
    if (textureId) {
        m_userTextures[textureId] = path;
    }
    
    return textureId;
}

void TextureManager::unloadUserTexture(GLuint id) {
    auto it = m_userTextures.find(id);
    if (it != m_userTextures.end()) {
        glDeleteTextures(1, &id);
        m_userTextures.erase(it);
    }
}

void TextureManager::bindTexture(GLuint textureId, int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, textureId);
}

void TextureManager::unbindTexture(int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, 0);
}

std::string TextureManager::getTextureName(BuiltinTextureType type) {
    switch (type) {
        case BuiltinTextureType::GrayNoise256: return "Gray Noise 256";
        case BuiltinTextureType::GrayNoiseMedium: return "Gray Noise 64";
        case BuiltinTextureType::GrayNoiseSmall: return "Gray Noise 32";
        case BuiltinTextureType::RGBANoise256: return "RGBA Noise 256";
        case BuiltinTextureType::RGBANoiseMedium: return "RGBA Noise 64";
        case BuiltinTextureType::RGBANoiseSmall: return "RGBA Noise 32";
        case BuiltinTextureType::PerlinNoise256: return "Perlin 256";
        case BuiltinTextureType::PerlinNoise512: return "Perlin 512";
        case BuiltinTextureType::OrganicNoise: return "Organic";
        case BuiltinTextureType::BlueNoise: return "Blue Noise";
        case BuiltinTextureType::Checkerboard: return "Checkerboard";
        case BuiltinTextureType::Black: return "Black";
        case BuiltinTextureType::White: return "White";
        case BuiltinTextureType::UV: return "UV Gradient";
        default: return "Unknown";
    }
}

// 创建灰度噪声纹理
GLuint TextureManager::createGrayNoiseTexture(int width, int height, unsigned int seed) {
    auto data = NoiseGenerator::generateWhiteNoise(width, height, 1, seed);
    return uploadTexture(data.data(), width, height, 1, true);
}

// 创建RGBA噪声纹理
GLuint TextureManager::createRGBANoiseTexture(int width, int height, unsigned int seed) {
    auto data = NoiseGenerator::generateWhiteNoise(width, height, 4, seed);
    return uploadTexture(data.data(), width, height, 4, true);
}

// 创建Perlin噪声纹理
GLuint TextureManager::createPerlinNoiseTexture(int width, int height, int octaves) {
    auto floatData = NoiseGenerator::generatePerlin2D(width, height, octaves, 0.5f, 4.0f);
    
    // 转换为uint8
    std::vector<uint8_t> data(floatData.size());
    for (size_t i = 0; i < floatData.size(); i++) {
        data[i] = static_cast<uint8_t>(std::min(floatData[i] * 255.0f, 255.0f));
    }
    
    return uploadTexture(data.data(), width, height, 1, true);
}

// 创建有机噪声纹理
GLuint TextureManager::createOrganicNoiseTexture(int width, int height) {
    auto data = NoiseGenerator::generateOrganic(width, height);
    return uploadTexture(data.data(), width, height, 4, true);
}

// 创建蓝噪声纹理
GLuint TextureManager::createBlueNoiseTexture(int width, int height) {
    auto data = NoiseGenerator::generateBlueNoise(width, height);
    return uploadTexture(data.data(), width, height, 1, true);
}

// 创建棋盘格纹理
GLuint TextureManager::createCheckerboardTexture(int width, int height, int cellSize) {
    std::vector<uint8_t> data(width * height);
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            bool isWhite = ((x / cellSize) + (y / cellSize)) % 2 == 0;
            data[y * width + x] = isWhite ? 255 : 0;
        }
    }
    
    return uploadTexture(data.data(), width, height, 1, true);
}

// 创建纯色纹理
GLuint TextureManager::createSolidColorTexture(int width, int height, float r, float g, float b, float a) {
    std::vector<uint8_t> data(width * height * 4);
    
    uint8_t R = static_cast<uint8_t>(r * 255.0f);
    uint8_t G = static_cast<uint8_t>(g * 255.0f);
    uint8_t B = static_cast<uint8_t>(b * 255.0f);
    uint8_t A = static_cast<uint8_t>(a * 255.0f);
    
    for (int i = 0; i < width * height; i++) {
        data[i * 4 + 0] = R;
        data[i * 4 + 1] = G;
        data[i * 4 + 2] = B;
        data[i * 4 + 3] = A;
    }
    
    return uploadTexture(data.data(), width, height, 4, true);
}

// 创建UV渐变纹理
GLuint TextureManager::createUVGradientTexture(int width, int height) {
    std::vector<uint8_t> data(width * height * 4);
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 4;
            data[idx + 0] = static_cast<uint8_t>((float)x / width * 255.0f);
            data[idx + 1] = static_cast<uint8_t>((float)y / height * 255.0f);
            data[idx + 2] = 0;
            data[idx + 3] = 255;
        }
    }
    
    return uploadTexture(data.data(), width, height, 4, false);
}

// 上传纹理到GPU
GLuint TextureManager::uploadTexture(const unsigned char* data, int width, int height, int channels, bool tileable) {
    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    
    // 设置纹理参数
    if (tileable) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // 确定格式
    GLenum format;
    GLenum internalFormat;
    switch (channels) {
        case 1:
            format = GL_RED;
            internalFormat = GL_R8;
            break;
        case 2:
            format = GL_RG;
            internalFormat = GL_RG8;
            break;
        case 3:
            format = GL_RGB;
            internalFormat = GL_RGB8;
            break;
        case 4:
        default:
            format = GL_RGBA;
            internalFormat = GL_RGBA8;
            break;
    }
    
    // 上传数据
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return textureId;
}

} // namespace shadertoy
