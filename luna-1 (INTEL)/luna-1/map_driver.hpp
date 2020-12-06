#pragma once
#include <windows.h>
#include <fstream>
#include <cstddef>
#include <cstdint>

#include "mapper_ctx/mapper_ctx.hpp"
namespace i6
{
	enum class i6_error
	{
		error_success = 0x000,	   // everything is good!
		image_invalid = 0x111,	   // the driver your trying to map is invalid (are you importing things that arent in ntoskrnl?)
		load_error = 0x222,	   // unable to load signed driver into the kernel (are you running as admin?)
		unload_error = 0x333,	   // unable to unload signed driver from kernel (are all handles to this driver closes?)
		piddb_fail = 0x444,	   // piddb cache clearing failed... (are you using this code below windows 10? if so please contact for a different version)
		init_failed = 0x555,	   // setting up library dependancies failed!
		failed_to_create_proc = 0x777 // was unable to create a new process to inject driver into! (RuntimeBroker.exe)
	};
	i6_error map_driver(std::uint8_t* drv_image, std::size_t image_size, void** entry_data);
}