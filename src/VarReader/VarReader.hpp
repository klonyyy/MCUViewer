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
	VarReader();
	~VarReader();

	bool start();
	bool stop();

	uint32_t getValue(uint32_t address);
	float getFloat(uint32_t address, Variable::type type);
	bool setValue(Variable& var, float value);

   private:
	enum class state
	{
		STOP = 0,
		RUN = 1,
	};
	state readerState = state::STOP;
	stlink_t* sl;
	std::mutex mtx;
};

#endif