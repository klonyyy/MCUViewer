#include "ImguiPlugins.hpp"

namespace ImGui
{
bool InputText(const char* label, std::string* str, ImGuiInputTextFlags flags)
{
	IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
	flags |= ImGuiInputTextFlags_CallbackResize;

	return InputText(label, (char*)str->c_str(), str->capacity() + 1, flags);
}

bool SelectableInput(const char* str_id, bool selected, ImGuiSelectableFlags flags, char* buf, size_t buf_size)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;
	ImVec2 pos_before = window->DC.CursorPos;

	PushID(str_id);

	PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(g.Style.ItemSpacing.x, g.Style.FramePadding.y * 2.0f));
	bool ret = Selectable("##Selectable", selected, flags | ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_AllowItemOverlap);
	PopStyleVar();

	ImGuiID id = window->GetID("##Input");
	KeepAliveID(id);
	bool temp_input_is_active = TempInputIsActive(id);
	bool temp_input_start = ret ? IsMouseDoubleClicked(0) : false;

	if (temp_input_start)
		SetActiveID(id, window);

	if (temp_input_is_active || temp_input_start)
	{
		ImVec2 pos_after = window->DC.CursorPos;
		window->DC.CursorPos = pos_before;
		ret = TempInputText(g.LastItemData.Rect, id, "##Input", buf, (int)buf_size, ImGuiInputTextFlags_None);
		window->DC.CursorPos = pos_after;
	}
	else
	{
		window->DrawList->AddText(pos_before, GetColorU32(ImGuiCol_Text), buf);
	}

	PopID();
	return ret;
}
};	// namespace ImGui