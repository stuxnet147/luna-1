#pragma once
#include "../mem_ctx/mem_ctx.hpp"

namespace nasa
{
	class patch_ctx
	{
	public:
		explicit patch_ctx(mem_ctx* mem_ctx);

		//
		// returns a virtual address mapping of the newly patched page(s).
		//
		void* patch(void* kernel_addr);
		__forceinline void enable()
		{
			mapped_pml4e->pfn = new_pml4e.pfn;
		}

		__forceinline void disable()
		{
			mapped_pml4e->pfn = old_pml4e.pfn;
		}
	private:

		//
		// std::pair< physical page, virtual address >
		//
		std::pair<void*, void*> make_pdpt(const pt_entries& kernel_entries, void* kernel_addr);
		std::pair<void*, void*> make_pd(const pt_entries& kernel_entries, void* kernel_addr);
		std::pair<void*, void*> make_pt(const pt_entries& kernel_entries, void* kernel_addr);
		std::pair<void*, void*> make_page(const pt_entries& kernel_entries, void* kernel_addr);

		//
		// context of the current process you want to patch.
		//
		mem_ctx* mem_ctx;

		//
		// newly created table entries and table pointers (pdpte, pde, pte)
		//
		pt_entries new_entries;

		//
		// old and new pml4e
		//
		pml4e new_pml4e;
		pml4e old_pml4e;

		//
		// kernel address of the patch.
		//
		void* kernel_addr;

		//
		// pointer to the mapped pml4e
		// used for enable/disable patch...
		//
		ppml4e mapped_pml4e;
	};
}