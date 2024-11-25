#ifndef _IVARIABLEREADER_HPP
#define _IVARIABLEREADER_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "RingBuffer.hpp"

class IDebugProbe
{
   public:
	enum Mode
	{
		NORMAL = 0,
		HSS = 1,
	};

	typedef struct
	{
		uint32_t debugProbe = 0;
		std::string serialNumber = "";
		std::string device = "";
		Mode mode = Mode::NORMAL;
		uint32_t speedkHz = 10000;

	} DebugProbeSettings;

	/* timestamp (first) and a map of <address-value> entries (second) only fo HSS mode */
	using varEntryType = std::pair<double, std::unordered_map<uint32_t, double>>;

	virtual ~IDebugProbe() = default;
	virtual bool startAcqusition(const DebugProbeSettings& probeSettings, std::vector<std::pair<uint32_t, uint8_t>>& addressSizeVector, uint32_t samplingFreqency) = 0;
	virtual bool stopAcqusition() = 0;
	virtual bool isValid() const = 0;
	virtual std::string getTargetName() = 0;

	/* only HSS mode */
	virtual std::optional<varEntryType> readSingleEntry() = 0;

	/* NORMAL mode */
	virtual bool readMemory(uint32_t address, uint8_t* buf, uint32_t size) = 0;
	virtual bool writeMemory(uint32_t address, uint8_t* buf, uint32_t size) = 0;

	virtual std::string getLastErrorMsg() const = 0;

	virtual std::vector<std::string> getConnectedDevices() = 0;

   protected:
	bool isRunning = false;
	std::string lastErrorMsg = "";
	mutable std::mutex mtx;
};

#endif