#pragma once

#include <vector>
#include <cstdint>
#include <random>

namespace shadertoy {

// 噪声生成器 - 用于生成各种程序化噪声纹理
class NoiseGenerator {
public:
    // Perlin噪声
    static std::vector<float> generatePerlin2D(int width, int height, int octaves = 4, float persistence = 0.5f, float scale = 4.0f);
    
    // Simplex噪声
    static std::vector<float> generateSimplex2D(int width, int height, int octaves = 4, float persistence = 0.5f, float scale = 4.0f);
    
    // Worley噪声 (细胞噪声)
    static std::vector<float> generateWorley2D(int width, int height, int numPoints = 32);
    
    // 白噪声
    static std::vector<uint8_t> generateWhiteNoise(int width, int height, int channels, unsigned int seed = 0);
    
    // 蓝噪声 (抖动图案)
    static std::vector<uint8_t> generateBlueNoise(int width, int height);
    
    // 有机噪声 (类似Shadertoy的Organic纹理 - 基于多层叠加)
    static std::vector<uint8_t> generateOrganic(int width, int height);
    
    // FBM (分形布朗运动)
    static std::vector<float> generateFBM2D(int width, int height, int octaves = 6, float lacunarity = 2.0f, float gain = 0.5f);
    
private:
    // Perlin噪声核心函数
    static float perlinNoise2D(float x, float y, const std::vector<int>& perm);
    static float fade(float t);
    static float lerp(float a, float b, float t);
    static float grad(int hash, float x, float y);
    
    // Simplex噪声核心函数
    static float simplexNoise2D(float x, float y, const std::vector<int>& perm);
    
    // 生成随机排列表
    static std::vector<int> generatePermutationTable(unsigned int seed = 0);
};

} // namespace shadertoy
