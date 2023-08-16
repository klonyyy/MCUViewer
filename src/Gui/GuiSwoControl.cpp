#include "Gui.hpp"

void Gui::drawStartButtonSwo()
{
	PlotHandlerBase::state state = tracePlotHandler->getViewerState();

	if (state == PlotHandlerBase::state::RUN)
	{
		ImVec4 color = (ImVec4)ImColor::HSV(0.365f, 0.94f, 0.37f);
		ImGui::PushStyleColor(ImGuiCol_Button, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
	}
	else if (state == PlotHandlerBase::state::STOP)
	{
		ImVec4 color = ImColor::HSV(0.116f, 0.97f, 0.72f);

		if (tracePlotHandler->getLastReaderError() != "")
			color = ImColor::HSV(0.0f, 0.95f, 0.70f);
		ImGui::PushStyleColor(ImGuiCol_Button, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
	}

	if (ImGui::Button(viewerStateMap.at(state).c_str(), ImVec2(-1, 50)))
	{
		if (state == PlotHandlerBase::state::STOP)
		{
			logger->info("Start trace clicked!");
			tracePlotHandler->eraseAllPlotData();
			tracePlotHandler->setViewerState(PlotHandlerBase::state::RUN);
		}
		else
		{
			logger->info("Stop trace clicked!");
			tracePlotHandler->setViewerState(PlotHandlerBase::state::STOP);
		}
	}

	ImGui::PopStyleColor(3);
}

void Gui::drawSettingsSwo()
{
	ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Plots").x) * 0.5f);
	ImGui::Text("Settings");
	ImGui::Separator();

	TracePlotHandler::TraceSettings settings = tracePlotHandler->getTraceSettings();

	ImGui::Text("Core Frequency [kHz]      ");
	ImGui::SameLine();

	drawInputText(settings.coreFrequency, [&](std::string str)
				  {settings.coreFrequency = std::stoi(str);
	tracePlotHandler->setTraceSettings(settings); });

	ImGui::Text("Trace Frequency [kHz]     ");
	ImGui::SameLine();
	drawInputText(settings.traceFrequency, [&](std::string str)
				  {settings.traceFrequency = std::stoi(str);
	tracePlotHandler->setTraceSettings(settings); });
}
void Gui::drawIndicatorsSwo()
{
	ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Plots").x) * 0.5f);
	ImGui::Text("Indicators");
	ImGui::Separator();

	std::map<const char*, uint32_t> traceQuality = tracePlotHandler->getTraceIndicators();

	for (auto& [name, value] : traceQuality)
	{
		ImGui::Text(name);
		ImGui::SameLine();
		ImGui::Text(std::to_string(value).c_str());
	}
}

void Gui::drawPlotsTreeSwo()
{
	const uint32_t windowHeight = 320;
	static std::string selected = tracePlotHandler->begin().operator*()->getName();
	;

	ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Plots").x) * 0.5f);
	ImGui::Text("Channels");
	ImGui::Separator();

	// if (!tracePlotHandler->checkIfPlotExists(std::move(selected)))
	// 	selected = tracePlotHandler->begin().operator*()->getName();

	ImGui::BeginChild("Plot Tree", ImVec2(-1, windowHeight));
	ImGui::BeginChild("left pane", ImVec2(150, -1), true);

	for (std::shared_ptr<Plot> plt : *tracePlotHandler)
	{
		std::string name = plt->getName();
		ImGui::Checkbox(std::string("##" + name).c_str(), &plt->getVisibilityVar());
		ImGui::SameLine();
		if (ImGui::Selectable(name.c_str(), selected == name))
			selected = name;

		if (plt->isHovered() && ImGui::IsMouseClicked(0))
			selected = plt->getName();
	}

	ImGui::EndChild();
	ImGui::SameLine();

	std::shared_ptr<Plot> plt = tracePlotHandler->getPlot(selected);
	std::string newName = plt->getName();
	ImGui::BeginGroup();
	ImGui::Text("name      ");
	ImGui::SameLine();
	ImGui::PushID(plt->getName().c_str());
	ImGui::InputText("##input", &newName, 0, NULL, NULL);
	bool mx0 = (tracePlotHandler->getViewerState() == PlotHandlerBase::state::RUN) ? false : plt->getMarkerStateX0();
	ImGui::Text("markers");
	ImGui::SameLine();
	ImGui::Checkbox("##mx0", &mx0);
	plt->setMarkerStateX0(mx0);
	plt->setMarkerStateX1(mx0);
	ImGui::PopID();
	ImGui::EndGroup();
	ImGui::EndChild();

	if ((ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) && newName != plt->getName())
	{
		tracePlotHandler->renamePlot(plt->getName(), newName);
		selected = newName;
	}
}

void Gui::drawInputText(uint32_t variable, std::function<void(std::string)> valueChanged)
{
	std::string str = std::to_string(variable);

	ImGui::InputText(str.c_str(), &str, 0, NULL, NULL);

	if ((ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) && str != std::to_string(variable))
	{
		logger->info(str);
		if (valueChanged)
			valueChanged(str);
	}
}