#include "FileDialog.h"

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")
#else
#include <cstdlib>
#endif

#include <algorithm>

namespace shadertoy {

std::vector<FileDialog::Filter> FileDialog::shaderFilters() {
    return {
        {"GLSL Shader", "*.glsl;*.frag;*.fs"},
        {"All Files", "*.*"}
    };
}

std::vector<FileDialog::Filter> FileDialog::projectFilters() {
    return {
        {"Shader Project", "*.json"},
        {"GLSL Shader", "*.glsl;*.frag;*.fs"},
        {"All Files", "*.*"}
    };
}

std::vector<FileDialog::Filter> FileDialog::imageFilters() {
    return {
        {"Image Files", "*.png;*.jpg;*.jpeg;*.bmp;*.tga"},
        {"All Files", "*.*"}
    };
}

#ifdef _WIN32

static std::string buildFilterString(const std::vector<FileDialog::Filter>& filters) {
    std::string result;
    for (const auto& filter : filters) {
        result += filter.name;
        result += '\0';
        result += filter.extensions;
        result += '\0';
    }
    result += '\0';
    return result;
}

std::string FileDialog::openFile(
    const std::string& title,
    const std::vector<Filter>& filters,
    const std::string& defaultPath
) {
    char filename[MAX_PATH] = "";
    
    std::string filterStr = buildFilterString(
        filters.empty() ? std::vector<Filter>{{"All Files", "*.*"}} : filters
    );
    
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter = filterStr.c_str();
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title.c_str();
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    
    if (!defaultPath.empty()) {
        ofn.lpstrInitialDir = defaultPath.c_str();
    }
    
    if (GetOpenFileNameA(&ofn)) {
        return std::string(filename);
    }
    
    return "";
}

std::string FileDialog::saveFile(
    const std::string& title,
    const std::vector<Filter>& filters,
    const std::string& defaultPath,
    const std::string& defaultName
) {
    char filename[MAX_PATH] = "";
    
    if (!defaultName.empty()) {
        strncpy_s(filename, defaultName.c_str(), MAX_PATH - 1);
    }
    
    std::string filterStr = buildFilterString(
        filters.empty() ? std::vector<Filter>{{"All Files", "*.*"}} : filters
    );
    
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter = filterStr.c_str();
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title.c_str();
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    ofn.lpstrDefExt = "json";
    
    if (!defaultPath.empty()) {
        ofn.lpstrInitialDir = defaultPath.c_str();
    }
    
    if (GetSaveFileNameA(&ofn)) {
        return std::string(filename);
    }
    
    return "";
}

std::string FileDialog::selectFolder(const std::string& title) {
    char path[MAX_PATH] = "";
    
    BROWSEINFOA bi = {};
    bi.lpszTitle = title.c_str();
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    
    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
    if (pidl) {
        SHGetPathFromIDListA(pidl, path);
        CoTaskMemFree(pidl);
        return std::string(path);
    }
    
    return "";
}

#else
// Linux/macOS 实现 (使用 zenity 或 kdialog)

std::string FileDialog::openFile(
    const std::string& title,
    const std::vector<Filter>& filters,
    const std::string& defaultPath
) {
    std::string cmd = "zenity --file-selection --title=\"" + title + "\"";
    
    if (!filters.empty()) {
        for (const auto& filter : filters) {
            cmd += " --file-filter=\"" + filter.name + "|" + filter.extensions + "\"";
        }
    }
    
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    
    char buffer[1024];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    pclose(pipe);
    
    // 移除末尾换行
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }
    
    return result;
}

std::string FileDialog::saveFile(
    const std::string& title,
    const std::vector<Filter>& filters,
    const std::string& defaultPath,
    const std::string& defaultName
) {
    std::string cmd = "zenity --file-selection --save --title=\"" + title + "\"";
    
    if (!defaultName.empty()) {
        cmd += " --filename=\"" + defaultName + "\"";
    }
    
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    
    char buffer[1024];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    pclose(pipe);
    
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }
    
    return result;
}

std::string FileDialog::selectFolder(const std::string& title) {
    std::string cmd = "zenity --file-selection --directory --title=\"" + title + "\"";
    
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    
    char buffer[1024];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    pclose(pipe);
    
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }
    
    return result;
}

#endif

} // namespace shadertoy
