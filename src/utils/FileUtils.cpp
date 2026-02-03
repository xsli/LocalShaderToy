#include "FileUtils.h"

#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace shadertoy {

std::string FileUtils::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

bool FileUtils::writeFile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    file << content;
    return true;
}

std::string FileUtils::getFileExtension(const std::string& path) {
    fs::path p(path);
    return p.extension().string();
}

std::string FileUtils::getFileName(const std::string& path) {
    fs::path p(path);
    return p.filename().string();
}

std::string FileUtils::getDirectory(const std::string& path) {
    fs::path p(path);
    return p.parent_path().string();
}

bool FileUtils::createDirectory(const std::string& path) {
    return fs::create_directories(path);
}

} // namespace shadertoy
