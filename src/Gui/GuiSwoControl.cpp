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

	TracePlotHandler::Settings settings = tracePlotHandler->getSettings();

	ImGui::Text("Core frequency [kHz]   ");
	ImGui::SameLine();

	drawInputText("frequency", settings.coreFrequency, [&](std::string str)
				  {settings.coreFrequency = std::stoi(str);
	tracePlotHandler->setSettings(settings); });

	ImGui::Text("Trace prescaler        ");
	ImGui::SameLine();
	drawInputText("prescaler", settings.tracePrescaler, [&](std::string str)
				  {settings.tracePrescaler = std::stoi(str);
	tracePlotHandler->setSettings(settings); });
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

	ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Plots").x) * 0.5f);
	ImGui::Text("Channels");
	ImGui::Separator();

	ImGui::BeginChild("Plot Tree", ImVec2(-1, windowHeight));
	ImGui::BeginChild("left pane", ImVec2(150, -1), true);

	for (std::shared_ptr<Plot> plt : *tracePlotHandler)
	{
		std::string name = plt->getName();
		std::string alias = plt->getAlias();
		ImGui::Checkbox(std::string("##" + name).c_str(), &plt->getVisibilityVar());
		ImGui::SameLine();
		if (ImGui::Selectable((name + " \"" + alias + "\"").c_str(), selected == name))
			selected = name;

		if (plt->isHovered() && ImGui::IsMouseClicked(0))
			selected = plt->getName();
	}

	ImGui::EndChild();
	ImGui::SameLine();

	std::shared_ptr<Plot> plt = tracePlotHandler->getPlot(selected);
	const char* plotDomains[2] = {"analog", "digital"};
	int32_t domainCombo = (int32_t)plt->getDomain();
	std::string newAlias = plt->getAlias();
	ImGui::BeginGroup();
	ImGui::Text("alias      ");
	ImGui::SameLine();
	ImGui::PushID(plt->getAlias().c_str());
	ImGui::InputText("##input", &newAlias, 0, NULL, NULL);
	ImGui::Text("domain     ");
	ImGui::SameLine();
	ImGui::Combo("##combo", &domainCombo, plotDomains, IM_ARRAYSIZE(plotDomains));
	bool mx0 = (tracePlotHandler->getViewerState() == PlotHandlerBase::state::RUN) ? false : plt->getMarkerStateX0();
	ImGui::Text("markers");
	ImGui::SameLine();
	ImGui::Checkbox("##mx0", &mx0);
	plt->setMarkerStateX0(mx0);
	plt->setMarkerStateX1(mx0);
	ImGui::PopID();
	ImGui::EndGroup();
	ImGui::EndChild();

	if (domainCombo != (int32_t)plt->getDomain())
		plt->setDomain(static_cast<Plot::Domain>(domainCombo));

	if ((ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) && newAlias != plt->getAlias())
	{
		plt->setAlias(newAlias);
	}
}

void Gui::drawInputText(const char* id, uint32_t variable, std::function<void(std::string)> valueChanged)
{
	std::string str = std::to_string(variable);

	ImGui::InputText(id, &str, 0, NULL, NULL);

	if ((ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) && str != std::to_string(variable))
	{
		logger->info(str);
		if (valueChanged)
			valueChanged(str);
	}
}