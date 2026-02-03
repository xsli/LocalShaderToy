/**
 * ImGui Compatibility Layer
 * 
 * 解决 ImGuiColorTextEdit 与新版 ImGui 的 API 兼容性问题
 * PushItemFlag/PopItemFlag 在 ImGui 1.89+ 中被移动到 imgui_internal.h
 */

#pragma once

#include <imgui.h>
#include <imgui_internal.h>  // 包含内部 API

// 如果 ImGui 版本 >= 1.89，需要使用内部 API
// 这个头文件确保 TextEditor.cpp 可以访问到这些函数

// 在新版 ImGui 中，这些函数仍然存在于 imgui_internal.h 中
// 我们只需要确保 TextEditor.cpp 能够找到它们

#ifndef IMGUI_HAS_IMSTR
// 兼容旧版 ImGui 的宏定义
#endif
