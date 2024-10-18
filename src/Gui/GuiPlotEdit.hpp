#ifndef _GUI_PLOTEDIT_HPP
#define _GUI_PLOTEDIT_HPP

#include "GuiHelper.hpp"
#include "Plot.hpp"
#include "imgui.h"

class PlotEditWindow
{
   public:
	void drawPlotEditWindow()
	{
		if (showPlotEditWindow)
			ImGui::OpenPopup("Plot Edit");

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(950 * GuiHelper::contentScale, 500 * GuiHelper::contentScale));
		if (ImGui::BeginPopupModal("Plot Edit", &showPlotEditWindow, 0))
		{
			drawPlotEditSettings();

			const float buttonHeight = 25.0f * GuiHelper::contentScale;
			ImGui::SetCursorPos(ImVec2(0, ImGui::GetWindowSize().y - buttonHeight / 2.0f - ImGui::GetFrameHeightWithSpacing()));

			if (ImGui::Button("Done", ImVec2(-1, buttonHeight)))
			{
				showPlotEditWindow = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void setPlotToEdit(std::shared_ptr<Plot> variable)
	{
		editedPlot = variable;
	}

	void setShowPlotEditWindowState(bool state)
	{
		showPlotEditWindow = state;
	}

	void drawPlotEditSettings()
	{
		if (ImGui::TreeNode("Advanced, with Selectable nodes"))
		{
			for (int i = 0; i < 6; i++)
			{
				bool node_open = ImGui::TreeNodeEx((void*)(intptr_t)i, 0, "Selectable Node %d", i);
			}

			ImGui::TreePop();
		}
	}

   private:
	/**
	 * @brief Text alignemnt in front of the input fields
	 *
	 */
	static constexpr size_t alignment = 30;

	bool showPlotEditWindow = false;

	std::shared_ptr<Plot> editedPlot = nullptr;
};

#endif