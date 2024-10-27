#ifndef _GUI_PLOTEDIT_HPP
#define _GUI_PLOTEDIT_HPP

#include "GuiHelper.hpp"
#include "GuiSelectVariable.hpp"
#include "Plot.hpp"
#include "PlotGroup.hpp"
#include "Popup.hpp"
#include "imgui.h"

class PlotEditWindow
{
   public:
	PlotEditWindow(PlotHandler* plotHandler, PlotGroupHandler* plotGroupHandler, std::map<std::string, std::shared_ptr<Variable>>* vars) : plotHandler(plotHandler), plotGroupHandler(plotGroupHandler), vars(vars)
	{
		selectVariableWindow = std::make_unique<SelectVariableWindow>(vars, &selection);
	}

	void drawPlotEditWindow()
	{
		if (showPlotEditWindow)
			ImGui::OpenPopup("Plot Edit");

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(700 * GuiHelper::contentScale, 500 * GuiHelper::contentScale));
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

			popup.handle();
			selectVariableWindow->draw();
			ImGui::EndPopup();
		}
	}

	void setPlotToEdit(std::shared_ptr<Plot> plot)
	{
		editedPlot = plot;
	}

	void setShowPlotEditWindowState(bool state)
	{
		showPlotEditWindow = state;
	}

	void drawPlotEditSettings()
	{
		if (editedPlot == nullptr)
			return;

		std::string name = editedPlot->getName();

		ImGui::Dummy(ImVec2(-1, 5));
		GuiHelper::drawCenteredText("Plot");
		ImGui::Separator();

		GuiHelper::drawTextAlignedToSize("name:", alignment);
		ImGui::SameLine();
		if (ImGui::InputText("##name", &name, ImGuiInputTextFlags_EnterReturnsTrue, NULL, NULL))
		{
			if (!plotHandler->checkIfPlotExists(name))
			{
				std::string oldName = editedPlot->getName();
				plotHandler->renamePlot(oldName, name);
				plotGroupHandler->renamePlotInAllGroups(oldName, name);
			}
			else
				popup.show("Error!", "Plot already exists!", 1.5f);
		}

		ImGui::Dummy(ImVec2(-1, 5));

		static int plotType = 0;
		ImGui::RadioButton("Time Plot", &plotType, 0);
		ImGui::SameLine();
		ImGui::RadioButton("XY Plot", &plotType, 1);

		if (plotType == 0)
		{
		}
		else if (plotType == 1)
		{
			ImGui::Dummy(ImVec2(-1, 5));
			GuiHelper::drawTextAlignedToSize("X-axis variable:", alignment);
			ImGui::SameLine();
			std::string selectedVariable = selection.empty() ? "" : *selection.begin();
			ImGui::InputText("##", &selectedVariable, 0, NULL, NULL);
			ImGui::SameLine();
			if (ImGui::Button("select...", ImVec2(65 * GuiHelper::contentScale, 19 * GuiHelper::contentScale)))
				selectVariableWindow->setShowState(true);
		}
	}

   private:
	/**
	 * @brief Text alignemnt in front of the input fields
	 *
	 */
	static constexpr size_t alignment = 18;

	bool showPlotEditWindow = false;

	std::shared_ptr<Plot> editedPlot = nullptr;

	PlotHandler* plotHandler;
	PlotGroupHandler* plotGroupHandler;
	std::map<std::string, std::shared_ptr<Variable>>* vars;

	Popup popup;

	std::set<std::string> selection;
	std::unique_ptr<SelectVariableWindow> selectVariableWindow;
};

#endif