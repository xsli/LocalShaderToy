#pragma once

#include <string>
#include <vector>

namespace shadertoy {

class ResourceLoader {
public:
    static bool loadFile(const std::string& path, std::string& content);
    static bool saveFile(const std::string& path, const std::string& content);
    static std::vector<std::string> listFiles(const std::string& directory, const std::string& extension = "");
    static bool fileExists(const std::string& path);
    static std::string getExecutablePath();
};

} // namespace shadertoy
