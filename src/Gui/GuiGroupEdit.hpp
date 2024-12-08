#pragma once

#include "GuiHelper.hpp"
#include "Plot.hpp"
#include "PlotGroupHandler.hpp"
#include "Popup.hpp"
#include "imgui.h"

class GroupEditWindow
{
   public:
	GroupEditWindow(PlotGroupHandler* plotGroupHandler) : plotGroupHandler(plotGroupHandler)
	{
	}

	void draw()
	{
		if (showGroupEditWindow)
			ImGui::OpenPopup("Group Edit");

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(700 * GuiHelper::contentScale, 500 * GuiHelper::contentScale));
		if (ImGui::BeginPopupModal("Group Edit", &showGroupEditWindow, 0))
		{
			drawGroupEditSettings();

			const float buttonHeight = 25.0f * GuiHelper::contentScale;
			ImGui::SetCursorPos(ImVec2(0, ImGui::GetWindowSize().y - buttonHeight / 2.0f - ImGui::GetFrameHeightWithSpacing()));

			if (ImGui::Button("Done", ImVec2(-1, buttonHeight)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
			{
				showGroupEditWindow = false;
				ImGui::CloseCurrentPopup();
			}

			popup.handle();
			ImGui::EndPopup();
		}
	}

	void setGroupToEdit(std::shared_ptr<PlotGroup> group)
	{
		editedGroup = group;
	}

	void setShowGroupEditWindowState(bool state)
	{
		if (showGroupEditWindow != state)
			stateChanged = true;
		showGroupEditWindow = state;
	}

	void drawGroupEditSettings()
	{
		if (editedGroup == nullptr)
			return;

		std::string name = editedGroup->getName();

		ImGui::Dummy(ImVec2(-1, 5));
		GuiHelper::drawCenteredText("Group");
		ImGui::Separator();

		GuiHelper::drawTextAlignedToSize("name:", alignment);
		ImGui::SameLine();

		if (stateChanged)
		{
			ImGui::SetKeyboardFocusHere(0);
			stateChanged = false;
		}

		ImGui::InputText("##name", &name, ImGuiInputTextFlags_None, NULL, NULL);

		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			if (!plotGroupHandler->checkIfGroupExists(name))
			{
				std::string oldName = editedGroup->getName();
				plotGroupHandler->renameGroup(oldName, name);
			}
			else
				popup.show("Error!", "Group already exists!", 1.5f);
		}

		PlotGroup::Type groupType = editedGroup->getType();

		GuiHelper::drawTextAlignedToSize("type:", alignment);
		ImGui::SameLine();
		ImGui::RadioButton("sampling", (int32_t*)(&groupType), 0);
		ImGui::SameLine();
		ImGui::RadioButton("recorder", (int32_t*)(&groupType), 1);
		ImGui::SameLine();
		ImGui::HelpMarker("A sampling group is sampled asynchonously in regular intervals without additional software on target's side. \r\n A recorder group is a group used for registering high frequency signals with predefined intervals. Recorder file and sampling function execution is needed for proper operation.");

		if (groupType == PlotGroup::Type::RECORDER)
		{
			ImGui::Text("Recorder");
		}

		editedGroup->setType(groupType);
	}

   private:
	/**
	 * @brief Text alignemnt in front of the input fields
	 *
	 */
	static constexpr size_t alignment = 18;

	bool showGroupEditWindow = false;

	PlotGroupHandler* plotGroupHandler;
	std::shared_ptr<PlotGroup> editedGroup = nullptr;

	Popup popup;

	bool stateChanged = false;
};
