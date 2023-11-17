#ifndef STMVIEWER_JLINKHANDLER_H
#define STMVIEWER_JLINKHANDLER_H

#include "ITargetMemoryHandler.hpp"

typedef int (*jlink_select_by_usb_t)(int);
typedef int (*jlink_lock_t)(int);
// Open takes a log and error handler as inputs
typedef int (*jlink_open_t)(void*, void*);
typedef bool (*jlink_is_open_t)();

// Addr, buf_size, buf, access
// https://github.com/square/pylink/blob/master/pylink/jlink.py#L2936
typedef bool (*jlink_read_mem_t)(uint32_t, int32_t size, uint8_t*, void*);

class JlinkHandler : public ITargetMemoryHandler
{
   public:
	JlinkHandler();
	bool startAcqusition() override;
	bool stopAcqusition() override;
	bool isValid() const override;

	bool readMemory(uint32_t address, uint32_t* value) override;
	bool writeMemory(uint32_t address, uint8_t* buf, uint32_t len) override;

	std::string getLastErrorMsg() const override;

   private:
	void* so_handle;
	jlink_select_by_usb_t jlink_select_by_usb;
	jlink_lock_t jlink_lock;
	jlink_open_t jlink_open;
	jlink_is_open_t jlink_is_open;
	jlink_read_mem_t jlink_read_mem;
};

#endif	// STMVIEWER_JLINKHANDLER_H
