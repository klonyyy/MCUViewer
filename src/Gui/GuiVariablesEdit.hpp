#ifndef _GUI_VARIABLESEDIT_HPP
#define _GUI_VARIABLESEDIT_HPP

#include "GuiHelper.hpp"
#include "Popup.hpp"
#include "Variable.hpp"
#include "VariableHandler.hpp"
#include "imgui.h"

class VariableEditWindow
{
   public:
	VariableEditWindow(VariableHandler* variableHandler) : variableHandler(variableHandler)
	{
		selectVariableWindow = std::make_unique<SelectVariableWindow>(variableHandler, &selection);
	}

	void draw()
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
				selection.clear();
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
		if (showVariableEditWindow != state)
			stateChanged = true;
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
		bool selectNameManually = editedVariable->getIsTrackedNameDifferent();

		ImGui::Dummy(ImVec2(-1, 5));
		GuiHelper::drawCenteredText("Variable");
		ImGui::Separator();

		if (stateChanged)
		{
			ImGui::SetKeyboardFocusHere(0);
			stateChanged = false;
		}

		GuiHelper::drawTextAlignedToSize("name:", alignment);
		ImGui::SameLine();
		if (ImGui::InputText("##name", &name, ImGuiInputTextFlags_None, NULL, NULL))
		{
			if (!variableHandler->contains(name))
			{
				variableHandler->renameVariable(editedVariable->getName(), name);
			}
			else
				popup.show("Error!", "Variable already exists!", 1.5f);
		}

		GuiHelper::drawTextAlignedToSize("specify tracked name:", alignment);
		ImGui::SameLine();
		if (ImGui::Checkbox("##selectNameManually", &selectNameManually))
			editedVariable->setIsTrackedNameDifferent(selectNameManually);

		ImGui::BeginDisabled(!selectNameManually);

		GuiHelper::drawTextAlignedToSize("tracked variable:", alignment);
		ImGui::SameLine();

		std::string trackedVarName = selection.empty() ? editedVariable->getTrackedName() : *selection.begin();

		if (ImGui::InputText("##trackedVarName", &trackedVarName, ImGuiInputTextFlags_None, NULL, NULL) || editedVariable->getTrackedName() != trackedVarName)
		{
			editedVariable->setTrackedName(trackedVarName);
			editedVariable->setAddress(variableHandler->getVariable(trackedVarName)->getAddress());
			editedVariable->setType(variableHandler->getVariable(trackedVarName)->getType());
			selection.clear();
		}
		ImGui::SameLine();
		if (ImGui::Button("select...", ImVec2(65 * GuiHelper::contentScale, 19 * GuiHelper::contentScale)))
			selectVariableWindow->setShowState(true);

		ImGui::EndDisabled();

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

		GuiHelper::drawTextAlignedToSize("base type:", alignment);
		ImGui::SameLine();
		int32_t type = static_cast<int32_t>(editedVariable->getType());

		if (ImGui::Combo("##varType", &type, Variable::types, IM_ARRAYSIZE(Variable::types)))
		{
			editedVariable->setType(static_cast<Variable::Type>(type));
		}

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

		GuiHelper::drawTextAlignedToSize("type:", alignment);
		ImGui::SameLine();
		int32_t highLeveltype = static_cast<int32_t>(editedVariable->getHighLevelType());

		if (ImGui::Combo("##varHighLevelType", &highLeveltype, Variable::highLevelTypes, IM_ARRAYSIZE(Variable::highLevelTypes)))
		{
			editedVariable->setHighLevelType(static_cast<Variable::HighLevelType>(highLeveltype));
		}

		if (editedVariable->getHighLevelType() == Variable::HighLevelType::SIGNEDFRAC || editedVariable->getHighLevelType() == Variable::HighLevelType::UNSIGNEDFRAC)
		{
			Variable::Fractional fractional = editedVariable->getFractional();
			std::string base = std::to_string(fractional.base);
			std::string fractionalBits = std::to_string(fractional.fractionalBits);
			bool shouldUpdate = false;

			GuiHelper::drawTextAlignedToSize("fractional bits:", alignment);
			ImGui::SameLine();
			if (ImGui::InputText("##fractional", &fractionalBits, ImGuiInputTextFlags_CharsDecimal, NULL, NULL))
				shouldUpdate = true;

			GuiHelper::drawTextAlignedToSize("base:", alignment);
			ImGui::SameLine();
			if (ImGui::InputText("##base", &base, ImGuiInputTextFlags_CharsDecimal, NULL, NULL))
				shouldUpdate = true;

			if (shouldUpdate)
			{
				fractional.fractionalBits = GuiHelper::convertStringToNumber<uint32_t>(fractionalBits);
				fractional.base = GuiHelper::convertStringToNumber<double>(base);
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
	bool stateChanged = false;

	std::shared_ptr<Variable> editedVariable = nullptr;

	VariableHandler* variableHandler;

	Popup popup;

	std::set<std::string> selection;
	std::unique_ptr<SelectVariableWindow> selectVariableWindow;
};

#endif