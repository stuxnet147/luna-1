#include "patch_ctx.hpp"

namespace nasa
{
	patch_ctx::patch_ctx(nasa::mem_ctx* mem_ctx)
		:
		mem_ctx(mem_ctx)
	{}

	void* patch_ctx::patch(void* kernel_addr)
	{
		if (!kernel_addr)
			return {};
		
		virt_addr_t kernel_addr_t{ kernel_addr };
		pt_entries kernel_entries;
		if (!mem_ctx->hyperspace_entries(kernel_entries, kernel_addr))
			return {};
		
		const auto [new_pdpt_phys, new_pdpt_virt] = make_pdpt(kernel_entries, kernel_addr);
		const auto [new_pd_phys, new_pd_virt] = make_pd(kernel_entries, kernel_addr);
		const auto [new_pt_phys, new_pt_virt] = make_pt(kernel_entries, kernel_addr);
		const auto [new_page_phys, new_page_virt] = make_page(kernel_entries, kernel_addr);

		// link pdpte to pd
		(reinterpret_cast<ppdpte>(new_pdpt_virt) + kernel_addr_t.pdpt_index)->pfn = reinterpret_cast<std::uintptr_t>(new_pd_phys) >> 12;
		if (kernel_entries.pd.second.page_size)
			// link pde to new page if its 2mb
			(reinterpret_cast<ppde>(new_pd_virt) + kernel_addr_t.pd_index)->pfn = reinterpret_cast<std::uintptr_t>(new_page_phys) >> 12;
		else
		{
			// link pde to pt
			(reinterpret_cast<ppde>(new_pd_virt) + kernel_addr_t.pd_index)->pfn = reinterpret_cast<std::uintptr_t>(new_pt_phys) >> 12;
			// link pte to page (1kb)
			(reinterpret_cast<ppte>(new_pt_virt) + kernel_addr_t.pt_index)->pfn = reinterpret_cast<std::uintptr_t>(new_page_phys) >> 12;
		}

		mapped_pml4e = 
			reinterpret_cast<ppml4e>(
				mem_ctx->set_page(
					kernel_entries.pml4.first));

		new_pml4e = kernel_entries.pml4.second;
		new_pml4e.pfn = reinterpret_cast<std::uintptr_t>(new_pdpt_phys) >> 12;
		old_pml4e = kernel_entries.pml4.second;
		return reinterpret_cast<void*>((std::uintptr_t)new_page_virt + kernel_addr_t.offset);
	}

	std::pair<void*, void*> patch_ctx::make_pdpt(const pt_entries& kernel_entries, void* kernel_addr)
	{
		if (!kernel_addr)
			return  {};

		virt_addr_t kernel_addr_t{ kernel_addr };
		const auto pdpt = reinterpret_cast<ppdpte>(
			mem_ctx->set_page(reinterpret_cast<void*>(
				kernel_entries.pml4.second.pfn << 12)));

		const auto new_pdpt =
			reinterpret_cast<ppdpte>(VirtualAlloc(
				NULL,
				0x1000,
				MEM_COMMIT | MEM_RESERVE,
				PAGE_READWRITE
			));

		//
		// zero pdpt and lock it.
		// 
		memset(new_pdpt, NULL, 0x1000);
		if (!VirtualLock(new_pdpt, 0x1000))
			return {};


		//
		// copy over pdpte's
		//
		for (auto idx = 0u; idx < 512; ++idx)
			*(pdpte*)(new_pdpt + idx) = *(pdpte*)(pdpt + idx);

		//
		// get physical address of new pdpt
		//
		pt_entries new_pdpt_entries;
		const auto physical_addr =
			mem_ctx->virt_to_phys(
				new_pdpt_entries,
				new_pdpt
			);

		return { physical_addr, new_pdpt };
	}

