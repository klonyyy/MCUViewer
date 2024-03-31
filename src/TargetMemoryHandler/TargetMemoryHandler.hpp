#ifndef _VARREADER_HPP
#define _VARREADER_HPP

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "IDebugProbe.hpp"
#include "Variable.hpp"

class TargetMemoryHandler
{
   public:
	bool start(const std::string& serialNumber, std::vector<std::pair<uint32_t, uint8_t>>& addressSizeVector, IDebugProbe::Mode mode = IDebugProbe::Mode::NORMAL, const std::string& device = "") const;
	bool stop() const;
	bool initRead() const;

	uint32_t getValue(uint32_t address) const;
	double getValue(uint32_t address, Variable::type type);
	bool setValue(const Variable& var, double value);
	std::string getLastErrorMsg() const;

	std::vector<std::string> getConnectedDevices() const;
	void changeDevice(std::shared_ptr<IDebugProbe> newProbe);

   private:
	mutable std::mutex mtx;
	std::shared_ptr<IDebugProbe> probe;
};

#endif