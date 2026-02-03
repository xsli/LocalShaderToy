#include "ShaderEditor.h"

#include <imgui.h>
#include <TextEditor.h>

namespace shadertoy {

static TextEditor s_editor;

void ShaderEditor::init() {
    auto lang = TextEditor::LanguageDefinition::GLSL();
    s_editor.SetLanguageDefinition(lang);
    s_editor.SetShowWhitespaces(false);
}

void ShaderEditor::render() {
    ImGui::Begin("Shader Editor");
    
    if (ImGui::Button("Compile (F5)") || ImGui::IsKeyPressed(ImGuiKey_F5)) {
        if (m_compileCallback) {
            m_compileCallback(getText());
        }
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        // TODO: Reset to default shader
    }
    
    s_editor.Render("Editor");
    
    ImGui::End();
}

void ShaderEditor::setText(const std::string& text) {
    m_text = text;
    s_editor.SetText(text);
}

std::string ShaderEditor::getText() const {
    return s_editor.GetText();
}

void ShaderEditor::setErrorMarkers(const std::string& errors) {
    // TODO: Parse error string and set markers
    TextEditor::ErrorMarkers markers;
    // Simple parsing for line numbers
    // markers[lineNumber] = "Error message";
    s_editor.SetErrorMarkers(markers);
}

void ShaderEditor::clearErrorMarkers() {
    TextEditor::ErrorMarkers markers;
    s_editor.SetErrorMarkers(markers);
}

} // namespace shadertoy
