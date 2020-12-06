#include "mapper_ctx.hpp"

namespace nasa
{
	mapper_ctx::mapper_ctx(
		nasa::mem_ctx& map_into,
		nasa::mem_ctx& map_from
	)
		:
		map_into(map_into),
		map_from(map_from)
	{}

	std::pair<void*, void*> mapper_ctx::map(std::vector<std::uint8_t>& raw_image)
	{
		const auto [drv_alloc, drv_entry_addr] = allocate_driver(raw_image);
		const auto new_pdpt = reinterpret_cast<::ppdpte>(create_self_ref(drv_alloc));
		auto [drv_ppml4e, drv_pml4e] = map_from.get_pml4e(drv_alloc);

		//
		// make all the ptes for the driver kernel access only.
		//
		make_ptes_kernel_access(drv_alloc, raw_image.size());

		//
		// set pfn of cloned pml4e to the newly created pdpt with a large pdpte.
		//
		drv_pml4e.pfn = reinterpret_cast<std::uintptr_t>(new_pdpt) >> 12;
		drv_pml4e.user_supervisor = false;
		
		//
		// set new pml4e into specific process.
		//
		map_into.write_phys(
			reinterpret_cast<std::uintptr_t*>(map_into.get_dirbase()) + PML4_MAP_INDEX,
			drv_pml4e
		);

		virt_addr_t new_addr = { reinterpret_cast<void*>(drv_alloc) };
		new_addr.pml4_index = PML4_MAP_INDEX;
		new_addr.pdpt_index = SELF_REF_ENTRY;
		new_addr.pd_index = NULL;
		return { new_addr.value, drv_entry_addr };
	}

	bool mapper_ctx::call_entry(void* drv_entry, void** hook_handler) const
	{
		const auto result = map_into.k_ctx->syscall<NTSTATUS(__fastcall*)(void**)>(drv_entry, hook_handler);
		return !result;
	}

	std::pair<void*, void*> mapper_ctx::allocate_driver(std::vector<std::uint8_t>& raw_image)
	{
		const auto _get_module = [&](std::string_view name)
		{
			return util::get_module_base(name.data());
		};

		const auto _get_export_name = [&](const char* base, const char* name)
		{
			return reinterpret_cast<std::uintptr_t>(util::get_module_export(base, name));
		};

		nasa::pe_image drv_image(raw_image);
		const auto process_handle =
			OpenProcess(
				PROCESS_ALL_ACCESS,
				FALSE,
				map_from.get_pid()
			);

		if (!process_handle)
			return {};

		drv_image.fix_imports(_get_module, _get_export_name);
		drv_image.map();

		const auto drv_alloc_base = 
			reinterpret_cast<std::uintptr_t>(
				direct::alloc_virtual_memory(
					process_handle,
					drv_image.size() + 0x1000 * 512, // allocate 2mb along with the size of the driver to create a new pt/pde.
					PAGE_READWRITE
				));

		if (!drv_alloc_base)
			return {};

		virt_addr_t new_addr = { reinterpret_cast<void*>(drv_alloc_base) };
		new_addr.pml4_index = PML4_MAP_INDEX;
		new_addr.pdpt_index = SELF_REF_ENTRY;
		new_addr.pd_index = NULL;

		//
		// dont write nt headers...
		//
		drv_image.relocate(reinterpret_cast<std::uintptr_t>(new_addr.value));
		const bool result = direct::write_virtual_memory(
			process_handle,
			reinterpret_cast<void*>((std::uint64_t)drv_alloc_base + drv_image.header_size()),
			reinterpret_cast<void*>((std::uint64_t)drv_image.data() + drv_image.header_size()),
			drv_image.size() - drv_image.header_size()
		);

		CloseHandle(process_handle);
		return
		{ 
			reinterpret_cast<void*>(drv_alloc_base),
			reinterpret_cast<void*>(drv_image.entry_point() + reinterpret_cast<std::uintptr_t>(new_addr.value)) 
		};
	}

	void* mapper_ctx::create_self_ref(void* virt_alloc_base)
	{
		const auto[ppte, pte] = map_from.get_pte(virt_alloc_base);
		auto [ppde, pde] = map_from.get_pde(virt_alloc_base);
		auto [ppdpte, pdpte] = map_from.get_pdpte(virt_alloc_base);

		const auto process_handle = 
			OpenProcess(
				PROCESS_ALL_ACCESS,
				FALSE, 
				map_from.get_pid()
			);

		const auto pdpt = reinterpret_cast<::ppdpte>(
			direct::alloc_virtual_memory(
				process_handle,
				0x1000,
				PAGE_READWRITE
			));

		pde.pfn = reinterpret_cast<std::uintptr_t>(ppte) >> 12;
		pde.user_supervisor = false;

		//
		// write entry into new pdpt
		//
		direct::write_virtual_memory(
			process_handle, 
			pdpt,
			&pde,
			sizeof(pde)
		);

		//
		// physical address of pdpt
		//
		pt_entries entries;
		const auto phys_addr_pdpt = map_from.virt_to_phys(entries, pdpt);

		//
		// make self referencing entry
		//
		pdpte.pfn = reinterpret_cast<std::uintptr_t>(phys_addr_pdpt) >> 12;
		pdpte.user_supervisor = false;

		//
		// write in self referencing entry.
		//
		direct::write_virtual_memory(
			process_handle,
			pdpt + SELF_REF_ENTRY,
			&pdpte,
			sizeof(pdpte)
		);

		CloseHandle(process_handle);
		return phys_addr_pdpt;
	}

	void mapper_ctx::make_ptes_kernel_access(void* drv_base, std::size_t drv_size)
	{
		if (!drv_base || !drv_size)
			return;

		//
		// for each pte make it kernel access only.
		//
		const auto [ppte, pte] = map_from.get_pte(drv_base);
		const auto phys_addr_pt = reinterpret_cast<::ppte>(((std::uintptr_t)ppte >> 12) << 12);
		auto virt_mapping = reinterpret_cast<::ppte>(map_from.set_page(phys_addr_pt));

		for (auto idx = virt_addr_t{ drv_base }.pt_index; idx < 512; ++idx)
		{
			auto drv_pte = *(virt_mapping + idx);
			if (drv_pte.value)
			{
				drv_pte.user_supervisor = false;
				drv_pte.nx = false;
				*(virt_mapping + idx) = drv_pte;
			}
		}
	}
}