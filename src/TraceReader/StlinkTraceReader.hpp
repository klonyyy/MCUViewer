#ifndef _STLINKTRACEREADER_HPP
#define _STLINKTRACEREADER_HPP

#include "ITraceReader.hpp"
#include "stlink.h"

class StlinkTraceReader : public ITraceReader
{
   public:
	StlinkTraceReader();
	bool startAcqusition() override;
	bool stopAcqusition() override;
	bool isValid() const override;

	bool readTrace(double& timestamp, std::array<bool, 10>& trace) override;

	std::string getLastErrorMsg() const override;

   private:
	stlink_t* sl = nullptr;
	bool isRunning = false;
	std::string lastErrorMsg = "";
};

#endif