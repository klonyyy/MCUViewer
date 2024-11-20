#ifndef _GUI_PLOTEDIT_HPP
#define _GUI_PLOTEDIT_HPP

#include "GuiHelper.hpp"
#include "GuiSelectVariable.hpp"
#include "Plot.hpp"
#include "PlotGroupHandler.hpp"
#include "Popup.hpp"
#include "imgui.h"

class PlotEditWindow
{
   public:
	PlotEditWindow(PlotHandler* plotHandler, PlotGroupHandler* plotGroupHandler, VariableHandler* variableHandler) : plotHandler(plotHandler), plotGroupHandler(plotGroupHandler), variableHandler(variableHandler)
	{
		selectVariableWindow = std::make_unique<SelectVariableWindow>(variableHandler, &selection, 1);
	}

	void draw()
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

			if (ImGui::Button("Done", ImVec2(-1, buttonHeight)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
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
		if (showPlotEditWindow != state)
			stateChanged = true;
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

		if (stateChanged)
		{
			ImGui::SetKeyboardFocusHere(0);
			stateChanged = false;
		}

		ImGui::InputText("##name", &name, ImGuiInputTextFlags_None, NULL, NULL);

		if (ImGui::IsItemDeactivatedAfterEdit())
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

		const char* plotTypes[] = {"curve", "bar", "table", "XY"};
		int32_t typeCombo = (int32_t)editedPlot->getType();
		GuiHelper::drawTextAlignedToSize("type:", alignment);
		ImGui::SameLine();
		if (ImGui::Combo("##combo", &typeCombo, plotTypes, IM_ARRAYSIZE(plotTypes)))
			editedPlot->setType((Plot::Type)typeCombo);

		if (editedPlot->getType() == Plot::Type::XY)
		{
			GuiHelper::drawTextAlignedToSize("X-axis variable:", alignment);
			ImGui::SameLine();
			std::string selectedVariable = selection.empty() ? "" : *selection.begin();
			ImGui::InputText("##", &selectedVariable, 0, NULL, NULL);
			if (variableHandler->contains(selectedVariable))
				editedPlot->setXAxisVariable(variableHandler->getVariable(selectedVariable).get());
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
	bool stateChanged = false;

	std::shared_ptr<Plot> editedPlot = nullptr;

	PlotHandler* plotHandler;
	PlotGroupHandler* plotGroupHandler;
	VariableHandler* variableHandler;

	Popup popup;

	std::set<std::string> selection;
	std::unique_ptr<SelectVariableWindow> selectVariableWindow;
};

#endif