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

bool JLinkTraceDevice::startTrace(uint32_t coreFrequency, uint32_t tracePrescaler, uint32_t activeChannelMask, bool shouldReset)
{
	logger->info("Starting reader thread!");
	return true;
}

int32_t JLinkTraceDevice::readTraceBuffer(uint8_t* buffer, uint32_t size)
{
	return -1;
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