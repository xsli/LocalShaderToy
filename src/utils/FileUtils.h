#pragma once

#include <string>
#include <vector>

namespace shadertoy {

class FileUtils {
public:
    static std::string readFile(const std::string& path);
    static bool writeFile(const std::string& path, const std::string& content);
    static std::string getFileExtension(const std::string& path);
    static std::string getFileName(const std::string& path);
    static std::string getDirectory(const std::string& path);
    static bool createDirectory(const std::string& path);
};

} // namespace shadertoy
