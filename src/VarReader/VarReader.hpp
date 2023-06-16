#ifndef _VARREADER_HPP
#define _VARREADER_HPP

#include <map>
#include <mutex>
#include <thread>

#include "IVariableReader.hpp"
#include "Variable.hpp"
#include "spdlog/spdlog.h"

class VarReader
{
   public:
	VarReader(IVariableReader* variableReader, std::shared_ptr<spdlog::logger> logger);

	bool start();
	bool stop();

	uint32_t getValue(uint32_t address) const;
	double getValue(uint32_t address, Variable::type type);
	bool setValue(const Variable& var, double value);
	std::string getLastErrorMsg() const;

   private:
	std::mutex mtx;
	std::unique_ptr<IVariableReader> variableReader;
	std::shared_ptr<spdlog::logger> logger;
};

#endif