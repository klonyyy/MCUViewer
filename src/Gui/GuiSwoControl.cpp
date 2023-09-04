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
	drawInputText("##level", settings.triggerLevel, [&](std::string str)
				  { settings.triggerLevel = std::stod(str); });

	if (state != PlotHandlerBase::state::STOP)
		ImGui::EndDisabled();
	else
		tracePlotHandler->setSettings(settings);
}
void Gui::drawIndicatorsSwo()
{
	ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Plots").x) * 0.5f);
	ImGui::Text("Indicators");
	ImGui::Separator();

	for (auto& [name, value] : tracePlotHandler->getTraceIndicators())
	{
		if (name == std::string("error frames in view") && value > 0)
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), name.c_str());
		else if (name == std::string("delayed timestamp 3") && value > 0)
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), name.c_str());
		else
			ImGui::Text(name.c_str());

		ImGui::SameLine();
		std::string separator = "";

		for (uint8_t i = 0; i < 24 - name.size(); i++)
			separator.append(" ");

		ImGui::Text((separator + std::to_string(value)).c_str());
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

	auto state = tracePlotHandler->getViewerState();

	for (std::shared_ptr<Plot> plt : *tracePlotHandler)
	{
		std::string name = plt->getName();
		std::string alias = plt->getAlias();

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
	ImGui::Text("markers    ");
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