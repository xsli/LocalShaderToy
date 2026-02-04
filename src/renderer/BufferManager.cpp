/**
 * BufferManager 实现
 */

#include "BufferManager.h"
#include <iostream>

namespace shadertoy {

// ============================================================================
// 类型转换工具
// ============================================================================

int BufferManager::typeToIndex(ShaderPassType type) {
    switch (type) {
        case ShaderPassType::BufferA: return 0;
        case ShaderPassType::BufferB: return 1;
        case ShaderPassType::BufferC: return 2;
        case ShaderPassType::BufferD: return 3;
        default: return -1;
    }
}

ShaderPassType BufferManager::indexToType(int index) {
    switch (index) {
        case 0: return ShaderPassType::BufferA;
        case 1: return ShaderPassType::BufferB;
        case 2: return ShaderPassType::BufferC;
        case 3: return ShaderPassType::BufferD;
        default: return ShaderPassType::Image;
    }
}

// ============================================================================
// Buffer 初始化
// ============================================================================

bool BufferManager::initBuffer(int index, int width, int height) {
    if (index < 0 || index >= MAX_BUFFERS) {
        return false;
    }
    
    m_width = width;
    m_height = height;
    
    // 清理现有的
    m_buffers[static_cast<size_t>(index)].cleanup();
    
    // 创建新的
    if (!m_buffers[static_cast<size_t>(index)].create(width, height)) {
        std::cerr << "BufferManager: Failed to create Buffer " 
                  << static_cast<char>('A' + index) << std::endl;
        return false;
    }
    
    std::cout << "BufferManager: Created Buffer " << static_cast<char>('A' + index)
              << " (" << width << "x" << height << ")" << std::endl;
    
    return true;
}

bool BufferManager::initBuffer(ShaderPassType type, int width, int height) {
    int index = typeToIndex(type);
    if (index < 0) return false;
    return initBuffer(index, width, height);
}

void BufferManager::initFromPasses(const std::vector<PassConfig>& passes, int width, int height) {
    // 首先禁用所有 buffer
    for (int i = 0; i < MAX_BUFFERS; i++) {
        m_buffers[static_cast<size_t>(i)].cleanup();
    }
    
    // 根据 passes 启用需要的 buffer
    for (const auto& pass : passes) {
        if (pass.enabled) {
            int idx = typeToIndex(pass.type);
            if (idx >= 0) {
                initBuffer(idx, width, height);
            }
        }
    }
}

// ============================================================================
// Buffer 禁用
// ============================================================================

void BufferManager::disableBuffer(int index) {
    if (index >= 0 && index < MAX_BUFFERS) {
        m_buffers[static_cast<size_t>(index)].cleanup();
    }
}

void BufferManager::disableBuffer(ShaderPassType type) {
    int index = typeToIndex(type);
    if (index >= 0) disableBuffer(index);
}

// ============================================================================
// 调整大小 & 清理
// ============================================================================

void BufferManager::resize(int width, int height) {
    if (width == m_width && height == m_height) return;
    
    m_width = width;
    m_height = height;
    
    for (auto& buffer : m_buffers) {
        if (buffer.enabled) {
            buffer.resize(width, height);
        }
    }
}

void BufferManager::cleanup() {
    for (auto& buffer : m_buffers) {
        buffer.cleanup();
    }
    m_width = 0;
    m_height = 0;
}

// ============================================================================
// Buffer 访问
// ============================================================================

BufferPass* BufferManager::getBuffer(int index) {
    if (index >= 0 && index < MAX_BUFFERS) {
        return &m_buffers[static_cast<size_t>(index)];
    }
    return nullptr;
}

BufferPass* BufferManager::getBuffer(ShaderPassType type) {
    int index = typeToIndex(type);
    return getBuffer(index);
}

const BufferPass* BufferManager::getBuffer(int index) const {
    if (index >= 0 && index < MAX_BUFFERS) {
        return &m_buffers[static_cast<size_t>(index)];
    }
    return nullptr;
}

const BufferPass* BufferManager::getBuffer(ShaderPassType type) const {
    int index = typeToIndex(type);
    return getBuffer(index);
}

GLuint BufferManager::getReadTexture(int index) const {
    if (index >= 0 && index < MAX_BUFFERS && m_buffers[static_cast<size_t>(index)].enabled) {
        return m_buffers[static_cast<size_t>(index)].getReadTexture();
    }
    return 0;
}

GLuint BufferManager::getReadTexture(ShaderPassType type) const {
    int index = typeToIndex(type);
    return getReadTexture(index);
}

// ============================================================================
// 渲染目标绑定
// ============================================================================

void BufferManager::bindBuffer(int index) {
    if (index >= 0 && index < MAX_BUFFERS && m_buffers[static_cast<size_t>(index)].enabled) {
        m_buffers[static_cast<size_t>(index)].bindForRender();
    }
}

void BufferManager::bindBuffer(ShaderPassType type) {
    int index = typeToIndex(type);
    bindBuffer(index);
}

void BufferManager::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ============================================================================
// Ping-Pong 交换
// ============================================================================

void BufferManager::swapBuffer(int index) {
    if (index >= 0 && index < MAX_BUFFERS && m_buffers[static_cast<size_t>(index)].enabled) {
        m_buffers[static_cast<size_t>(index)].swap();
    }
}

void BufferManager::swapBuffer(ShaderPassType type) {
    int index = typeToIndex(type);
    swapBuffer(index);
}

void BufferManager::swapAll() {
    for (auto& buffer : m_buffers) {
        if (buffer.enabled) {
            buffer.swap();
        }
    }
}

void BufferManager::clearAll() {
    // 清除所有启用的 Buffer 内容（设置为黑色）
    for (auto& buffer : m_buffers) {
        if (buffer.enabled) {
            // 清除 front buffer
            if (buffer.front) {
                buffer.front->bind();
                glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                glClear(GL_COLOR_BUFFER_BIT);
            }
            // 清除 back buffer
            if (buffer.back) {
                buffer.back->bind();
                glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                glClear(GL_COLOR_BUFFER_BIT);
            }
        }
    }
    // 恢复默认帧缓冲
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ============================================================================
// 状态查询
// ============================================================================

bool BufferManager::isEnabled(int index) const {
    if (index >= 0 && index < MAX_BUFFERS) {
        return m_buffers[static_cast<size_t>(index)].enabled;
    }
    return false;
}

bool BufferManager::isEnabled(ShaderPassType type) const {
    int index = typeToIndex(type);
    return isEnabled(index);
}

} // namespace shadertoy
