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

	if (ImGui::Button((viewerStateMap.at(state) + " " + tracePlotHandler->getLastReaderError()).c_str(), ImVec2(-1, 50)))
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
	ImGui::Dummy(ImVec2(-1, 5));
	ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Plots").x) * 0.5f);
	ImGui::Text("Settings");
	ImGui::Separator();

	auto settings = tracePlotHandler->getSettings();
	auto state = tracePlotHandler->getViewerState();

	if (state == PlotHandlerBase::state::RUN)
		ImGui::BeginDisabled();

	ImGui::Text("core frequency [kHz]   ");
	ImGui::SameLine();

	drawInputText("##frequency", settings.coreFrequency, [&](std::string str) { settings.coreFrequency = std::stoi(str); });

	ImGui::Text("trace prescaler        ");
	ImGui::SameLine();
	drawInputText("##prescaler", settings.tracePrescaler, [&](std::string str) { settings.tracePrescaler = std::stoi(str); });

	const char* triggers[] = {"OFF", "CH0", "CH1", "CH2", "CH3", "CH4", "CH5", "CH6", "CH7", "CH8", "CH9"};
	int32_t trigerCombo = settings.triggerChannel + 1;
	ImGui::Text("trigger channel        ");
	ImGui::SameLine();
	if (ImGui::Combo("##trigger", &trigerCombo, triggers, IM_ARRAYSIZE(triggers)))
		settings.triggerChannel = trigerCombo - 1;

	ImGui::Text("trigger level          ");
	ImGui::SameLine();

	for (auto plt : *tracePlotHandler)
		if (plt->trigger.getState())
			settings.triggerLevel = plt->trigger.getValue();

	drawInputText("##level", settings.triggerLevel, [&](std::string str) { settings.triggerLevel = std::stod(str); });

	if (state != PlotHandlerBase::state::STOP)
		ImGui::EndDisabled();
	else
		tracePlotHandler->setSettings(settings);
}
void Gui::drawIndicatorsSwo()
{
	ImGui::Dummy(ImVec2(-1, 5));
	ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Plots").x) * 0.5f);
	ImGui::Text("Indicators");
	ImGui::SameLine();
	ImGui::HelpMarker("Indicators help to ascess the quality of trace waveforms. Look out for red indicators that tell you a frame might be misinterpreted. In such cases try to increase the trace prescaler or limit the ative trace channels.");
	ImGui::Separator();

	auto indicators = tracePlotHandler->getTraceIndicators();

	ImGui::Text("frames total:           ");
	ImGui::SameLine();
	ImGui::Text("%s", (std::to_string(indicators.framesTotal)).c_str());

	ImGui::Text("sleep cycles:           ");
	ImGui::SameLine();
	ImGui::Text("%s", (std::to_string(indicators.sleepCycles)).c_str());

	ImGui::Text("error frames total:     ");
	ImGui::SameLine();
	ImGui::Text("%s", (std::to_string(indicators.errorFramesTotal)).c_str());

	const char* inView = "error frames in view:   ";
	if (indicators.errorFramesInView > 0)
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", inView);
	else
		ImGui::Text("%s", inView);
	ImGui::SameLine();
	ImGui::Text("%s", (std::to_string(indicators.errorFramesInView)).c_str());

	ImGui::Text("delayed timestamp 1:    ");
	ImGui::SameLine();
	ImGui::Text("%s", (std::to_string(indicators.delayedTimestamp1)).c_str());
	ImGui::Text("delayed timestamp 2:    ");
	ImGui::SameLine();
	ImGui::Text("%s", (std::to_string(indicators.delayedTimestamp2)).c_str());

	const char* timestampDelayed3 = "delayed timestamp 3:    ";
	if (indicators.delayedTimestamp3 > 0)
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", timestampDelayed3);
	else
		ImGui::Text("%s", timestampDelayed3);
	ImGui::SameLine();
	ImGui::Text("%s", (std::to_string(indicators.delayedTimestamp3)).c_str());
}

