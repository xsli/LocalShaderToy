/**
 * MultiPassRenderer 实现
 */

#include "MultiPassRenderer.h"
#include <iostream>
#include <sstream>

namespace shadertoy {

// ============================================================================
// 初始化和清理
// ============================================================================

void MultiPassRenderer::init(int width, int height) {
    m_width = width;
    m_height = height;
    
    // 预创建 Image pass（始终存在）
    getOrCreatePass(ShaderPassType::Image);
    
    std::cout << "MultiPassRenderer: Initialized (" << width << "x" << height << ")" << std::endl;
}

void MultiPassRenderer::resize(int width, int height) {
    if (width == m_width && height == m_height) return;
    
    m_width = width;
    m_height = height;
    
    // 调整所有 Buffer 大小
    m_bufferManager.resize(width, height);
    
    std::cout << "MultiPassRenderer: Resized to " << width << "x" << height << std::endl;
}

void MultiPassRenderer::cleanup() {
    m_bufferManager.cleanup();
    m_passes.clear();
    m_commonCode.clear();
}

// ============================================================================
// Pass 管理
// ============================================================================

PassRenderState& MultiPassRenderer::getOrCreatePass(ShaderPassType type) {
    auto it = m_passes.find(type);
    if (it == m_passes.end()) {
        m_passes.emplace(type, PassRenderState(type));
    }
    return m_passes[type];
}

void MultiPassRenderer::setCommonCode(const std::string& code) {
    m_commonCode = code;
}

bool MultiPassRenderer::compilePass(ShaderPassType type, const std::string& code,
                                     const std::array<int, 4>& channels) {
    // Common pass 不编译，只存储代码
    if (type == ShaderPassType::Common) {
        setCommonCode(code);
        return true;
    }
    
    PassRenderState& pass = getOrCreatePass(type);
    pass.channels = channels;
    
    // 如果代码为空，禁用该 pass
    if (code.empty()) {
        pass.enabled = false;
        pass.compiled = false;
        
        // 禁用对应的 Buffer
        int bufIdx = BufferManager::typeToIndex(type);
        if (bufIdx >= 0) {
            m_bufferManager.disableBuffer(bufIdx);
        }
        return true;
    }
    
    // 组合 Common 代码和 Pass 代码
    std::string fullCode;
    if (!m_commonCode.empty()) {
        fullCode = m_commonCode + "\n\n// ========== Pass Code ==========\n\n" + code;
    } else {
        fullCode = code;
    }
    
    // 转译代码
    std::string transpiledCode = m_transpiler.transpile(fullCode);
    
    // 确保 shader engine 存在
    if (!pass.shader) {
        pass.shader = std::make_unique<ShaderEngine>();
    }
    
    // 编译
    std::string error;
    bool success = pass.shader->compileShader(transpiledCode, error);
    
    if (success) {
        pass.enabled = true;
        pass.compiled = true;
        pass.lastError.clear();
        
        // 如果是 Buffer 类型，确保 FBO 已创建
        int bufIdx = BufferManager::typeToIndex(type);
        if (bufIdx >= 0 && m_width > 0 && m_height > 0) {
            if (!m_bufferManager.isEnabled(bufIdx)) {
                m_bufferManager.initBuffer(bufIdx, m_width, m_height);
            }
        }
        
        std::cout << "MultiPassRenderer: Compiled " 
                  << PassConfig::getTypeName(type) << " successfully" << std::endl;
    } else {
        pass.enabled = false;
        pass.compiled = false;
        pass.lastError = "[" + std::string(PassConfig::getTypeName(type)) + "] " + error;
        
        std::cerr << "MultiPassRenderer: Failed to compile " 
                  << PassConfig::getTypeName(type) << ":\n" << error << std::endl;
    }
    
    return success;
}

void MultiPassRenderer::disablePass(ShaderPassType type) {
    auto it = m_passes.find(type);
    if (it != m_passes.end()) {
        it->second.enabled = false;
        it->second.compiled = false;
    }
    
    // 禁用对应的 Buffer
    int bufIdx = BufferManager::typeToIndex(type);
    if (bufIdx >= 0) {
        m_bufferManager.disableBuffer(bufIdx);
    }
}

bool MultiPassRenderer::isPassEnabled(ShaderPassType type) const {
    auto it = m_passes.find(type);
    return it != m_passes.end() && it->second.enabled && it->second.compiled;
}

std::string MultiPassRenderer::getPassError(ShaderPassType type) const {
    auto it = m_passes.find(type);
    if (it != m_passes.end()) {
        return it->second.lastError;
    }
    return "";
}

std::string MultiPassRenderer::getAllErrors() const {
    std::stringstream ss;
    bool first = true;
    
    for (const auto& [type, pass] : m_passes) {
        if (!pass.lastError.empty()) {
            if (!first) ss << "\n\n";
            ss << pass.lastError;
            first = false;
        }
    }
    
    return ss.str();
}

// ============================================================================
// 渲染
// ============================================================================

void MultiPassRenderer::render(
    std::function<void(GLuint, ShaderPassType)> uniforms,
    std::function<void(GLuint, int, int)> bindTextures,
    std::function<void()> renderQuad)
{
    static int frameCount = 0;
    frameCount++;
    bool shouldLog = (frameCount % 300 == 1); // 每 5 秒日志一次（假设 60fps）
    
    // 按顺序渲染：Buffer A -> B -> C -> D -> Image
    for (ShaderPassType type : RENDER_ORDER) {
        // 如果是 Debug Buffer 模式且这是 Image pass，使用 Debug Shader
        if (type == ShaderPassType::Image && m_debugBufferIndex >= 0) {
            renderDebugBuffer(uniforms, renderQuad);
            continue;
        }
        
        auto it = m_passes.find(type);
        if (it == m_passes.end()) {
            if (shouldLog && type == ShaderPassType::Image) {
                std::cout << "[Render] Image pass not found in m_passes!" << std::endl;
            }
            continue;
        }
        if (!it->second.enabled) {
            if (shouldLog && type == ShaderPassType::Image) {
                std::cout << "[Render] Image pass found but NOT enabled!" << std::endl;
            }
            continue;
        }
        if (!it->second.compiled) {
            if (shouldLog && type == ShaderPassType::Image) {
                std::cout << "[Render] Image pass found but NOT compiled!" << std::endl;
            }
            continue;
        }
        
        if (shouldLog) {
            std::cout << "[Render] Rendering pass: " << PassConfig::getTypeName(type) << std::endl;
        }
        
        renderPass(it->second, uniforms, bindTextures, renderQuad);
    }
    
    // 在所有 Pass 渲染完成后，交换所有 Buffer
    m_bufferManager.swapAll();
}

void MultiPassRenderer::renderPass(
    PassRenderState& pass,
    std::function<void(GLuint, ShaderPassType)>& uniforms,
    std::function<void(GLuint, int, int)>& bindTextures,
    std::function<void()>& renderQuad)
{
    if (!pass.shader || !pass.shader->isValid()) {
        return;
    }
    
    // 如果是 Buffer 类型，绑定到 FBO
    int bufIdx = BufferManager::typeToIndex(pass.type);
    if (bufIdx >= 0) {
        m_bufferManager.bindBuffer(bufIdx);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    
    // 使用 shader
    pass.shader->use();
    GLuint program = pass.shader->getProgram();
    
    // 绑定纹理
    for (int ch = 0; ch < 4; ch++) {
        int binding = pass.channels[static_cast<size_t>(ch)];
        
        // 检查是否是 Buffer 绑定
        if (binding >= ChannelBind::BufferA && binding <= ChannelBind::BufferD) {
            // 绑定 Buffer 纹理
            bindBufferTexture(program, ch, binding);
        } else {
            // 使用外部回调绑定纹理
            bindTextures(program, ch, binding);
        }
    }
    
    // 设置 uniforms
    uniforms(program, pass.type);
    
    // 渲染
    renderQuad();
    
    // 解绑 FBO
    if (bufIdx >= 0) {
        m_bufferManager.unbind();
    }
}

void MultiPassRenderer::bindBufferTexture(GLuint program, int channel, int binding) {
    int bufIdx = binding - ChannelBind::BufferA;
    
    glActiveTexture(GL_TEXTURE0 + channel);
    
    GLuint texId = m_bufferManager.getReadTexture(bufIdx);
    if (texId != 0) {
        glBindTexture(GL_TEXTURE_2D, texId);
    } else {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    
    // 设置 iChannel uniform
    std::string channelName = "iChannel" + std::to_string(channel);
    GLint channelLoc = glGetUniformLocation(program, channelName.c_str());
    if (channelLoc >= 0) {
        glUniform1i(channelLoc, channel);
    }
    
    // 设置 iChannelResolution（使用 Buffer 分辨率）
    std::string resName = "iChannelResolution[" + std::to_string(channel) + "]";
    GLint resLoc = glGetUniformLocation(program, resName.c_str());
    if (resLoc >= 0) {
        glUniform3f(resLoc, 
            static_cast<float>(m_width),
            static_cast<float>(m_height),
            1.0f);
    }
}

GLuint MultiPassRenderer::getBufferTexture(ShaderPassType type) const {
    int bufIdx = BufferManager::typeToIndex(type);
    if (bufIdx < 0) {
        return 0;  // 不是 Buffer 类型
    }
    
    if (!m_bufferManager.isEnabled(bufIdx)) {
        return 0;  // Buffer 未启用
    }
    
    // 返回 front 纹理
    // 在 swapAll() 之后：front = 交换前的 back = Image pass 实际读取的纹理
    // 这确保 Debug 面板显示的和 Image pass 看到的一致
    const BufferPass* buffer = m_bufferManager.getBuffer(bufIdx);
    return buffer ? buffer->getFrontTexture() : 0;
}

// ============================================================================
// Debug Buffer 功能
// ============================================================================

void MultiPassRenderer::setDebugBuffer(int bufferIndex) {
    m_debugBufferIndex = bufferIndex;
    
    // 首次使用时编译 Debug Shader
    if (bufferIndex >= 0 && !m_debugShaderCompiled) {
        compileDebugShader();
    }
}

bool MultiPassRenderer::compileDebugShader() {
    // 预制的 Buffer 采样 Shader
    // 带有简单的 Reinhard Tonemapping，适合查看 HDR Buffer
    static const char* debugShaderCode = R"(
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = fragCoord / iResolution.xy;
    vec4 hdr = texture(iChannel0, uv);
    
    // Reinhard Tonemapping: color / (1 + color)
    vec3 mapped = hdr.rgb / (1.0 + hdr.rgb);
    
    // Gamma 校正
    mapped = pow(mapped, vec3(1.0 / 2.2));
    
    fragColor = vec4(mapped, hdr.a);
}
)";

    if (!m_debugShader) {
        m_debugShader = std::make_unique<ShaderEngine>();
    }
    
    // 转译代码
    std::string transpiledCode = m_transpiler.transpile(debugShaderCode);
    
    // 编译
    std::string error;
    bool success = m_debugShader->compileShader(transpiledCode, error);
    
    if (success) {
        m_debugShaderCompiled = true;
        std::cout << "MultiPassRenderer: Debug shader compiled successfully" << std::endl;
    } else {
        std::cerr << "MultiPassRenderer: Failed to compile debug shader: " << error << std::endl;
    }
    
    return success;
}

void MultiPassRenderer::renderDebugBuffer(
    std::function<void(GLuint, ShaderPassType)>& uniforms,
    std::function<void()>& renderQuad)
{
    // 检查 Debug Shader 是否可用
    if (!m_debugShaderCompiled || !m_debugShader || !m_debugShader->isValid()) {
        // 尝试重新编译
        if (!compileDebugShader()) {
            return;
        }
    }
    
    // 检查目标 Buffer 是否有效
    if (m_debugBufferIndex < 0 || m_debugBufferIndex > 3) {
        return;
    }
    
    if (!m_bufferManager.isEnabled(m_debugBufferIndex)) {
        return;
    }
    
    // 使用 Debug Shader
    m_debugShader->use();
    GLuint program = m_debugShader->getProgram();
    
    // 绑定目标 Buffer 到 iChannel0
    glActiveTexture(GL_TEXTURE0);
    GLuint texId = m_bufferManager.getReadTexture(m_debugBufferIndex);
    glBindTexture(GL_TEXTURE_2D, texId);
    
    // 设置 iChannel0 uniform
    GLint channelLoc = glGetUniformLocation(program, "iChannel0");
    if (channelLoc >= 0) {
        glUniform1i(channelLoc, 0);
    }
    
    // 设置 iChannelResolution[0]
    GLint resLoc = glGetUniformLocation(program, "iChannelResolution[0]");
    if (resLoc >= 0) {
        glUniform3f(resLoc,
            static_cast<float>(m_width),
            static_cast<float>(m_height),
            1.0f);
    }
    
    // 设置基本 uniforms（iResolution 等）
    uniforms(program, ShaderPassType::Image);
    
    // 渲染
    renderQuad();
}

// 静态成员定义
constexpr ShaderPassType MultiPassRenderer::RENDER_ORDER[];

// ============================================================================
// 简化接口方法（用于屏保模式等）
// ============================================================================

bool MultiPassRenderer::hasValidMainPass() const {
    auto it = m_passes.find(ShaderPassType::Image);
    if (it == m_passes.end()) return false;
    return it->second.enabled && it->second.compiled && 
           it->second.shader && it->second.shader->isValid();
}

bool MultiPassRenderer::compileMainPass(const std::string& code, std::string& error) {
    std::array<int, 4> channels = {-1, -1, -1, -1};
    
    // 保留现有的 channel 绑定
    auto it = m_passes.find(ShaderPassType::Image);
    if (it != m_passes.end()) {
        channels = it->second.channels;
    }
    
    bool success = compilePass(ShaderPassType::Image, code, channels);
    if (!success) {
        error = getPassError(ShaderPassType::Image);
    }
    return success;
}

bool MultiPassRenderer::compileBufferPass(int bufferIndex, const std::string& code, std::string& error) {
    if (bufferIndex < 0 || bufferIndex > 3) {
        error = "Invalid buffer index";
        return false;
    }
    
    ShaderPassType type = BufferManager::indexToType(bufferIndex);
    std::array<int, 4> channels = {-1, -1, -1, -1};
    
    // 保留现有的 channel 绑定
    auto it = m_passes.find(type);
    if (it != m_passes.end()) {
        channels = it->second.channels;
    }
    
    bool success = compilePass(type, code, channels);
    if (!success) {
        error = getPassError(type);
    }
    return success;
}

void MultiPassRenderer::setChannelBinding(int channel, int binding) {
    if (channel < 0 || channel > 3) return;
    
    PassRenderState& pass = getOrCreatePass(ShaderPassType::Image);
    pass.channels[channel] = binding;
}

void MultiPassRenderer::setBufferChannelBinding(int bufferIndex, int channel, int binding) {
    if (bufferIndex < 0 || bufferIndex > 3) return;
    if (channel < 0 || channel > 3) return;
    
    ShaderPassType type = BufferManager::indexToType(bufferIndex);
    PassRenderState& pass = getOrCreatePass(type);
    pass.channels[channel] = binding;
}

void MultiPassRenderer::render(UniformManager& uniformManager, Renderer& renderer) {
    // 包装回调函数
    auto uniformsCallback = [&uniformManager](GLuint program, ShaderPassType type) {
        (void)type;  // 所有 Pass 使用相同的 uniforms
        uniformManager.applyUniforms(program);
    };
    
    auto bindTexturesCallback = [this](GLuint program, int channel, int binding) {
        // 绑定 Buffer 或 Texture 到 channel
        bindBufferTexture(program, channel, binding);
    };
    
    auto renderQuadCallback = [&renderer]() {
        renderer.renderFullscreenQuad();
    };
    
    // 调用主渲染函数
    render(uniformsCallback, bindTexturesCallback, renderQuadCallback);
}

} // namespace shadertoy
