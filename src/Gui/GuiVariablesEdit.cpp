#include "Gui.hpp"

void Gui::drawVariableEditWindow()
{
	if (showVariableEditWindow)
		ImGui::OpenPopup("Variable Edit");

	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(950 * contentScale, 500 * contentScale));
	if (ImGui::BeginPopupModal("Variable Edit", &showVariableEditWindow, 0))
	{
		const float buttonHeight = 25.0f * contentScale;
		ImGui::SetCursorPos(ImVec2(0, ImGui::GetWindowSize().y - buttonHeight / 2.0f - ImGui::GetFrameHeightWithSpacing()));

		if (ImGui::Button("Done", ImVec2(-1, buttonHeight)))
		{
			showVariableEditWindow = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}