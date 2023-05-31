#ifndef _VARREADER_HPP
#define _VARREADER_HPP

#include <map>
#include <mutex>
#include <thread>

#include "Variable.hpp"
#include "stlink.h"

class VarReader
{
   public:
	VarReader() = default;
	~VarReader() = default;

	bool start();
	bool stop();

	uint32_t getValue(uint32_t address) const;
	double getDouble(uint32_t address, Variable::type type);
	bool setValue(const Variable& var, double value);
	std::string getLastErrorMsg() const;

   private:
	enum class state
	{
		STOP = 0,
		RUN = 1,
	};
	state readerState = state::STOP;
	stlink_t* sl;
	std::mutex mtx;
	std::string lastErrorMsg = {};
};

#endif