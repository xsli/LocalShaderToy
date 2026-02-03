#pragma once

#include "ShaderProject.h"
#include <string>
#include <vector>
#include <functional>

namespace shadertoy {

class ProjectManager {
public:
    ProjectManager();
    ~ProjectManager() = default;
    
    // 项目操作
    void newProject();
    bool loadProject(const std::string& path);
    bool saveProject(const std::string& path = "");
    bool saveProjectAs(const std::string& path);
    
    // 从剪贴板或文本加载
    bool loadFromCode(const std::string& code);
    bool loadFromJson(const std::string& json);
    
    // 获取当前项目
    ShaderProject& getProject() { return m_project; }
    const ShaderProject& getProject() const { return m_project; }
    
    // 项目状态
    bool isModified() const { return m_project.modified; }
    void setModified(bool modified) { m_project.modified = modified; }
    std::string getProjectPath() const { return m_project.filePath; }
    std::string getProjectName() const { return m_project.name; }
    
    // 最近项目
    const std::vector<std::string>& getRecentProjects() const { return m_recentProjects; }
    void addRecentProject(const std::string& path);
    void loadRecentProjects();
    void saveRecentProjects();
    
    // 回调
    using ProjectChangedCallback = std::function<void(const ShaderProject&)>;
    void setProjectChangedCallback(ProjectChangedCallback callback) { 
        m_projectChangedCallback = callback; 
    }
    
    // 智能检测并加载 (自动判断是代码还是JSON)
    bool loadFromText(const std::string& text);
    
    // 导出为不同格式
    std::string exportAsGlsl() const;
    std::string exportAsJson() const;
    std::string exportAsShadertoyUrl() const;

private:
    ShaderProject m_project;
    std::vector<std::string> m_recentProjects;
    ProjectChangedCallback m_projectChangedCallback;
    
    void notifyProjectChanged();
    bool isValidGlslCode(const std::string& text) const;
    bool isValidJson(const std::string& text) const;
};

} // namespace shadertoy