void Gui::drawPlotsTreeSwo()
{
	const uint32_t windowHeight = 320;
	static std::string selected = tracePlotHandler->begin().operator*()->getName();

	ImGui::Dummy(ImVec2(-1, 5));
	ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Plots").x) * 0.5f);
	ImGui::Text("Channels");
	ImGui::SameLine();
	ImGui::HelpMarker("Uncheck a channel to disable it and free some of the SWO pin bandwidth.");
	ImGui::Separator();

	ImGui::BeginChild("Plot Tree", ImVec2(-1, windowHeight));
	ImGui::BeginChild("left pane", ImVec2(150, -1), true);

	auto state = tracePlotHandler->getViewerState();
	int32_t iter = 0;

	for (std::shared_ptr<Plot> plt : *tracePlotHandler)
	{
		std::string name = plt->getName();
		std::string alias = plt->getAlias();

		plt->trigger.setState(tracePlotHandler->getSettings().triggerChannel == iter++);

		if (state == PlotHandlerBase::state::RUN)
			ImGui::BeginDisabled();
		ImGui::Checkbox(std::string("##" + name).c_str(), &plt->getVisibilityVar());
		if (state == PlotHandlerBase::state::RUN)
			ImGui::EndDisabled();

		ImGui::SameLine();
		if (ImGui::Selectable((name + " \"" + alias + "\"").c_str(), selected == name))
			selected = name;

		if (plt->isHovered() && ImGui::IsMouseClicked(0))
			selected = plt->getName();
	}

	ImGui::EndChild();
	ImGui::SameLine();

	std::shared_ptr<Plot> plt = tracePlotHandler->getPlot(selected);
	const char* plotDomains[] = {"analog", "digital"};
	const char* traceVarTypes[] = {"uint8_t", "int8_t", "uint16_t", "int16_t", "uint32_t", "int32_t", "float"};
	int32_t domainCombo = (int32_t)plt->getDomain();
	int32_t traceVarTypeCombo = (int32_t)plt->getTraceVarType();
	std::string newAlias = plt->getAlias();
	ImGui::BeginGroup();
	ImGui::Text("alias      ");
	ImGui::SameLine();
	ImGui::PushID(plt->getAlias().c_str());
	ImGui::InputText("##input", &newAlias, 0, NULL, NULL);
	ImGui::Text("domain     ");
	ImGui::SameLine();
	ImGui::Combo("##combo", &domainCombo, plotDomains, IM_ARRAYSIZE(plotDomains));
	if (domainCombo == static_cast<int32_t>(Plot::Domain::ANALOG))
	{
		ImGui::Text("type       ");
		ImGui::SameLine();
		ImGui::Combo("##combo2", &traceVarTypeCombo, traceVarTypes, IM_ARRAYSIZE(traceVarTypes));
	}
	bool mx0 = (tracePlotHandler->getViewerState() == PlotHandlerBase::state::RUN) ? false : plt->markerX0.getState();
	ImGui::Text("markers    ");
	ImGui::SameLine();
	ImGui::Checkbox("##mx0", &mx0);
	plt->markerX0.setState(mx0);
	plt->markerX1.setState(mx0);
	ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetWindowSize().y - 25 / 2.0f - ImGui::GetFrameHeightWithSpacing()));
	drawExportPlotToCSVButton(plt);
	ImGui::PopID();
	ImGui::EndGroup();
	ImGui::EndChild();

	if (domainCombo != (int32_t)plt->getDomain())
		plt->setDomain(static_cast<Plot::Domain>(domainCombo));

	if ((traceVarTypeCombo) != (int32_t)plt->getTraceVarType())
		plt->setTraceVarType(static_cast<Plot::TraceVarType>(traceVarTypeCombo));

	if ((ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) && newAlias != plt->getAlias())
	{
		plt->setAlias(newAlias);
	}
}