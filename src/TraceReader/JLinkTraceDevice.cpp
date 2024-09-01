#include "JLinkTraceDevice.hpp"

#include <cstring>
#include <random>

#include "logging.h"

JLinkTraceDevice::JLinkTraceDevice(spdlog::logger* logger) : logger(logger)
{
}

bool JLinkTraceDevice::stopTrace()
{
	JLINKARM_Close();
	logger->info("Trace stopped.");
	return true;
}

bool JLinkTraceDevice::startTrace(const TraceProbeSettings& probeSettings, uint32_t coreFrequency, uint32_t tracePrescaler, uint32_t activeChannelMask, bool shouldReset)
{
	int32_t serialNumberInt = std::atoi(probeSettings.serialNumber.c_str());
	std::string lastErrorMsg = "";

	if (JLINKARM_EMU_SelectByUSBSN(serialNumberInt) < 0)
	{
		lastErrorMsg = "Could not connect to the selected probe";
		return false;
	}

	const char* error = JLINKARM_Open();

	if (error != 0)
		logger->error(error);

	/* try to set maximum possible speed TODO: not always a good thing */
	JLINKARM_SetSpeed(probeSettings.speedkHz > maxSpeedkHz ? maxSpeedkHz : probeSettings.speedkHz);
	logger->info("J-Link speed set to: {}", JLINKARM_GetSpeed());

	/* select interface - SWD only for now */
	JLINKARM_TIF_Select(JLINKARM_TIF_SWD);

	/* set the desired target */
	char acOut[256];
	auto deviceCmd = "Device =" + probeSettings.device;
	JLINKARM_ExecCommand(deviceCmd.c_str(), acOut, sizeof(acOut));

	if (acOut[0] != 0)
		logger->error(acOut);

	/* try to connect to target */
	if (JLINKARM_Connect() < 0)
	{
		lastErrorMsg = "Could not connect to the target!";
		logger->error(lastErrorMsg);
		JLINKARM_Close();
		return false;
	}

	/* turn on relative timestamping */
	JLINKARM_SWO_Config("TSEnable=1");

	/* calculate SWO speed */
	const uint32_t traceFrequency = coreFrequency / (tracePrescaler + 1);
	int32_t result = JLINKARM_SWO_EnableTarget(coreFrequency, traceFrequency, JLINKARM_SWO_IF_UART, activeChannelMask);

	if (result == 0)
	{
		logger->info("Starting Jlink reader thread!");
		return true;
	}

	logger->info("Error starting Jlink reader thread! Error code {}", result);
	return true;
}

int32_t JLinkTraceDevice::readTraceBuffer(uint8_t* buffer, uint32_t size)
{
	/* TODO error handling of these two functions? */
	JLINKARM_SWO_Read(buffer, 0, &size);
	JLINKARM_SWO_Control(JLINKARM_SWO_CMD_FLUSH, &size);
	return size;
}

std::string JLinkTraceDevice::getTargetName()
{
	JLINKARM_DEVICE_SELECT_INFO info;
	info.SizeOfStruct = sizeof(JLINKARM_DEVICE_SELECT_INFO);
	int32_t index = JLINKARM_DEVICE_SelectDialog(NULL, 0, &info);

	JLINKARM_DEVICE_INFO devInfo{};
	devInfo.SizeOfStruct = sizeof(JLINKARM_DEVICE_INFO);
	JLINKARM_DEVICE_GetInfo(index, &devInfo);

	return devInfo.sName ? std::string(devInfo.sName) : std::string();
}

std::vector<std::string> JLinkTraceDevice::getConnectedDevices()
{
	std::vector<std::string> deviceIDs{};

	JLINKARM_EMU_CONNECT_INFO connectInfo[maxDevices]{};
	int32_t result = JLINKARM_EMU_GetList(1, (JLINKARM_EMU_CONNECT_INFO*)connectInfo, maxDevices);

	if (result < 0)
	{
		logger->error("Error reading JLink devices list. Error code {}", result);
		return std::vector<std::string>{};
	}

	for (size_t i = 0; i < maxDevices; i++)
	{
		auto serialNumber = connectInfo[i].SerialNumber;

		if (serialNumber != 0)
		{
			logger->info("JLink serial number {}", serialNumber);
			deviceIDs.push_back(std::to_string(serialNumber));
		}
	}

	return deviceIDs;
}