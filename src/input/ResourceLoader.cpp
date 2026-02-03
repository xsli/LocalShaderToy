#include "ResourceLoader.h"

#include <fstream>
#include <sstream>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

namespace fs = std::filesystem;

namespace shadertoy {

bool ResourceLoader::loadFile(const std::string& path, std::string& content) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    std::stringstream ss;
    ss << file.rdbuf();
    content = ss.str();
    return true;
}

bool ResourceLoader::saveFile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    file << content;
    return true;
}

std::vector<std::string> ResourceLoader::listFiles(const std::string& directory, const std::string& extension) {
    std::vector<std::string> files;
    
    if (!fs::exists(directory)) {
        return files;
    }
    
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            if (extension.empty() || entry.path().extension() == extension) {
                files.push_back(entry.path().string());
            }
        }
    }
    
    return files;
}

bool ResourceLoader::fileExists(const std::string& path) {
    return fs::exists(path);
}

std::string ResourceLoader::getExecutablePath() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    std::string path(buffer);
    return path.substr(0, path.find_last_of("\\/"));
#else
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) {
        buffer[len] = '\0';
        std::string path(buffer);
        return path.substr(0, path.find_last_of("/"));
    }
    return ".";
#endif
}

} // namespace shadertoy
