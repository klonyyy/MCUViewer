#include "VarReader.hpp"

#include <map>

#include "iostream"

VarReader::VarReader()
{
	// threadHandle = std::thread(&VarReader::readerHandler, this);
}
VarReader::~VarReader()
{
	// if (threadHandle.joinable())
	// 	threadHandle.join();
}

bool VarReader::addAddress(uint32_t address)
{
	adrMap[address] = 0;

	std::cout << "Map entries:" << std::endl;
	for (auto& addr : adrMap)
		std::cout << " - " << addr.first << std::endl;

	return true;
}
bool VarReader::removeAddress(uint32_t address)
{
	adrMap.erase(address);	// erasing by key
	return true;
}
bool VarReader::removeAllAddresses()
{
	adrMap.clear();
	return true;
}

bool VarReader::start()
{
	if (readerState == state::RUN)
		return false;

	sl = stlink_open_usb((ugly_loglevel)10, (connect_type)0, NULL, 4000);

	std::cout << "Startig! **************" << std::endl;

	if (sl != NULL)
	{
		std::cout << "STlink detected!" << std::endl;
		stlink_version(sl);
		stlink_enter_swd_mode(sl);
		readerState = state::RUN;
		return true;
	}

	std::cout << "STLink not detected!" << std::endl;
	return false;
}
bool VarReader::stop()
{
	if (readerState == state::STOP)
		return false;

	readerState = state::STOP;
	stlink_close(sl);
	return true;
}
void VarReader::readerHandler()
{
	while (1)
	{
		if (readerState == state::RUN)
		{
			std::lock_guard<std::mutex> guard(*mtx);
			for (auto& addr : adrMap)
				stlink_read_debug32(sl, addr.first, (uint32_t*)&addr.second);
		}
		std::this_thread::sleep_for(std::chrono::microseconds(10));
	}
}

uint32_t VarReader::getValue(uint32_t address)
{
	return adrMap[address];
}

float VarReader::getFloat(uint32_t address)
{
	// return *(float*)&adrMap[address];
	uint32_t value = 0;
	if (sl != nullptr)
		stlink_read_debug32(sl, address, (uint32_t*)&value);
	return *(float*)&value;
}