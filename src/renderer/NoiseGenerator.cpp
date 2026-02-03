#include "NoiseGenerator.h"
#include <cmath>
#include <algorithm>
#include <numeric>

namespace shadertoy {

// 生成随机排列表
std::vector<int> NoiseGenerator::generatePermutationTable(unsigned int seed) {
    std::vector<int> perm(512);
    std::vector<int> p(256);
    
    // 初始化0-255
    std::iota(p.begin(), p.end(), 0);
    
    // 随机打乱
    std::mt19937 rng(seed);
    std::shuffle(p.begin(), p.end(), rng);
    
    // 复制两份
    for (int i = 0; i < 256; i++) {
        perm[i] = perm[i + 256] = p[i];
    }
    
    return perm;
}

// 平滑插值函数
float NoiseGenerator::fade(float t) {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

// 线性插值
float NoiseGenerator::lerp(float a, float b, float t) {
    return a + t * (b - a);
}

// 梯度函数
float NoiseGenerator::grad(int hash, float x, float y) {
    int h = hash & 7;
    float u = h < 4 ? x : y;
    float v = h < 4 ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
}

// Perlin噪声2D核心
float NoiseGenerator::perlinNoise2D(float x, float y, const std::vector<int>& perm) {
    // 找到单元格坐标
    int X = (int)std::floor(x) & 255;
    int Y = (int)std::floor(y) & 255;
    
    // 找到单元格内的相对坐标
    x -= std::floor(x);
    y -= std::floor(y);
    
    // 计算淡入曲线
    float u = fade(x);
    float v = fade(y);
    
    // 哈希坐标的四个角
    int A = perm[X] + Y;
    int AA = perm[A];
    int AB = perm[A + 1];
    int B = perm[X + 1] + Y;
    int BA = perm[B];
    int BB = perm[B + 1];
    
    // 混合四个角的梯度贡献
    float res = lerp(
        lerp(grad(perm[AA], x, y), grad(perm[BA], x - 1, y), u),
        lerp(grad(perm[AB], x, y - 1), grad(perm[BB], x - 1, y - 1), u),
        v
    );
    
    return (res + 1.0f) / 2.0f; // 归一化到[0,1]
}

// 生成Perlin噪声纹理
std::vector<float> NoiseGenerator::generatePerlin2D(int width, int height, int octaves, float persistence, float scale) {
    std::vector<float> result(width * height);
    auto perm = generatePermutationTable(42);
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float nx = (float)x / width * scale;
            float ny = (float)y / height * scale;
            
            float amplitude = 1.0f;
            float frequency = 1.0f;
            float noiseValue = 0.0f;
            float maxValue = 0.0f;
            
            for (int o = 0; o < octaves; o++) {
                noiseValue += perlinNoise2D(nx * frequency, ny * frequency, perm) * amplitude;
                maxValue += amplitude;
                amplitude *= persistence;
                frequency *= 2.0f;
            }
            
            result[y * width + x] = noiseValue / maxValue;
        }
    }
    
    return result;
}

