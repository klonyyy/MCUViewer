#include "Gui.hpp"

static constexpr size_t alignment = 30;

void Gui::acqusitionSettingsViewer()
{
	ImGui::Dummy(ImVec2(-1, 5));
	drawCenteredText("Project");
	ImGui::Separator();

	drawTextAlignedToSize("*.elf file:", alignment);
	ImGui::SameLine();
	ImGui::InputText("##", &projectElfPath, 0, NULL, NULL);
	ImGui::SameLine();
	if (ImGui::Button("...", ImVec2(35 * contentScale, 19 * contentScale)))
		openElfFile();

	PlotHandler::Settings settings = plotHandler->getSettings();

	drawTextAlignedToSize("Refresh vars on *.elf change:", alignment);
	ImGui::SameLine();
	ImGui::Checkbox("##refresh", &settings.refreshAddressesOnElfChange);

	drawTextAlignedToSize("Stop on *.elf change:", alignment);
	ImGui::SameLine();
	ImGui::Checkbox("##stop", &settings.stopAcqusitionOnElfChange);

	drawTextAlignedToSize("Sampling [Hz]:", alignment);
	ImGui::SameLine();
	ImGui::InputScalar("##sample", ImGuiDataType_U32, &settings.sampleFrequencyHz, NULL, NULL, "%u");
	ImGui::SameLine();
	ImGui::HelpMarker("Maximum sampling frequency. Depending on the used debug probe it can be reached or not.");
	settings.sampleFrequencyHz = std::clamp(settings.sampleFrequencyHz, PlotHandler::minSamplinFrequencyHz, PlotHandler::maxSamplinFrequencyHz);

	const uint32_t minPoints = 100;
	const uint32_t maxPoints = 20000;
	drawTextAlignedToSize("Max points:", alignment);
	ImGui::SameLine();
	ImGui::InputScalar("##maxPoints", ImGuiDataType_U32, &settings.maxPoints, NULL, NULL, "%u");
	ImGui::SameLine();
	ImGui::HelpMarker("Max points used for a single series after which the oldest points will be overwritten.");
	settings.maxPoints = std::clamp(settings.maxPoints, minPoints, maxPoints);

	drawTextAlignedToSize("Max view points:", alignment);
	ImGui::SameLine();
	ImGui::InputScalar("##maxViewportPoints", ImGuiDataType_U32, &settings.maxViewportPoints, NULL, NULL, "%u");
	ImGui::SameLine();
	ImGui::HelpMarker("Max points used for a single series that will be shown in the viewport without scroling.");
	settings.maxViewportPoints = std::clamp(settings.maxViewportPoints, minPoints, settings.maxPoints);

	drawDebugProbes();
	drawLoggingSettings(plotHandler, settings);
	plotHandler->setSettings(settings);
}

