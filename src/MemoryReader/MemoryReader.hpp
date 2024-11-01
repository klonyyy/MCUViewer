#ifndef _MEMORYREADER_HPP
#define _MEMORYREADER_HPP

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "IDebugProbe.hpp"
#include "Variable.hpp"

class MemoryReader
{
   public:
	bool start(const IDebugProbe::DebugProbeSettings& probeSettings, std::vector<std::pair<uint32_t, uint8_t>>& addressSizeVector, uint32_t samplingFreqency) const;
	bool stop() const;

	std::optional<IDebugProbe::varEntryType> readSingleEntry();

	uint32_t getValue(uint32_t address, Variable::type type, bool& result);

	bool setValue(const Variable& var, double value);
	std::string getLastErrorMsg() const;

	std::vector<std::string> getConnectedDevices() const;
	void changeDevice(std::shared_ptr<IDebugProbe> newProbe);
	std::string getTargetName();

   private:
	mutable std::mutex mtx;
	std::shared_ptr<IDebugProbe> probe;
};

#endif