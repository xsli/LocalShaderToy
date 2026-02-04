#pragma once

#include <string>
#include <functional>

struct ImFont;

namespace shadertoy {

class ShaderEditor {
public:
    ShaderEditor() = default;
    
    void init();
    void render();
    
    void setText(const std::string& text);
    std::string getText() const;
    
    void setCompileCallback(std::function<void(const std::string&)> callback) {
        m_compileCallback = callback;
    }
    
    void setErrorMarkers(const std::string& errors);
    void clearErrorMarkers();
    
    // 设置编辑器专用字体
    void setFont(ImFont* font) { m_editorFont = font; }

private:
    std::string m_text;
    std::function<void(const std::string&)> m_compileCallback;
    bool m_needsCompile = false;
    ImFont* m_editorFont = nullptr;
    
    // 创建增强的 GLSL 语言定义
    static void setupEnhancedGLSL();
};

// 全局函数：设置编辑器字体（需要在 UI 初始化后调用）
void setEditorFont(ImFont* font);

} // namespace shadertoy