// Simplex噪声核心 (简化版)
float NoiseGenerator::simplexNoise2D(float x, float y, const std::vector<int>& perm) {
    const float F2 = 0.5f * (std::sqrt(3.0f) - 1.0f);
    const float G2 = (3.0f - std::sqrt(3.0f)) / 6.0f;
    
    // 倾斜输入空间
    float s = (x + y) * F2;
    int i = (int)std::floor(x + s);
    int j = (int)std::floor(y + s);
    
    float t = (i + j) * G2;
    float X0 = i - t;
    float Y0 = j - t;
    float x0 = x - X0;
    float y0 = y - Y0;
    
    // 确定在哪个simplex中
    int i1, j1;
    if (x0 > y0) {
        i1 = 1; j1 = 0;
    } else {
        i1 = 0; j1 = 1;
    }
    
    float x1 = x0 - i1 + G2;
    float y1 = y0 - j1 + G2;
    float x2 = x0 - 1.0f + 2.0f * G2;
    float y2 = y0 - 1.0f + 2.0f * G2;
    
    int ii = i & 255;
    int jj = j & 255;
    
    // 计算每个角的贡献
    float n0 = 0.0f, n1 = 0.0f, n2 = 0.0f;
    
    float t0 = 0.5f - x0 * x0 - y0 * y0;
    if (t0 >= 0) {
        t0 *= t0;
        n0 = t0 * t0 * grad(perm[ii + perm[jj]], x0, y0);
    }
    
    float t1 = 0.5f - x1 * x1 - y1 * y1;
    if (t1 >= 0) {
        t1 *= t1;
        n1 = t1 * t1 * grad(perm[ii + i1 + perm[jj + j1]], x1, y1);
    }
    
    float t2 = 0.5f - x2 * x2 - y2 * y2;
    if (t2 >= 0) {
        t2 *= t2;
        n2 = t2 * t2 * grad(perm[ii + 1 + perm[jj + 1]], x2, y2);
    }
    
    return (70.0f * (n0 + n1 + n2) + 1.0f) / 2.0f;
}

// 生成Simplex噪声纹理
std::vector<float> NoiseGenerator::generateSimplex2D(int width, int height, int octaves, float persistence, float scale) {
    std::vector<float> result(width * height);
    auto perm = generatePermutationTable(42);
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float nx = (float)x / width * scale;
            float ny = (float)y / height * scale;
            
            float amplitude = 1.0f;
            float frequency = 1.0f;
            float noiseValue = 0.0f;
            float maxValue = 0.0f;
            
            for (int o = 0; o < octaves; o++) {
                noiseValue += simplexNoise2D(nx * frequency, ny * frequency, perm) * amplitude;
                maxValue += amplitude;
                amplitude *= persistence;
                frequency *= 2.0f;
            }
            
            result[y * width + x] = noiseValue / maxValue;
        }
    }
    
    return result;
}

// 生成Worley噪声 (细胞噪声)
std::vector<float> NoiseGenerator::generateWorley2D(int width, int height, int numPoints) {
    std::vector<float> result(width * height);
    std::vector<std::pair<float, float>> points(numPoints);
    
    // 生成随机点
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    for (int i = 0; i < numPoints; i++) {
        points[i] = {dist(rng) * width, dist(rng) * height};
    }
    
    // 计算每个像素到最近点的距离
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float minDist = std::numeric_limits<float>::max();
            
            for (const auto& p : points) {
                // 考虑平铺边界
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        float px = p.first + dx * width;
                        float py = p.second + dy * height;
                        float dist = std::sqrt((x - px) * (x - px) + (y - py) * (y - py));
                        minDist = std::min(minDist, dist);
                    }
                }
            }
            
            // 归一化距离
            float maxPossibleDist = std::sqrt((float)(width * width + height * height)) / std::sqrt((float)numPoints);
            result[y * width + x] = std::min(minDist / maxPossibleDist, 1.0f);
        }
    }
    
    return result;
}

// 生成白噪声
std::vector<uint8_t> NoiseGenerator::generateWhiteNoise(int width, int height, int channels, unsigned int seed) {
    std::vector<uint8_t> result(width * height * channels);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(0, 255);
    
    for (size_t i = 0; i < result.size(); i++) {
        result[i] = static_cast<uint8_t>(dist(rng));
    }
    
    return result;
}

