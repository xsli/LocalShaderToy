#include "ProjectManager.h"
#include "../input/ResourceLoader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <regex>
#include <filesystem>

namespace fs = std::filesystem;

namespace shadertoy {

ProjectManager::ProjectManager() {
    loadRecentProjects();
}

void ProjectManager::newProject() {
    m_project = ShaderProject();
    notifyProjectChanged();
}

bool ProjectManager::loadProject(const std::string& path) {
    std::string content;
    if (!ResourceLoader::loadFile(path, content)) {
        std::cerr << "Failed to load file: " << path << std::endl;
        return false;
    }
    
    // 尝试检测格式
    if (!loadFromText(content)) {
        return false;
    }
    
    m_project.filePath = path;
    m_project.modified = false;
    
    // 从文件名推断项目名
    if (m_project.name.empty() || m_project.name == "Untitled") {
        fs::path p(path);
        m_project.name = p.stem().string();
    }
    
    addRecentProject(path);
    notifyProjectChanged();
    return true;
}

bool ProjectManager::saveProject(const std::string& path) {
    std::string savePath = path.empty() ? m_project.filePath : path;
    
    if (savePath.empty()) {
        std::cerr << "No save path specified" << std::endl;
        return false;
    }
    
    return saveProjectAs(savePath);
}

bool ProjectManager::saveProjectAs(const std::string& path) {
    std::string json = m_project.toJson();
    
    if (!ResourceLoader::saveFile(path, json)) {
        std::cerr << "Failed to save file: " << path << std::endl;
        return false;
    }
    
    m_project.filePath = path;
    m_project.modified = false;
    
    addRecentProject(path);
    return true;
}

bool ProjectManager::loadFromCode(const std::string& code) {
    // 直接将代码设置为Image pass
    m_project = ShaderProject::fromCode(code);
    m_project.modified = true;
    notifyProjectChanged();
    return true;
}

bool ProjectManager::loadFromJson(const std::string& json) {
    ShaderProject proj = ShaderProject::fromJson(json);
    
    // 检查是否加载成功 (至少有一个pass)
    if (proj.passes.empty()) {
        return false;
    }
    
    m_project = proj;
    m_project.modified = true;
    notifyProjectChanged();
    return true;
}

bool ProjectManager::loadFromText(const std::string& text) {
    // 首先尝试作为JSON解析
    if (isValidJson(text)) {
        return loadFromJson(text);
    }
    
    // 如果看起来像GLSL代码
    if (isValidGlslCode(text)) {
        return loadFromCode(text);
    }
    
    // 最后尝试作为普通代码
    return loadFromCode(text);
}

bool ProjectManager::isValidJson(const std::string& text) const {
    // 简单检查：是否以 { 开头（去除空白后）
    std::string trimmed = text;
    size_t start = trimmed.find_first_not_of(" \t\n\r");
    if (start != std::string::npos && trimmed[start] == '{') {
        // 尝试解析
        try {
            nlohmann::json::parse(text);
            return true;
        } catch (...) {
            return false;
        }
    }
    return false;
}

bool ProjectManager::isValidGlslCode(const std::string& text) const {
    // 检查是否包含 mainImage 函数（Shadertoy 标准）
    std::regex mainImageRegex(R"(void\s+mainImage\s*\()");
    if (std::regex_search(text, mainImageRegex)) {
        return true;
    }
    
    // 检查是否包含标准 main 函数
    std::regex mainRegex(R"(void\s+main\s*\(\s*\))");
    if (std::regex_search(text, mainRegex)) {
        return true;
    }
    
    // 检查是否包含 GLSL 关键字
    if (text.find("vec2") != std::string::npos ||
        text.find("vec3") != std::string::npos ||
        text.find("vec4") != std::string::npos ||
        text.find("uniform") != std::string::npos ||
        text.find("fragColor") != std::string::npos) {
        return true;
    }
    
    return false;
}

std::string ProjectManager::exportAsGlsl() const {
    return m_project.getImageCode();
}

std::string ProjectManager::exportAsJson() const {
    return m_project.toJson();
}

std::string ProjectManager::exportAsShadertoyUrl() const {
    // 这个功能需要在线服务，暂时返回空
    return "";
}

void ProjectManager::addRecentProject(const std::string& path) {
    // 移除已存在的相同路径
    auto it = std::find(m_recentProjects.begin(), m_recentProjects.end(), path);
    if (it != m_recentProjects.end()) {
        m_recentProjects.erase(it);
    }
    
    // 添加到开头
    m_recentProjects.insert(m_recentProjects.begin(), path);
    
    // 限制数量
    const size_t maxRecent = 10;
    if (m_recentProjects.size() > maxRecent) {
        m_recentProjects.resize(maxRecent);
    }
    
    saveRecentProjects();
}

void ProjectManager::loadRecentProjects() {
    std::string configPath = ResourceLoader::getExecutablePath() + "/recent_projects.txt";
    std::string content;
    
    if (ResourceLoader::loadFile(configPath, content)) {
        m_recentProjects.clear();
        std::istringstream iss(content);
        std::string line;
        while (std::getline(iss, line)) {
            if (!line.empty() && fs::exists(line)) {
                m_recentProjects.push_back(line);
            }
        }
    }
}

void ProjectManager::saveRecentProjects() {
    std::string configPath = ResourceLoader::getExecutablePath() + "/recent_projects.txt";
    std::ostringstream oss;
    
    for (const auto& path : m_recentProjects) {
        oss << path << "\n";
    }
    
    ResourceLoader::saveFile(configPath, oss.str());
}

void ProjectManager::notifyProjectChanged() {
    if (m_projectChangedCallback) {
        m_projectChangedCallback(m_project);
    }
}

} // namespace shadertoy
