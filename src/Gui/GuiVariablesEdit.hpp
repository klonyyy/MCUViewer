#ifndef _GUI_VARIABLESEDIT_HPP
#define _GUI_VARIABLESEDIT_HPP

#include "GuiHelper.hpp"
#include "Popup.hpp"
#include "Variable.hpp"
#include "imgui.h"

class VariableEditWindow
{
   public:
	VariableEditWindow(std::map<std::string, std::shared_ptr<Variable>>* vars) : vars(vars)
	{
		selectVariableWindow = std::make_unique<SelectVariableWindow>(vars, &selection);
	}

	void drawVariableEditWindow()
	{
		if (showVariableEditWindow)
			ImGui::OpenPopup("Variable Edit");

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(700 * GuiHelper::contentScale, 500 * GuiHelper::contentScale));
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

			popup.handle();
			selectVariableWindow->draw();
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
		std::string shift = std::to_string(editedVariable->getShift());
		std::string mask = std::string("0x") + std::string(GuiHelper::intToHexString(editedVariable->getMask()));
		bool shouldUpdateFromElf = editedVariable->getShouldUpdateFromElf();
		bool fractional = editedVariable->getIsFractional();
		static bool selectNameManually = false;

		ImGui::Dummy(ImVec2(-1, 5));
		GuiHelper::drawCenteredText("Variable");
		ImGui::Separator();

		GuiHelper::drawTextAlignedToSize("name:", alignment);
		ImGui::SameLine();
		if (ImGui::InputText("##name", &name, ImGuiInputTextFlags_EnterReturnsTrue, NULL, NULL))
		{
			if (!vars->contains(name))
				editedVariable->rename(name);
			else
				popup.show("Error!", "Variable already exists!", 1.5f);
		}

		GuiHelper::drawTextAlignedToSize("specify tracked name:", alignment);
		ImGui::SameLine();
		ImGui::Checkbox("##selectNameManually", &selectNameManually);

		if (selectNameManually)
		{
			GuiHelper::drawTextAlignedToSize("tracked variable:", alignment);
			ImGui::SameLine();
			std::string trackedVarName = selection.empty() ? editedVariable->getTrackedName() : *selection.begin();
			if (ImGui::InputText("##trackedVarName", &trackedVarName, ImGuiInputTextFlags_EnterReturnsTrue, NULL, NULL) || editedVariable->getTrackedName() != trackedVarName)
			{
				editedVariable->setTrackedName(trackedVarName);
			}
			ImGui::SameLine();
			if (ImGui::Button("select...", ImVec2(65 * GuiHelper::contentScale, 19 * GuiHelper::contentScale)))
				selectVariableWindow->setShowState(true);
		}
		else
			editedVariable->setTrackedName(name);

		GuiHelper::drawTextAlignedToSize("update from *.elf:", alignment);
		ImGui::SameLine();
		if (ImGui::Checkbox("##shouldUpdateFromElf", &shouldUpdateFromElf))
		{
			editedVariable->setShouldUpdateFromElf(shouldUpdateFromElf);
		}

		ImGui::BeginDisabled(shouldUpdateFromElf);

		GuiHelper::drawTextAlignedToSize("address:", alignment);
		ImGui::SameLine();
		if (ImGui::InputText("##address", &address, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase, NULL, NULL))
		{
			uint32_t addressDec = GuiHelper::hexStringToDecimal(address);
			editedVariable->setAddress(addressDec);
		}

		GuiHelper::drawTextAlignedToSize("type:", alignment);
		ImGui::SameLine();
		int32_t type = static_cast<int32_t>(editedVariable->getType());

		if (ImGui::Combo("##varType", &type, Variable::types, IM_ARRAYSIZE(Variable::types)))
		{
			editedVariable->setType(static_cast<Variable::type>(type));
		}

		GuiHelper::drawTextAlignedToSize("size:", alignment);
		ImGui::SameLine();
		ImGui::InputText("##size", &size, 0, NULL, NULL);

		ImGui::EndDisabled();

		/* POSTPROCESSING */
		ImGui::Dummy(ImVec2(-1, 5));
		GuiHelper::drawCenteredText("Postprocessing");
		ImGui::Separator();

		GuiHelper::drawTextAlignedToSize("shift right:", alignment);
		ImGui::SameLine();
		if (ImGui::InputText("##shift", &shift, ImGuiInputTextFlags_CharsDecimal, NULL, NULL))
		{
			uint32_t shiftDec = GuiHelper::convertStringToNumber<uint32_t>(shift);
			editedVariable->setShift(shiftDec);
		}

		GuiHelper::drawTextAlignedToSize("mask:", alignment);
		ImGui::SameLine();
		if (ImGui::InputText("##mask", &mask, ImGuiInputTextFlags_None, NULL, NULL))
		{
			uint32_t maskDec = GuiHelper::hexStringToDecimal(mask);
			editedVariable->setMask(maskDec);
		}

		/* INTERPRETATION */
		ImGui::Dummy(ImVec2(-1, 5));
		GuiHelper::drawCenteredText("Interpretation");
		ImGui::Separator();

		GuiHelper::drawTextAlignedToSize("fractional:", alignment);
		ImGui::SameLine();
		if (ImGui::Checkbox("##isfractional", &fractional))
		{
			editedVariable->setIsFractional(fractional);
		}

		if (fractional)
		{
			bool shouldUpdate = false;
			Variable::Fractional fractional = editedVariable->getFractional();
			std::string decimalBits = std::to_string(fractional.decimalBits);
			std::string fractionalBits = std::to_string(fractional.fractionalBits);

			GuiHelper::drawTextAlignedToSize("sign:", alignment);
			ImGui::SameLine();
			if (ImGui::Checkbox("##sign", &fractional.sign))
				shouldUpdate = true;

			GuiHelper::drawTextAlignedToSize("decimal bits:", alignment);
			ImGui::SameLine();
			if (ImGui::InputText("##decimal", &decimalBits, ImGuiInputTextFlags_CharsDecimal, NULL, NULL))
				shouldUpdate = true;

			GuiHelper::drawTextAlignedToSize("fractional bits:", alignment);
			ImGui::SameLine();
			if (ImGui::InputText("##fractional", &fractionalBits, ImGuiInputTextFlags_CharsDecimal, NULL, NULL))
				shouldUpdate = true;

			if (shouldUpdate)
			{
				fractional.decimalBits = GuiHelper::convertStringToNumber<uint32_t>(decimalBits);
				fractional.fractionalBits = GuiHelper::convertStringToNumber<uint32_t>(fractionalBits);
				editedVariable->setFractional(fractional);
			}
		}
	}

   private:
	/**
	 * @brief Text alignemnt in front of the input fields
	 *
	 */
	static constexpr size_t alignment = 22;

	bool showVariableEditWindow = false;

	std::shared_ptr<Variable> editedVariable = nullptr;

	std::map<std::string, std::shared_ptr<Variable>>* vars;

	Popup popup;

	std::set<std::string> selection;
	std::unique_ptr<SelectVariableWindow> selectVariableWindow;
};

#endif