	std::pair<void*, void*> patch_ctx::make_pd(const pt_entries& kernel_entries, void* kernel_addr)
	{
		if (!kernel_addr)
			return {};

		virt_addr_t kernel_addr_t{ kernel_addr };
		const auto new_pd = reinterpret_cast<ppde>(
			VirtualAlloc(
				NULL,
				0x1000,
				MEM_COMMIT | MEM_RESERVE,
				PAGE_READWRITE
			));

		//
		// zero page and lock it.
		//
		memset(new_pd, NULL, 0x1000);
		if (!VirtualLock(new_pd, 0x1000))
			return {};

		const auto pd = reinterpret_cast<ppde>(
			mem_ctx->set_page(
				reinterpret_cast<void*>(
					kernel_entries.pdpt.second.pfn << 12)));

		//
		// copy over pde's
		//
		for (auto idx = 0u; idx < 512; ++idx)
			*(::pde*)(new_pd + idx) = *(::pde*)(pd + idx);

		//
		// get physical address of new pd
		//
		pt_entries pd_entries;
		const auto physical_addr =
			mem_ctx->virt_to_phys(
				pd_entries,
				new_pd
			);

		return { physical_addr, new_pd };
	}

	std::pair<void*, void*> patch_ctx::make_pt(const pt_entries& kernel_entries, void* kernel_addr)
	{
		if (!kernel_addr || kernel_entries.pd.second.page_size) // if this address is a 2mb mapping.
			return {};

		virt_addr_t kernel_addr_t{ kernel_addr };
		const auto new_pt = reinterpret_cast<ppte>(
			VirtualAlloc(
				NULL,
				0x1000,
				MEM_COMMIT | MEM_RESERVE,
				PAGE_READWRITE
			));

		//
		// zero pt and lock it.
		//
		memset(new_pt, NULL, 0x1000);
		if (!VirtualLock(new_pt, 0x1000))
			return {};

		const auto pt = reinterpret_cast<ppde>(
			mem_ctx->set_page(
				reinterpret_cast<void*>(
					kernel_entries.pd.second.pfn << 12)));

		//
		// copy over pte's
		//
		for (auto idx = 0u; idx < 512; ++idx)
			*(::pde*)(new_pt + idx) = *(::pde*)(pt + idx);

		//
		// get physical address of new pt
		//
		pt_entries entries;
		const auto physical_addr =
			mem_ctx->virt_to_phys(
				entries,
				new_pt
			);

		return { physical_addr, new_pt };
	}

	std::pair<void*, void*> patch_ctx::make_page(const pt_entries& kernel_entries, void* kernel_addr)
	{
		if (!kernel_addr)
			return {};

		virt_addr_t kernel_addr_t{ kernel_addr };

		//
		// if its a 2mb page
		//
		if (kernel_entries.pd.second.page_size)
		{
			const auto new_page = VirtualAlloc(
				NULL,
				0x1000 * 512 * 2, // 4mb
				MEM_COMMIT | MEM_RESERVE,
				PAGE_READWRITE
			);
			memset(new_page, NULL, 512 * 0x1000); // zero 2mb

			//
			// copy 2mb one page at a time.
			//
			for (auto idx = 0u; idx < 512; ++idx)
			{
				const auto old_page = mem_ctx->set_page(reinterpret_cast<void*>((kernel_entries.pt.second.pfn << 12) + (idx * 0x1000)));
				memcpy((void*)((std::uintptr_t)new_page + (idx * 0x1000)), old_page, 0x1000);
			}

			pt_entries new_page_entries;
			const auto new_page_phys = mem_ctx->virt_to_phys(new_page_entries, new_page);
			return { new_page_phys, new_page };
		}
		else // 1kb
		{
			const auto new_page = VirtualAlloc(
				NULL,
				0x1000, 
				MEM_COMMIT | MEM_RESERVE,
				PAGE_READWRITE
			);

			//
			// zero and lock new page.
			//
			memset(new_page, NULL, 0x1000);
			if (!VirtualLock(new_page, 0x1000))
				return {};

			const auto old_page =
				mem_ctx->set_page(
					reinterpret_cast<void*>(
						kernel_entries.pt.second.pfn << 12));
			memcpy(new_page, old_page, 0x1000);

			pt_entries new_page_entries;
			const auto new_page_phys = 
				mem_ctx->virt_to_phys(
					new_page_entries,
					new_page
				);

			return { new_page_phys, new_page };
		}
	}
}