void Gui::drawDebugProbes()
{
	static bool shouldListDevices = false;
	static int SNptr = 0;
	bool modified = false;

	ImGui::PushID("DebugProbes");
	ImGui::Dummy(ImVec2(-1, 5));
	drawCenteredText("Debug Probe");
	ImGui::SameLine();
	ImGui::HelpMarker("Select the debug probe type and the serial number of the probe to unlock the START button.");
	ImGui::Separator();

	drawTextAlignedToSize("Debug probe:", alignment);
	ImGui::SameLine();

	const char* debugProbes[] = {"STLINK", "JLINK"};
	IDebugProbe::DebugProbeSettings probeSettings = plotHandler->getProbeSettings();
	int32_t debugProbe = probeSettings.debugProbe;

	if (ImGui::Combo("##debugProbe", &debugProbe, debugProbes, IM_ARRAYSIZE(debugProbes)))
	{
		probeSettings.debugProbe = debugProbe;
		modified = true;

		if (probeSettings.debugProbe == 1)
		{
			debugProbeDevice = jlinkProbe;
			shouldListDevices = true;
		}
		else
		{
			debugProbeDevice = stlinkProbe;
			shouldListDevices = true;
		}
		SNptr = 0;
	}
	drawTextAlignedToSize("Debug probe S/N:", alignment);
	ImGui::SameLine();

	if (ImGui::Combo("##debugProbeSN", &SNptr, devicesList))
	{
		probeSettings.serialNumber = devicesList[SNptr];
		modified = true;
	}

	ImGui::SameLine();

	if (ImGui::Button("@", ImVec2(35 * contentScale, 19 * contentScale)) || shouldListDevices || devicesList.empty())
	{
		devicesList = debugProbeDevice->getConnectedDevices();
		if (!devicesList.empty())
		{
			probeSettings.serialNumber = devicesList[0];
			std::cout << devicesList.size();
			modified = true;
		}
		shouldListDevices = false;
	}

	drawTextAlignedToSize("SWD speed [kHz]:", alignment);
	ImGui::SameLine();

	if (ImGui::InputScalar("##speed", ImGuiDataType_U32, &probeSettings.speedkHz, NULL, NULL, "%u"))
		modified = true;

	if (probeSettings.debugProbe == 1)
	{
		drawTextAlignedToSize("Target name:", alignment);
		ImGui::SameLine();

		if (ImGui::InputText("##device", &probeSettings.device, 0, NULL, NULL))
			modified = true;

		ImGui::SameLine();
		if (ImGui::Button("...", ImVec2(35 * contentScale, 19 * contentScale)))
		{
			probeSettings.device = debugProbeDevice->getTargetName();
			modified = true;
		}

		drawTextAlignedToSize("Mode:", alignment);
		ImGui::SameLine();

		const char* probeModes[] = {"NORMAL", "HSS"};
		int32_t probeMode = probeSettings.mode;

		if (ImGui::Combo("##mode", &probeMode, probeModes, IM_ARRAYSIZE(probeModes)))
		{
			probeSettings.mode = static_cast<IDebugProbe::Mode>(probeMode);
			modified = true;
		}

		ImGui::SameLine();
		ImGui::HelpMarker("Select normal or high speed sampling (HSS) mode");
	}
	else
		probeSettings.mode = IDebugProbe::Mode::NORMAL;

	if (devicesList.empty())
		devicesList.push_back(noDevices);

	if (modified)
	{
		plotHandler->setProbeSettings(probeSettings);
		plotHandler->setDebugProbe(debugProbeDevice);
	}
	ImGui::PopID();
}

template <typename Settings>
void Gui::drawLoggingSettings(PlotHandlerBase* handler, Settings& settings)
{
	ImGui::PushID("logging");
	ImGui::Dummy(ImVec2(-1, 5));
	drawCenteredText("Logging");
	ImGui::SameLine();
	ImGui::HelpMarker("Log all registered variables values to a selected log file.");
	ImGui::Separator();

	/* CSV streamer */
	drawTextAlignedToSize("Log to file:", alignment);
	ImGui::SameLine();
	ImGui::Checkbox("##logging", &settings.shouldLog);

	ImGui::BeginDisabled(!settings.shouldLog);

	drawTextAlignedToSize("Logfile directory:", alignment);
	ImGui::SameLine();
	ImGui::InputText("##", &settings.logFilePath, 0, NULL, NULL);
	ImGui::SameLine();
	if (ImGui::Button("...", ImVec2(35 * contentScale, 19 * contentScale)))
		openLogDirectory(settings.logFilePath);

	ImGui::EndDisabled();
	ImGui::PopID();
}

