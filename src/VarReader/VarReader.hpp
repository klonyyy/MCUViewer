#ifndef _VARREADER_HPP
#define _VARREADER_HPP

#include <map>
#include <mutex>
#include <thread>

#include "stlink.h"

class VarReader
{
   public:
	VarReader();
	~VarReader();

	bool addAddress(uint32_t address);
	bool removeAddress(uint32_t address);
	bool removeAllAddresses();

	bool start();
	bool stop();

	uint32_t getValue(uint32_t address);

	float getFloat(uint32_t address);

   private:
	std::mutex* mtx;
	enum class state
	{
		STOP = 0,
		RUN = 1,
	};
	float a;
	std::map<uint32_t, uint32_t> adrMap;
	std::map<uint32_t, uint32_t>::iterator it;
	state readerState = state::STOP;
	stlink_t* sl;
	std::thread threadHandle;
	void readerHandler();
};

#endif