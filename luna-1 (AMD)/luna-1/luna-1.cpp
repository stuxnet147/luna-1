#include "luna-1.hpp"
#include "raw_driver.hpp"
#include "patch_ctx/patch_ctx.hpp"
#include "map_driver.hpp"

namespace i6
{
	inline nasa::kernel_ctx* kernel_ctx;
	inline nasa::mem_ctx* mem_ctx;
	inline nasa::patch_ctx* kernel_patch;
	inline hook::detour* win32kbase_detour;

	inline std::mutex syscall_lock;
	inline void* kernel_hook_handler = nullptr;
	inline void* win32u_handler = nullptr;
	inline void* win32kbase_handler = nullptr;
	inline void* patch_address = nullptr;

	enum com_type
	{
		READ,
		WRITE,
		READ_KERNEL_MEMORY,
		WRITE_KERNEL_MEMORY,
		GET_PROCESS_BASE,
		GET_MODULE_BASE
	};

	typedef struct _com_struct
	{
		com_type type;
		unsigned pid;
		unsigned size;
		void* data_from;
		void* data_to;
	} com_struct, * pcom_struct;

	bool begin()
	{
		//
		// make kernel_ctx and fix syscall page so there is no physical memory mapped into this process!
		//
		if (!nasa::load_drv())
			return false;

		//
		// make driver and get the address of hook handler.
		//
		const auto result =
			nasa::map_driver(
				i6::luna_1,
				sizeof(i6::luna_1),
				&kernel_hook_handler
			);

		if (!result)
			goto exit;

		kernel_ctx = new nasa::kernel_ctx();
		mem_ctx = new nasa::mem_ctx(*kernel_ctx, GetCurrentProcessId());
		if (!kernel_ctx->clear_piddb_cache(nasa::drv_key, util::get_file_header((void*)raw_driver)->TimeDateStamp))
			goto exit;

		memset(i6::luna_1, NULL, sizeof(i6::luna_1));
		win32kbase_handler = 
			util::get_module_export(
				"win32kbase.sys",
				"NtDCompositionBeginFrame"
			);

		if (!win32kbase_handler)
			goto exit;

		//
		// needed for syscall
		//
		LoadLibrary("user32.dll");
		win32u_handler =
			::GetProcAddress(
				LoadLibraryA("win32u.dll"),
				"NtDCompositionBeginFrame"
			);

		if (!win32u_handler)
			goto exit;

		kernel_patch = new nasa::patch_ctx(mem_ctx);
		patch_address = kernel_patch->patch(win32kbase_handler);
		win32kbase_detour = new hook::detour(patch_address, kernel_hook_handler, false);

		//
		// enable patch and flush tlb!
		//
		kernel_patch->enable();
		FLUSH_TLB;

	exit:
		if (mem_ctx) delete mem_ctx;
		if (kernel_patch) delete kernel_patch;
		if (kernel_ctx) delete kernel_ctx;

		nasa::unmap_all();
		if (!nasa::unload_drv())
			return false;
		return true;
	}

	void syscall(const pcom_struct com_data)
	{
		syscall_lock.lock();
		win32kbase_detour->install();
		reinterpret_cast<void(*)(pcom_struct)>(win32u_handler)(com_data);
		win32kbase_detour->uninstall();
		syscall_lock.unlock();
	}

	std::uintptr_t get_process_base(unsigned pid)
	{
		if (!pid)
			return {};

		com_struct com_data;
		com_data.type = GET_PROCESS_BASE;
		com_data.pid = pid;
		syscall(&com_data);
		return reinterpret_cast<std::uintptr_t>(com_data.data_from);
	}

	std::uintptr_t get_module_base(unsigned pid, const wchar_t* module_name)
	{
		if (!pid || !module_name)
			return {};

		com_struct com_data;
		com_data.type = GET_MODULE_BASE;
		com_data.data_to = (void*)module_name;
		com_data.pid = pid;
		syscall(&com_data);
		return reinterpret_cast<std::uintptr_t>(com_data.data_from);
	}

	bool read(unsigned pid, std::uintptr_t addr, void* buffer, std::size_t size)
	{
		if (!pid || !addr || !buffer || !size)
			return false;

		com_struct com_data;
		com_data.type = READ;
		com_data.pid = pid;
		com_data.data_from = reinterpret_cast<void*>(addr);
		com_data.data_to = buffer;
		com_data.size = size;
		syscall(&com_data);
		return true;
	}

	bool write(unsigned pid, std::uintptr_t addr, void* buffer, std::size_t size)
	{
		if (!pid || !addr || !buffer || !size)
			return false;

		com_struct com_data;
		com_data.type = WRITE;
		com_data.pid = pid;
		com_data.data_from = buffer;
		com_data.data_to = reinterpret_cast<void*>(addr);
		com_data.size = size;
		syscall(&com_data);
		return true;
	}

	bool rkm(std::uintptr_t addr, void* buffer, std::size_t size)
	{
		if (!addr || !buffer || !size)
			return false;

		com_struct com_data;
		com_data.type = READ_KERNEL_MEMORY;
		com_data.data_from = reinterpret_cast<void*>(addr);
		com_data.data_to = buffer;
		com_data.size = size;
		syscall(&com_data);
		return true;
	}

	bool wkm(std::uintptr_t addr, void* buffer, std::size_t size)
	{
		if (!addr || !buffer || !size)
			return false;

		com_struct com_data;
		com_data.type = WRITE_KERNEL_MEMORY;
		com_data.data_from = buffer;
		com_data.data_to = reinterpret_cast<void*>(addr);
		com_data.size = size;
		syscall(&com_data);
		return true;
	}

	unsigned get_pid(const char* proc_name)
	{
		PROCESSENTRY32 proc_info;
		proc_info.dwSize = sizeof(proc_info);

		const auto proc_snapshot =
			CreateToolhelp32Snapshot(
				TH32CS_SNAPPROCESS,
				NULL
			);

		if (proc_snapshot == INVALID_HANDLE_VALUE)
			return NULL;

		Process32First(proc_snapshot, &proc_info);
		if (!strcmp(proc_info.szExeFile, proc_name))
		{
			CloseHandle(proc_snapshot);
			return proc_info.th32ProcessID;
		}

		while (Process32Next(proc_snapshot, &proc_info))
		{
			if (!strcmp(proc_info.szExeFile, proc_name))
			{
				CloseHandle(proc_snapshot);
				return proc_info.th32ProcessID;
			}
		}

		CloseHandle(proc_snapshot);
		return {};
	}
}