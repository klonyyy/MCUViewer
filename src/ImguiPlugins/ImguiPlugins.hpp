#ifndef _IMGUI_PLUGINS_HPP
#define _IMGUI_PLUGINS_HPP
#include <string>

#include "imgui.h"
#include "imgui_internal.h"

namespace ImGui
{
bool InputText(const char* label, std::string* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data);
bool SelectableInput(const char* str_id, bool selected, ImGuiSelectableFlags flags, char* buf, size_t buf_size);
void HelpMarker(const char* desc);
}  // namespace ImGui
#endif