void Gui::acqusitionSettingsTrace()
{
	TracePlotHandler::Settings settings = tracePlotHandler->getSettings();

	drawTextAlignedToSize("Max points:", alignment);
	ImGui::SameLine();
	ImGui::InputScalar("##maxPoints", ImGuiDataType_U32, &settings.maxPoints, NULL, NULL, "%u");
	ImGui::SameLine();
	ImGui::HelpMarker("Max points used for a single series after which the oldest points will be overwritten.");
	settings.maxPoints = std::clamp(settings.maxPoints, static_cast<uint32_t>(100), static_cast<uint32_t>(20000));

	drawTextAlignedToSize("Viewport width [%%]:", alignment);
	ImGui::SameLine();
	ImGui::InputScalar("##maxViewportPoints", ImGuiDataType_U32, &settings.maxViewportPointsPercent, NULL, NULL, "%u");
	ImGui::SameLine();
	ImGui::HelpMarker("The percentage of trace time visible during collect. Expressed in percent since the sample period is not constant.");
	settings.maxViewportPointsPercent = std::clamp(settings.maxViewportPointsPercent, static_cast<uint32_t>(1), static_cast<uint32_t>(100));

	drawTextAlignedToSize("Timeout [s]:", alignment);
	ImGui::SameLine();
	ImGui::InputScalar("##timeout", ImGuiDataType_U32, &settings.timeout, NULL, NULL, "%u");
	ImGui::SameLine();
	ImGui::HelpMarker("Timeout is the period after which trace will be stopped due to no trace data being received.");
	settings.timeout = std::clamp(settings.timeout, static_cast<uint32_t>(1), static_cast<uint32_t>(999999));

	drawTraceProbes();
	drawLoggingSettings(tracePlotHandler, settings);
	tracePlotHandler->setSettings(settings);
}

void Gui::drawTraceProbes()
{
	static bool shouldListDevices = false;
	static int SNptr = 0;
	bool modified = false;

	ImGui::PushID("DebugProbes");
	ImGui::Dummy(ImVec2(-1, 5));
	drawCenteredText("Debug Probe");
	ImGui::SameLine();
	ImGui::HelpMarker("Select the debug probe type and the serial number of the probe to unlock the START button.");
	ImGui::Separator();

	drawTextAlignedToSize("Debug probe:", alignment);
	ImGui::SameLine();

	const char* debugProbes[] = {"STLINK", "JLINK"};
	ITraceProbe::TraceProbeSettings probeSettings = tracePlotHandler->getProbeSettings();
	int32_t debugProbe = probeSettings.debugProbe;

	if (ImGui::Combo("##debugProbe", &debugProbe, debugProbes, IM_ARRAYSIZE(debugProbes)))
	{
		probeSettings.debugProbe = debugProbe;
		modified = true;

		if (probeSettings.debugProbe == 1)
		{
			traceProbeDevice = jlinkTraceProbe;
			shouldListDevices = true;
		}
		else
		{
			traceProbeDevice = stlinkTraceProbe;
			shouldListDevices = true;
		}
		SNptr = 0;
	}
	drawTextAlignedToSize("Debug probe S/N:", alignment);
	ImGui::SameLine();

	if (ImGui::Combo("##debugProbeSN", &SNptr, devicesList))
	{
		probeSettings.serialNumber = devicesList[SNptr];
		modified = true;
	}

	ImGui::SameLine();

	if (ImGui::Button("@", ImVec2(35 * contentScale, 19 * contentScale)) || shouldListDevices || devicesList.empty())
	{
		devicesList = traceProbeDevice->getConnectedDevices();
		if (!devicesList.empty())
		{
			probeSettings.serialNumber = devicesList[0];
			std::cout << devicesList.size();
			modified = true;
		}
		shouldListDevices = false;
	}

	drawTextAlignedToSize("SWD speed [kHz]:", alignment);
	ImGui::SameLine();

	if (ImGui::InputScalar("##speed", ImGuiDataType_U32, &probeSettings.speedkHz, NULL, NULL, "%u"))
		modified = true;

	if (probeSettings.debugProbe == 1)
	{
		drawTextAlignedToSize("Target name:", alignment);
		ImGui::SameLine();

		if (ImGui::InputText("##device", &probeSettings.device, 0, NULL, NULL))
			modified = true;

		ImGui::SameLine();
		if (ImGui::Button("...", ImVec2(35 * contentScale, 19 * contentScale)))
		{
			probeSettings.device = traceProbeDevice->getTargetName();
			modified = true;
		}
	}

	if (devicesList.empty())
		devicesList.push_back(noDevices);

	if (modified)
	{
		tracePlotHandler->setProbeSettings(probeSettings);
		tracePlotHandler->setDebugProbe(traceProbeDevice);
	}
	ImGui::PopID();
}