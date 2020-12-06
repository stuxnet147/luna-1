#pragma once
#include "../util/nt.hpp"
#include "../kernel_ctx/kernel_ctx.h"

#define FLUSH_TLB Sleep(10)

struct pt_entries
{
	std::pair<ppml4e, pml4e>	pml4;
	std::pair<ppdpte, pdpte>	pdpt;
	std::pair<ppde, pde>		pd;
	std::pair<ppte, pte>		pt;
};

namespace nasa
{
	class mem_ctx
	{
	public:
		explicit mem_ctx(kernel_ctx& k_ctx, DWORD pid);
		~mem_ctx();

		//
		// PTE manipulation
		//
		std::pair<ppte, pte> get_pte(void* addr, bool use_hyperspace = false);
		void set_pte(void* addr, const ::pte& pte, bool use_hyperspace = false);

		//
		// PDE manipulation
		//
		std::pair<ppde, pde> get_pde(void* addr, bool use_hyperspace = false);
		void set_pde(void* addr, const ::pde& pde, bool use_hyperspace = false);

		//
		// PDPTE manipulation
		//
		std::pair<ppdpte, pdpte> get_pdpte(void* addr, bool use_hyperspace = false);
		void set_pdpte(void* addr, const ::pdpte& pdpte, bool use_hyperspace = false);

		//
		// PML4E manipulation
		//
		std::pair<ppml4e, pml4e> get_pml4e(void* addr, bool use_hyperspace = false);
		void set_pml4e(void* addr, const ::pml4e& pml4e, bool use_hyperspace = false);

		//
		// gets dirbase (not the PTE or PFN but actual physical address)
		//
		void* get_dirbase() const;
		static void* get_dirbase(kernel_ctx& k_ctx, DWORD pid);

		void read_phys(void* buffer, void* addr, std::size_t size);
		void write_phys(void* buffer, void* addr, std::size_t size);

		template <class T>
		T read_phys(void* addr)
		{
			if (!addr)
				return {};
			T buffer;
			read_phys((void*)&buffer, addr, sizeof(T));
			return buffer;
		}

		template <class T>
		void write_phys(void* addr, const T& data)
		{
			if (!addr)
				return;
			write_phys((void*)&data, addr, sizeof(T));
		}

		std::pair<void*, void*> read_virtual(void* buffer, void* addr, std::size_t size);
		std::pair<void*, void*> write_virtual(void* buffer, void* addr, std::size_t size);

		template <class T>
		T read_virtual(void* addr)
		{
			if (!addr)
				return {};
			T buffer;
			read_virtual((void*)&buffer, addr, sizeof(T));
			return buffer;
		}

		template <class T>
		void write_virtual(void* addr, const T& data)
		{
			write_virtual((void*)&data, addr, sizeof(T));
		}

		//
		// linear address translation (not done by hyperspace mappings)
		//
		void* virt_to_phys(pt_entries& entries, void* addr);

		//
		// given an address fill pt entries with physical addresses and entry values.
		//
		bool hyperspace_entries(pt_entries& entries, void* addr);

		//
		// these are used for the pfn backdoor, this will be removed soon
		//
		void* set_page(void* addr);

		//
		// get current page mem_ctx is set to.
		//
		void* get_page() const;

		//
		// invalidate TLB
		//
		void page_accessed() const;
		unsigned get_pid() const;

		//
		// set dirbase.
		//
		void set_dirbase(void* dirbase);

		kernel_ctx* k_ctx;
	private:
		std::pair<void*, pte> genesis_page;
		std::pair<ppte, pte> genesis_cursor;
		void* dirbase;

		unsigned pid;
		unsigned short page_offset;
	};
}