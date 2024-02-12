#ifndef _VARREADER_HPP
#define _VARREADER_HPP

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "ITargetMemoryHandler.hpp"
#include "Variable.hpp"
#include "spdlog/spdlog.h"

class TargetMemoryHandler
{
   public:
	TargetMemoryHandler(std::unique_ptr<ITargetMemoryHandler> memoryHandler, spdlog::logger* logger);

	bool start() const;
	bool stop() const;

	uint32_t getValue(uint32_t address) const;
	double getValue(uint32_t address, Variable::type type);
	bool setValue(const Variable& var, double value);
	std::string getLastErrorMsg() const;

	std::vector<uint32_t> getConnectedDevices();

   private:
	std::mutex mtx;
	std::unique_ptr<ITargetMemoryHandler> memoryHandler;
	spdlog::logger* logger;
};

#endif