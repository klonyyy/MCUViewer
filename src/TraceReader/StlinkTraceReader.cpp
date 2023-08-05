#include "StlinkTraceReader.hpp"

#include <cstring>
#include <random>

#include "logging.h"

StlinkTraceReader::StlinkTraceReader()
{
	init_chipids("./chips");
}

bool StlinkTraceReader::startAcqusition()
{
	// sl = stlink_open_usb(UINFO, CONNECT_HOT_PLUG, NULL, 24000);
	// isRunning = false;

	// if (sl != nullptr)
	// {
	// 	if (stlink_enter_swd_mode(sl) != 0 || stlink_target_connect(sl, CONNECT_HOT_PLUG) != 0)
	// 	{
	// 		stopAcqusition();
	// 		lastErrorMsg = "STM32 target not found!";
	// 		return false;
	// 	}

	isRunning = true;
	// lastErrorMsg = "";
	return true;
	// }
	// lastErrorMsg = "STLink not found!";
	// return false;
}
bool StlinkTraceReader::stopAcqusition()
{
	isRunning = false;
	// stlink_close(sl);
	return true;
}

bool StlinkTraceReader::isValid() const
{
	return isRunning;
}

bool StlinkTraceReader::readTrace(double& timestamp, std::array<bool, 10>& trace)
{
	std::random_device rd;
	std::mt19937 gen(rd());

	// Define the range for the double value
	double minDouble = 0.0;
	double maxDouble = 1.0;
	std::uniform_real_distribution<double> distDouble(minDouble, maxDouble);

	// Define the range for the bool value
	std::uniform_int_distribution<int> distBool(0, 1);

	timestamp = distDouble(gen);

	for (auto& entry : trace)
		entry = distBool(gen);

	return true;
}

std::string StlinkTraceReader::getLastErrorMsg() const
{
	return lastErrorMsg;
}