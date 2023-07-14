#ifndef _IMGUI_PLUGINS_HPP
#define _IMGUI_PLUGINS_HPP
#include <string>

#include "imgui.h"
#include "imgui_internal.h"

namespace ImGui
{
bool InputText(const char* label, std::string* str, ImGuiInputTextFlags flags);
bool SelectableInput(const char* str_id, bool selected, ImGuiSelectableFlags flags, char* buf, size_t buf_size);
}  // namespace ImGui
#endif