#include "Gui.hpp"

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

	drawInputText("##frequency", settings.coreFrequency, [&](std::string str)
				  { settings.coreFrequency = std::stoi(str); });

	ImGui::Text("trace prescaler        ");
	ImGui::SameLine();
	drawInputText("##prescaler", settings.tracePrescaler, [&](std::string str)
				  { settings.tracePrescaler = std::stoi(str); });

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

	drawInputText("##level", settings.triggerLevel, [&](std::string str)
				  { settings.triggerLevel = std::stod(str); });

	static bool shouldReset = false;
	ImGui::Text("should reset           ");
	ImGui::SameLine();
	ImGui::Checkbox("##rst", &shouldReset);
	settings.shouldReset = shouldReset;

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
	ImGui::HelpMarker("Indicators help to ascess the quality of trace waveforms. Look out for red indicators that tell you a frame might be misinterpreted. In such cases try to increase the trace prescaler or limit the number of active trace channels.");
	ImGui::Separator();

	auto indicators = tracePlotHandler->getTraceIndicators();

	drawDescriptionWithNumber("frames total:           ", indicators.framesTotal);
	drawDescriptionWithNumber("sleep cycles:           ", indicators.sleepCycles);
	drawDescriptionWithNumber("error frames total:     ", indicators.errorFramesTotal);
	drawDescriptionWithNumber("error frames in view:   ", indicators.errorFramesInView, "", 5, 0, {1, 0, 0, 1});
	drawDescriptionWithNumber("delayed timestamp 1:    ", indicators.delayedTimestamp1, "", 5, 0, {1, 1, 0, 1});
	drawDescriptionWithNumber("delayed timestamp 2:    ", indicators.delayedTimestamp2, "", 5, 0, {1, 1, 0, 1});
	drawDescriptionWithNumber("delayed timestamp 3:    ", indicators.delayedTimestamp3);
	drawDescriptionWithNumber("delayed timestamp 3 in view:    ", indicators.delayedTimestamp3InView, "", 5, 0, {1, 0, 0, 1});
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
	ImGui::BeginChild("left pane", ImVec2(150 * contentScale, -1), true);

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
	int32_t domainCombo = (int32_t)plt->getDomain();
	int32_t traceVarTypeCombo = (int32_t)plt->getTraceVarType();
	std::string newAlias = plt->getAlias();
	ImGui::BeginGroup();
	ImGui::Text("alias      ");
	ImGui::SameLine();
	ImGui::PushID(plt->getAlias().c_str());
	ImGui::InputText("##input", &newAlias, ImGuiInputTextFlags_EnterReturnsTrue, NULL, NULL);
	ImGui::Text("domain     ");
	ImGui::SameLine();
	ImGui::Combo("##combo", &domainCombo, plotDomains, IM_ARRAYSIZE(plotDomains));
	if (domainCombo == static_cast<int32_t>(Plot::Domain::ANALOG))
	{
		const char* traceVarTypes[] = {"uint8_t", "int8_t", "uint16_t", "int16_t", "uint32_t", "int32_t", "float"};
		ImGui::Text("type       ");
		ImGui::SameLine();
		ImGui::Combo("##combo2", &traceVarTypeCombo, traceVarTypes, IM_ARRAYSIZE(traceVarTypes));
		drawStatisticsAnalog(plt);
	}
	else
		drawStatisticsDigital(plt);

	bool mx0 = (tracePlotHandler->getViewerState() == PlotHandlerBase::state::RUN) ? false : plt->markerX0.getState();
	ImGui::Text("markers    ");
	ImGui::SameLine();
	ImGui::Checkbox("##mx0", &mx0);
	plt->markerX0.setState(mx0);
	plt->markerX1.setState(mx0);
	ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetWindowSize().y - 25 * contentScale / 2.0f - ImGui::GetFrameHeightWithSpacing()));
	drawExportPlotToCSVButton(plt);
	ImGui::PopID();
	ImGui::EndGroup();
	ImGui::EndChild();

	if (domainCombo != (int32_t)plt->getDomain())
		plt->setDomain(static_cast<Plot::Domain>(domainCombo));

	if ((traceVarTypeCombo) != (int32_t)plt->getTraceVarType())
		plt->setTraceVarType(static_cast<Plot::TraceVarType>(traceVarTypeCombo));

	if ((ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter) || ImGui::IsMouseClicked(0)) && newAlias != plt->getAlias())
	{
		plt->setAlias(newAlias);
	}
}
