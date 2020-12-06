#pragma once
#include <cstddef>
#include <cstdint>

namespace i6
{
	//
	// please call this before making any other calls!
	//
	bool begin();

	//
	// get process id of process.
	//
	unsigned get_pid(const char* proc_name);

	//
	// get process base address.
	//
	std::uintptr_t get_process_base(unsigned pid);

	//
	// get module base
	//
	std::uintptr_t get_module_base(unsigned pid, const wchar_t* module_name);

	//
	// read/write to specific process
	//
	bool read(unsigned pid, std::uintptr_t addr, void* buffer, std::size_t size);
	bool write(unsigned pid, std::uintptr_t addr, void* buffer, std::size_t size);

	//
	// read/write kernel memory (you can write to readonly with this)
	//
	bool rkm(std::uintptr_t addr, void* buffer, std::size_t size);
	bool wkm(std::uintptr_t addr, void* buffer, std::size_t size);

	template <class T>
	T rkm(std::uintptr_t addr)
	{
		T buffer{};
		rkm(addr, (void*)&buffer, sizeof(T));
		return buffer;
	}

	template <class T>
	bool wkm(std::uintptr_t addr, const T& data)
	{
		return wkm(addr, (void*)&data, sizeof(T));
	}

	template <class T>
	T read(unsigned pid, std::uintptr_t addr)
	{
		T buffer{};
		read(pid, addr, (void*)&buffer, sizeof(T));
		return buffer;
	}

	template <class T>
	bool write(unsigned pid, std::uintptr_t addr, const T& data)
	{
		return write(pid, addr, (void*)&data, sizeof(T));
	}
}