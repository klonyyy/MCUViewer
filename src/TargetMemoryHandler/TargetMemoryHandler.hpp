#ifndef _VARREADER_HPP
#define _VARREADER_HPP

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "IDebugProbe.hpp"
#include "Variable.hpp"
#include "spdlog/spdlog.h"

class TargetMemoryHandler
{
   public:
	TargetMemoryHandler(spdlog::logger* logger);

	bool start(const std::string& serialNumber) const;
	bool stop() const;

	uint32_t getValue(uint32_t address) const;
	double getValue(uint32_t address, Variable::type type);
	bool setValue(const Variable& var, double value);
	std::string getLastErrorMsg() const;

	std::vector<std::string> getConnectedDevices();

	/* TODO */
	void changeDevice(std::shared_ptr<IDebugProbe> newProbe)
	{
		memoryHandler = newProbe;
	}

   private:
	std::mutex mtx;
	std::shared_ptr<IDebugProbe> memoryHandler;
	spdlog::logger* logger;
};

#endif