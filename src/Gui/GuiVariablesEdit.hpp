#ifndef _GUI_VARIABLESEDIT_HPP
#define _GUI_VARIABLESEDIT_HPP

#include "GuiHelper.hpp"
#include "Variable.hpp"
#include "imgui.h"

class VariableEditWindow
{
   public:
	void drawVariableEditWindow()
	{
		if (showVariableEditWindow)
			ImGui::OpenPopup("Variable Edit");

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(950 * GuiHelper::contentScale, 500 * GuiHelper::contentScale));
		if (ImGui::BeginPopupModal("Variable Edit", &showVariableEditWindow, 0))
		{
			drawVariableEditSettings();

			const float buttonHeight = 25.0f * GuiHelper::contentScale;
			ImGui::SetCursorPos(ImVec2(0, ImGui::GetWindowSize().y - buttonHeight / 2.0f - ImGui::GetFrameHeightWithSpacing()));

			if (ImGui::Button("Done", ImVec2(-1, buttonHeight)))
			{
				showVariableEditWindow = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void setVariableToEdit(std::shared_ptr<Variable> variable)
	{
		editedVariable = variable;
	}

	void setShowVariableEditWindowState(bool state)
	{
		showVariableEditWindow = state;
	}

	void drawVariableEditSettings()
	{
		if (editedVariable == nullptr)
			return;

		std::string name = editedVariable->getName();
		std::string address = std::string("0x") + std::string(GuiHelper::intToHexString(editedVariable->getAddress()));
		std::string size = std::to_string(editedVariable->getSize());

		ImGui::Dummy(ImVec2(-1, 5));
		GuiHelper::drawCenteredText("Variable");
		ImGui::Separator();

		GuiHelper::drawTextAlignedToSize("name:", alignment);
		ImGui::SameLine();
		if (ImGui::InputText("##name", &name, 0, NULL, NULL))
		{
			editedVariable->setName(name);
		}

		GuiHelper::drawTextAlignedToSize("address:", alignment);
		ImGui::SameLine();
		ImGui::InputText("##address", &address, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase, NULL, NULL);

		GuiHelper::drawTextAlignedToSize("type:", alignment);
		ImGui::SameLine();
		int32_t type = static_cast<int32_t>(editedVariable->getType());

		if (ImGui::Combo("##varType", &type, Variable::types, IM_ARRAYSIZE(Variable::types)))
		{
		}

		GuiHelper::drawTextAlignedToSize("size:", alignment);
		ImGui::SameLine();
		ImGui::InputText("##size", &size, 0, NULL, NULL);

		ImGui::Dummy(ImVec2(-1, 5));
		GuiHelper::drawCenteredText("Interpretation");
		ImGui::Separator();
	}

   private:
	/**
	 * @brief Text alignemnt in front of the input fields
	 *
	 */
	static constexpr size_t alignment = 30;

	bool showVariableEditWindow = false;

	std::shared_ptr<Variable> editedVariable = nullptr;
};

#endif