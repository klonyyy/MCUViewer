#include "Gui.hpp"

void Gui::drawStartButtonSwo()
{
	TracePlotHandler::state state = tracePlotHandler->getViewerState();

	if (state == TracePlotHandler::state::RUN)
	{
		ImVec4 color = (ImVec4)ImColor::HSV(0.365f, 0.94f, 0.37f);
		ImGui::PushStyleColor(ImGuiCol_Button, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
	}
	else if (state == TracePlotHandler::state::STOP)
	{
		ImVec4 color = ImColor::HSV(0.116f, 0.97f, 0.72f);

		// if (tracePlotHandler->getLastReaderError() != "")
		// 	color = ImColor::HSV(0.0f, 0.95f, 0.70f);
		ImGui::PushStyleColor(ImGuiCol_Button, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
	}

	if (ImGui::Button(traceReaderStateMap.at(state).c_str(), ImVec2(-1, 50)))
	{
		if (state == TracePlotHandler::state::STOP)
		{
			tracePlotHandler->eraseAllPlotData();
			tracePlotHandler->setViewerState(TracePlotHandler::state::RUN);
		}
		else
			tracePlotHandler->setViewerState(TracePlotHandler::state::STOP);
	}

	ImGui::PopStyleColor(3);
}