#ifndef _VARREADER_HPP
#define _VARREADER_HPP

#include <thread>

#include "stlink.h"

class VarReader
{
   public:
	VarReader();
	~VarReader();

	bool start();
	bool stop();

	float geta();

   private:
	enum class state
	{
		STOP = 0,
		RUN = 1,
	};
	float a;
	state readerState = state::STOP;
	stlink_t* sl;
	std::thread threadHandle;
	void readerHandler();
};

#endif