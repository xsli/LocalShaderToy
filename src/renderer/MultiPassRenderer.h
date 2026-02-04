/**
 * MultiPassRenderer - 多 Pass 渲染管理器
 * 
 * 管理多个 Shader Pass 的编译和渲染
 * 支持 Buffer A/B/C/D -> Image 的渲染管线
 */

#pragma once

#include "BufferManager.h"
#include "Renderer.h"
#include "../core/ShaderEngine.h"
#include "../core/UniformManager.h"
#include "../core/ScreensaverMode.h"
#include "../transpiler/GLSLTranspiler.h"
#include <array>
#include <map>
#include <memory>
#include <string>
#include <functional>

namespace shadertoy {

/**
 * 单个 Pass 的渲染状态
 */
struct PassRenderState {
    ShaderPassType type = ShaderPassType::Image;
    std::unique_ptr<ShaderEngine> shader;
    std::array<int, 4> channels = {-1, -1, -1, -1};
    bool enabled = false;
    bool compiled = false;
    std::string lastError;
    
    PassRenderState() = default;
    PassRenderState(ShaderPassType t) : type(t) {
        shader = std::make_unique<ShaderEngine>();
    }
};

/**
 * 多 Pass 渲染器
 */
class MultiPassRenderer {
public:
    MultiPassRenderer() = default;
    ~MultiPassRenderer() = default;
    
    /**
     * 初始化渲染器
     * @param width 渲染宽度
     * @param height 渲染高度
     */
    void init(int width, int height);
    
    /**
     * 调整渲染分辨率
     */
    void resize(int width, int height);
    
    /**
     * 清理所有资源
     */
    void cleanup();
    
    /**
     * 设置 Common 代码
     */
    void setCommonCode(const std::string& code);
    
    /**
     * 编译指定 Pass
     * @param type Pass 类型
     * @param code Shader 代码
     * @param channels Channel 绑定
     * @return 是否成功
     */
    bool compilePass(ShaderPassType type, const std::string& code, 
                     const std::array<int, 4>& channels);
    
    /**
     * 禁用指定 Pass
     */
    void disablePass(ShaderPassType type);
    
    /**
     * 检查 Pass 是否启用
     */
    bool isPassEnabled(ShaderPassType type) const;
    
    /**
     * 获取 Pass 的编译错误
     */
    std::string getPassError(ShaderPassType type) const;
    
    /**
     * 获取所有错误（合并）
     */
    std::string getAllErrors() const;
    
    /**
     * 渲染所有 Pass
     * 顺序: Buffer A -> B -> C -> D -> Image
     * 
     * @param uniforms Uniform 设置回调 (program, passType)
     * @param bindTextures 纹理绑定回调 (program, channel, channelBinding)
     * @param renderQuad 渲染四边形回调
     */
    void render(
        std::function<void(GLuint, ShaderPassType)> uniforms,
        std::function<void(GLuint, int, int)> bindTextures,
        std::function<void()> renderQuad
    );
    
    /**
     * 获取 Buffer 管理器（用于绑定纹理）
     */
    BufferManager& getBufferManager() { return m_bufferManager; }
    const BufferManager& getBufferManager() const { return m_bufferManager; }
    
    /**
     * 获取 Buffer 的当前纹理 ID（用于调试显示）
     * @param type Buffer 类型 (BufferA/B/C/D)
     * @return 纹理 ID，如果不存在返回 0
     */
    GLuint getBufferTexture(ShaderPassType type) const;
    
    /**
     * 获取当前宽高
     */
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    
    /**
     * 检查主 Pass (Image) 是否有效
     */
    bool hasValidMainPass() const;
    
    /**
     * 编译主 Pass (Image) - 简化接口
     * @param code Shader 代码（不含 Common）
     * @param error 错误信息输出
     * @return 是否成功
     */
    bool compileMainPass(const std::string& code, std::string& error);
    
    /**
     * 编译 Buffer Pass - 简化接口
     * @param bufferIndex Buffer 索引 (0-3 对应 A-D)
     * @param code Shader 代码（不含 Common）
     * @param error 错误信息输出
     * @return 是否成功
     */
    bool compileBufferPass(int bufferIndex, const std::string& code, std::string& error);
    
    /**
     * 设置 Image Pass 的 Channel 绑定
     */
    void setChannelBinding(int channel, int binding);
    
    /**
     * 设置 Buffer Pass 的 Channel 绑定
     */
    void setBufferChannelBinding(int bufferIndex, int channel, int binding);
    
    /**
     * 简化版渲染（使用 UniformManager 和 Renderer）
     */
    void render(UniformManager& uniformManager, Renderer& renderer);
    
    /**
     * 设置 Debug Buffer 模式
     * @param bufferIndex -1=关闭, 0=Buffer A, 1=B, 2=C, 3=D
     */
    void setDebugBuffer(int bufferIndex);
    
    /**
     * 获取当前 Debug Buffer 索引
     */
    int getDebugBuffer() const { return m_debugBufferIndex; }
    
    /**
     * 编译 Debug Buffer 预制 Shader
     */
    bool compileDebugShader();

private:
    // 创建或获取 Pass 状态
    PassRenderState& getOrCreatePass(ShaderPassType type);
    
    // 渲染单个 Pass
    void renderPass(PassRenderState& pass,
                    std::function<void(GLuint, ShaderPassType)>& uniforms,
                    std::function<void(GLuint, int, int)>& bindTextures,
                    std::function<void()>& renderQuad);
    
    // 绑定 Buffer 纹理到 iChannel
    void bindBufferTexture(GLuint program, int channel, int binding);
    
    // 渲染 Debug Buffer（使用预制 shader 采样指定 Buffer）
    void renderDebugBuffer(
        std::function<void(GLuint, ShaderPassType)>& uniforms,
        std::function<void()>& renderQuad);

private:
    BufferManager m_bufferManager;
    GLSLTranspiler m_transpiler;
    
    // 各 Pass 的渲染状态
    std::map<ShaderPassType, PassRenderState> m_passes;
    
    // Common 代码
    std::string m_commonCode;
    
    // 渲染分辨率
    int m_width = 0;
    int m_height = 0;
    
    // Debug Buffer 模式
    int m_debugBufferIndex = -1;  // -1=关闭, 0-3=Buffer A-D
    std::unique_ptr<ShaderEngine> m_debugShader;  // 预制的采样 shader
    bool m_debugShaderCompiled = false;
    
    // 渲染顺序
    static constexpr ShaderPassType RENDER_ORDER[] = {
        ShaderPassType::BufferA,
        ShaderPassType::BufferB,
        ShaderPassType::BufferC,
        ShaderPassType::BufferD,
        ShaderPassType::Image
    };
};

} // namespace shadertoy