// 生成蓝噪声 (简化版 - 使用Mitchell的最佳候选算法)
std::vector<uint8_t> NoiseGenerator::generateBlueNoise(int width, int height) {
    std::vector<uint8_t> result(width * height);
    std::vector<std::pair<int, int>> samples;
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> distX(0, width - 1);
    std::uniform_int_distribution<int> distY(0, height - 1);
    
    int numSamples = width * height / 4;
    
    // 生成蓝噪声采样点
    for (int i = 0; i < numSamples; i++) {
        float bestDist = -1;
        int bestX = 0, bestY = 0;
        
        // 生成多个候选点，选择离现有点最远的
        int numCandidates = std::min(10, (int)samples.size() + 1);
        for (int c = 0; c < numCandidates; c++) {
            int cx = distX(rng);
            int cy = distY(rng);
            
            float minDist = std::numeric_limits<float>::max();
            for (const auto& s : samples) {
                float dx = (float)(cx - s.first);
                float dy = (float)(cy - s.second);
                // 考虑平铺
                dx = std::min(std::abs(dx), (float)width - std::abs(dx));
                dy = std::min(std::abs(dy), (float)height - std::abs(dy));
                float dist = dx * dx + dy * dy;
                minDist = std::min(minDist, dist);
            }
            
            if (samples.empty() || minDist > bestDist) {
                bestDist = minDist;
                bestX = cx;
                bestY = cy;
            }
        }
        
        samples.push_back({bestX, bestY});
    }
    
    // 将采样点转换为纹理
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float minDist = std::numeric_limits<float>::max();
            for (const auto& s : samples) {
                float dx = std::abs((float)(x - s.first));
                float dy = std::abs((float)(y - s.second));
                dx = std::min(dx, (float)width - dx);
                dy = std::min(dy, (float)height - dy);
                float dist = std::sqrt(dx * dx + dy * dy);
                minDist = std::min(minDist, dist);
            }
            
            float maxDist = std::sqrt((float)(width * height) / numSamples) * 0.8f;
            result[y * width + x] = static_cast<uint8_t>(std::min(minDist / maxDist * 255.0f, 255.0f));
        }
    }
    
    return result;
}

// 生成有机噪声 (多层Perlin叠加 + 颜色变换)
std::vector<uint8_t> NoiseGenerator::generateOrganic(int width, int height) {
    std::vector<uint8_t> result(width * height * 4);
    
    // 生成多层噪声
    auto noise1 = generatePerlin2D(width, height, 4, 0.5f, 4.0f);
    auto noise2 = generatePerlin2D(width, height, 6, 0.4f, 8.0f);
    auto noise3 = generateWorley2D(width, height, 16);
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * width + x;
            
            float n1 = noise1[idx];
            float n2 = noise2[idx];
            float n3 = noise3[idx];
            
            // 混合噪声创建有机外观
            float r = n1 * 0.7f + n2 * 0.2f + n3 * 0.1f;
            float g = n1 * 0.5f + n2 * 0.4f + n3 * 0.1f;
            float b = n1 * 0.3f + n2 * 0.3f + n3 * 0.4f;
            float a = 1.0f;
            
            // 添加一些颜色偏移
            r = std::pow(r, 0.8f);
            g = std::pow(g, 1.0f);
            b = std::pow(b, 1.2f);
            
            int outIdx = idx * 4;
            result[outIdx + 0] = static_cast<uint8_t>(std::min(r * 255.0f, 255.0f));
            result[outIdx + 1] = static_cast<uint8_t>(std::min(g * 255.0f, 255.0f));
            result[outIdx + 2] = static_cast<uint8_t>(std::min(b * 255.0f, 255.0f));
            result[outIdx + 3] = static_cast<uint8_t>(a * 255.0f);
        }
    }
    
    return result;
}

// 生成FBM噪声
std::vector<float> NoiseGenerator::generateFBM2D(int width, int height, int octaves, float lacunarity, float gain) {
    std::vector<float> result(width * height);
    auto perm = generatePermutationTable(42);
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float nx = (float)x / width * 4.0f;
            float ny = (float)y / height * 4.0f;
            
            float amplitude = 1.0f;
            float frequency = 1.0f;
            float noiseValue = 0.0f;
            float maxValue = 0.0f;
            
            for (int o = 0; o < octaves; o++) {
                noiseValue += perlinNoise2D(nx * frequency, ny * frequency, perm) * amplitude;
                maxValue += amplitude;
                amplitude *= gain;
                frequency *= lacunarity;
            }
            
            result[y * width + x] = noiseValue / maxValue;
        }
    }
    
    return result;
}

} // namespace shadertoy
