#pragma once
#include "../util/util.hpp"
#include "../physmeme/physmeme.hpp"
#include "../util/hook.hpp"

namespace nasa
{
	//
	// offset of function into a physical page
	// used for comparing bytes when searching
	//
	inline std::uint16_t nt_page_offset{};

	//
	// rva of nt function we are going to hook
	//
	inline std::uint32_t nt_rva{};

	//
	// base address of ntoskrnl (inside of this process)
	//
	inline const std::uint8_t* ntoskrnl_buffer{};

	//
	// has the page been found yet?
	//
	inline std::atomic<bool> is_page_found = false;

	//
	// mapping of a syscalls physical memory (for installing hooks)
	//
	inline std::atomic<void*> psyscall_func{};

	//
	// you can edit this how you choose, im hooking NtShutdownSystem.
	//
	inline const std::pair<std::string_view, std::string_view> syscall_hook = { "NtShutdownSystem", "ntdll.dll" };

	class kernel_ctx
	{
		friend class mem_ctx;
	public:
		kernel_ctx();

		//
		// read kernel memory into buffer
		//
		void rkm(void* buffer, void* address, std::size_t size);

		//
		// write kernel memory from buffer
		//
		void wkm(void* buffer, void* address, std::size_t size);

		template <class T>
		T rkm(void* addr)
		{
			if (!addr)
				return {};
			T buffer;
			rkm((void*)&buffer, addr, sizeof(T));
			return buffer;
		}

		template <class T>
		void wkm(void* addr, const T& data)
		{
			if (!addr)
				return;
			wkm((void*)&data, addr, sizeof(T));
		}

		//
		// gets physical address from virtual
		//
		void* get_physical(void* virt_addr);

		//
		// uses the pfn database to get the virtual address
		//
		void* get_virtual(void* virt_addr);

		//
		// use this to call any function in the kernel
		//
		template <class T, class ... Ts>
		std::invoke_result_t<T, Ts...> syscall(void* addr, Ts ... args) const
		{
			static const auto proc =
				GetProcAddress(
					GetModuleHandleA(syscall_hook.second.data()),
					syscall_hook.first.data()
				);

			hook::make_hook(psyscall_func, addr);
			auto result = reinterpret_cast<T>(proc)(args ...);
			hook::remove(psyscall_func);
			return result;
		}

		//
		// clear piddb cache of specific process.
		//
		bool clear_piddb_cache(const std::string& drv_name, const std::uint32_t epoch_time);

		//
		// get a pointer to an eprocess given process id.
		//
		PEPROCESS get_peprocess(unsigned pid) const;

		//
		// get base address of process (used to compare and ensure we find the right page).
		//
		void* get_proc_base(unsigned pid) const;
		void* allocate_pool(std::size_t size, POOL_TYPE pool_type);
		void zero_kernel_memory(void* addr, std::size_t size);
	private:

		//
		// find and map the physical page of a syscall into this process
		//
		void map_syscall(std::uintptr_t begin, std::uintptr_t end) const;
	};
}