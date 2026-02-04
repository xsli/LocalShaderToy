/**
 * BufferManager - 管理 Multi-pass 渲染的 FBO
 * 
 * 支持 Buffer A/B/C/D，每个 Buffer 使用双缓冲 (ping-pong) 实现反馈循环
 */

#pragma once

#include "Framebuffer.h"
#include "../core/ScreensaverMode.h"
#include <array>
#include <memory>
#include <string>

namespace shadertoy {

/**
 * 单个 Buffer 的双缓冲管理
 */
struct BufferPass {
    std::unique_ptr<Framebuffer> front;  // 当前帧渲染目标
    std::unique_ptr<Framebuffer> back;   // 上一帧结果（可作为输入）
    bool enabled = false;
    
    BufferPass() = default;
    
    // 创建双缓冲
    bool create(int width, int height) {
        front = std::make_unique<Framebuffer>();
        back = std::make_unique<Framebuffer>();
        if (!front->create(width, height)) return false;
        if (!back->create(width, height)) return false;
        enabled = true;
        return true;
    }
    
    // 清理
    void cleanup() {
        if (front) front->cleanup();
        if (back) back->cleanup();
        front.reset();
        back.reset();
        enabled = false;
    }
    
    // 调整大小
    void resize(int width, int height) {
        if (front) front->resize(width, height);
        if (back) back->resize(width, height);
    }
    
    // 交换前后缓冲（每帧渲染后调用）
    void swap() {
        std::swap(front, back);
    }
    
    // 获取可读取的纹理（上一帧结果）
    GLuint getReadTexture() const {
        return back ? back->getTexture() : 0;
    }
    
    // 获取当前渲染目标纹理（用于调试）
    GLuint getFrontTexture() const {
        return front ? front->getTexture() : 0;
    }
    
    // 绑定为渲染目标
    void bindForRender() {
        if (front) front->bind();
    }
    
    // 解绑
    void unbind() {
        if (front) front->unbind();
    }
};

/**
 * Buffer 管理器 - 管理所有 Buffer Pass
 */
class BufferManager {
public:
    static constexpr int MAX_BUFFERS = 4;  // A, B, C, D
    
    BufferManager() = default;
    ~BufferManager() { cleanup(); }
    
    /**
     * 初始化指定的 Buffer
     * @param index Buffer 索引 (0=A, 1=B, 2=C, 3=D)
     * @param width 宽度
     * @param height 高度
     */
    bool initBuffer(int index, int width, int height);
    
    /**
     * 根据 ShaderPassType 初始化 Buffer
     */
    bool initBuffer(ShaderPassType type, int width, int height);
    
    /**
     * 批量初始化所有启用的 Buffer
     */
    void initFromPasses(const std::vector<PassConfig>& passes, int width, int height);
    
    /**
     * 禁用指定 Buffer
     */
    void disableBuffer(int index);
    void disableBuffer(ShaderPassType type);
    
    /**
     * 调整所有 Buffer 大小
     */
    void resize(int width, int height);
    
    /**
     * 清理所有资源
     */
    void cleanup();
    
    /**
     * 获取 Buffer
     */
    BufferPass* getBuffer(int index);
    BufferPass* getBuffer(ShaderPassType type);
    const BufferPass* getBuffer(int index) const;
    const BufferPass* getBuffer(ShaderPassType type) const;
    
    /**
     * 获取 Buffer 的可读纹理 ID
     */
    GLuint getReadTexture(int index) const;
    GLuint getReadTexture(ShaderPassType type) const;
    
    /**
     * 绑定 Buffer 为渲染目标
     */
    void bindBuffer(int index);
    void bindBuffer(ShaderPassType type);
    
    /**
     * 解绑回默认帧缓冲
     */
    void unbind();
    
    /**
     * 交换指定 Buffer 的前后缓冲
     */
    void swapBuffer(int index);
    void swapBuffer(ShaderPassType type);
    
    /**
     * 交换所有启用的 Buffer
     */
    void swapAll();
    
    /**
     * 清除所有 Buffer 内容（用于重置状态）
     */
    void clearAll();
    
    /**
     * 检查 Buffer 是否启用
     */
    bool isEnabled(int index) const;
    bool isEnabled(ShaderPassType type) const;
    
    /**
     * 获取渲染分辨率
     */
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    
    /**
     * 将 ShaderPassType 转换为 Buffer 索引
     * @return 索引 (0-3) 或 -1 如果不是 Buffer 类型
     */
    static int typeToIndex(ShaderPassType type);
    
    /**
     * 将索引转换为 ShaderPassType
     */
    static ShaderPassType indexToType(int index);

private:
    std::array<BufferPass, MAX_BUFFERS> m_buffers;
    int m_width = 0;
    int m_height = 0;
};

} // namespace shadertoy
