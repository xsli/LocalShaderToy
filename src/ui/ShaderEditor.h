#pragma once

#include <string>
#include <functional>

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

private:
    std::string m_text;
    std::function<void(const std::string&)> m_compileCallback;
    bool m_needsCompile = false;
};

} // namespace shadertoy
