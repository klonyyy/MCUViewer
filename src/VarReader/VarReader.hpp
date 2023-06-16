#ifndef _VARREADER_HPP
#define _VARREADER_HPP

#include <map>
#include <mutex>
#include <thread>

#include "Variable.hpp"
#include "spdlog/spdlog.h"
#include "stlink.h"

class VarReader
{
   public:
	VarReader(std::shared_ptr<spdlog::logger> logger);

	bool start();
	bool stop();

	uint32_t getValue(uint32_t address) const;
	double getDouble(uint32_t address, Variable::type type);
	bool setValue(const Variable& var, double value);
	std::string getLastErrorMsg() const;

   private:
	stlink_t* sl;
	std::mutex mtx;
	std::string lastErrorMsg = {};
	std::shared_ptr<spdlog::logger> logger;
};

#endif