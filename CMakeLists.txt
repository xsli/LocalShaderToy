cmake_minimum_required(VERSION 3.20)
project(LocalShadertoy VERSION 1.0.0 LANGUAGES CXX C)

# C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 选项
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(SHADERTOY_BUILD_EXAMPLES "Build example shaders" ON)

# ============================================================================
# 依赖项配置
# ============================================================================

# 使用 FetchContent 自动下载依赖
include(FetchContent)

# GLFW - 窗口管理
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.3.8
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# GLM - 数学库
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
)
FetchContent_MakeAvailable(glm)

# Dear ImGui - GUI (使用最新稳定版)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.6-docking
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()

# ImGuiColorTextEdit - 代码编辑器 (使用已修复兼容性的 fork)
FetchContent_Declare(
    imgui_textedit
    GIT_REPOSITORY https://github.com/BalazsJako/ImGuiColorTextEdit.git
    GIT_TAG        master
)
FetchContent_GetProperties(imgui_textedit)
if(NOT imgui_textedit_POPULATED)
    FetchContent_Populate(imgui_textedit)
endif()

# JSON 解析
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(json)

# ============================================================================
# GLAD 配置 (本地生成)
# ============================================================================
add_library(glad STATIC
    ${CMAKE_SOURCE_DIR}/third_party/glad/src/glad.c
)
target_include_directories(glad PUBLIC
    ${CMAKE_SOURCE_DIR}/third_party/glad/include
)

# ============================================================================
# ImGui 库
# ============================================================================
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${imgui_textedit_SOURCE_DIR}/TextEditor.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${imgui_textedit_SOURCE_DIR}
)
target_link_libraries(imgui PUBLIC glfw)

# 为 TextEditor.cpp 添加兼容性：强制包含 imgui_internal.h
# 这解决了 PushItemFlag/PopItemFlag 在新版 ImGui 中被移到 internal 的问题
set_source_files_properties(
    ${imgui_textedit_SOURCE_DIR}/TextEditor.cpp
    PROPERTIES COMPILE_FLAGS "/FI\"${imgui_SOURCE_DIR}/imgui_internal.h\""
)

# ============================================================================
# STB 头文件库
# ============================================================================
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE
    ${CMAKE_SOURCE_DIR}/third_party/stb
)

# ============================================================================
# 主程序源文件
# ============================================================================
set(SOURCES
    src/main.cpp
    src/core/Application.cpp
    src/core/ShaderEngine.cpp
    src/core/UniformManager.cpp
    src/core/ShaderProject.cpp
    src/core/ProjectManager.cpp
    src/core/ScreensaverMode.cpp
    src/renderer/Renderer.cpp
    src/renderer/Framebuffer.cpp
    src/renderer/Texture.cpp
    src/renderer/TextureManager.cpp
    src/renderer/NoiseGenerator.cpp
    src/transpiler/GLSLTranspiler.cpp
    src/input/ResourceLoader.cpp
    src/ui/UIManager.cpp
    src/ui/ShaderEditor.cpp
    src/utils/FileUtils.cpp
    src/utils/Timer.cpp
    src/utils/FileDialog.cpp
)

set(HEADERS
    src/core/Application.h
    src/core/ShaderEngine.h
    src/core/UniformManager.h
    src/renderer/Renderer.h
    src/renderer/Framebuffer.h
    src/renderer/Texture.h
    src/transpiler/GLSLTranspiler.h
    src/input/ResourceLoader.h
    src/ui/UIManager.h
    src/ui/ShaderEditor.h
    src/utils/FileUtils.h
    src/utils/Timer.h
)

# ============================================================================
# 主可执行文件
# ============================================================================
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glad
    glm::glm
    imgui
    stb
    nlohmann_json::nlohmann_json
)

# Windows 特定配置
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE opengl32)
    # 隐藏控制台窗口 (Release 模式)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()
endif()

# Linux 特定配置
if(UNIX AND NOT APPLE)
    find_package(OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL ${CMAKE_DL_LIBS})
endif()

# macOS 特定配置
if(APPLE)
    find_package(OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)
endif()

# ============================================================================
# 复制资源文件到输出目录
# ============================================================================
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
)

# ============================================================================
# Windows 屏幕保护程序 (.scr) 构建
# ============================================================================
if(WIN32)
    # 创建 .scr 文件（复制 .exe 并重命名为 .scr）
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:${PROJECT_NAME}>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.scr
        COMMENT "Creating screensaver file: ${PROJECT_NAME}.scr"
    )
    
    message(STATUS "Screensaver: ${PROJECT_NAME}.scr will be created in bin folder")
endif()

# ============================================================================
# 安装配置
# ============================================================================
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
install(DIRECTORY resources/ DESTINATION bin/resources)
install(DIRECTORY shaders/ DESTINATION bin/shaders)

message(STATUS "=========================================")
message(STATUS "Local Shadertoy Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "=========================================")
