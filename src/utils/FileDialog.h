#pragma once

#include <string>
#include <vector>

namespace shadertoy {

// 跨平台文件对话框
class FileDialog {
public:
    struct Filter {
        std::string name;
        std::string extensions;  // e.g., "*.json;*.glsl"
    };
    
    // 打开文件对话框
    static std::string openFile(
        const std::string& title = "Open File",
        const std::vector<Filter>& filters = {},
        const std::string& defaultPath = ""
    );
    
    // 保存文件对话框
    static std::string saveFile(
        const std::string& title = "Save File",
        const std::vector<Filter>& filters = {},
        const std::string& defaultPath = "",
        const std::string& defaultName = ""
    );
    
    // 选择文件夹对话框
    static std::string selectFolder(
        const std::string& title = "Select Folder"
    );
    
    // 常用过滤器
    static std::vector<Filter> shaderFilters();
    static std::vector<Filter> projectFilters();
    static std::vector<Filter> imageFilters();
};

} // namespace shadertoy
