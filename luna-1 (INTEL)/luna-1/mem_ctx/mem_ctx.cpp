#include "mem_ctx.hpp"
#include <cassert>

namespace nasa
{
	mem_ctx::mem_ctx(kernel_ctx& krnl_ctx, DWORD pid)
		:
		k_ctx(&krnl_ctx),
		dirbase(get_dirbase(krnl_ctx, pid)),
		pid(pid)
	{
		genesis_page.first = VirtualAlloc(
			NULL,
			PAGE_SIZE,
			MEM_COMMIT | MEM_RESERVE,
			PAGE_READWRITE
		);

		//
		// page in the page, do not remove this makes the entries.
		//
		*(std::uintptr_t*)genesis_page.first = 0xC0FFEE;

		//
		// flush tlb since we just accessed the page
		//
		page_accessed();

		//
		// get the ppte and pte of the page we allocated
		//
		auto [page_ppte, page_pte] = get_pte(genesis_page.first, true);
		genesis_page.second = page_pte;

		//
		// allocate a page that will get the mapping of the first pages PT
		//
		genesis_cursor.first = reinterpret_cast<::ppte>(
			VirtualAlloc(
				NULL,
				0x1000,
				MEM_COMMIT | MEM_RESERVE,
				PAGE_READWRITE
			));

		//
		// page it in
		//
		*(std::uintptr_t*)genesis_cursor.first = 0xC0FFEE;

		//
		// flush tlb since we just accessed the page
		//
		page_accessed();

		// 
		// get ppte and pte of the cursor page.
		//
		auto [cursor_ppte, cursor_pte] = get_pte(genesis_cursor.first, true);
		genesis_cursor.second = cursor_pte;

		//
		// change the page to the PT of the first page we allocated.
		//
		cursor_pte.pfn = reinterpret_cast<std::uint64_t>(page_ppte) >> 12;
		set_pte(genesis_cursor.first, cursor_pte, true);

		//
		// change the offset of genesis cursor page to genesis pages pt_index since the page is now a PT
		// WARNING: pointer arithmetic, do not add pt_index * 8
		//
		genesis_cursor.first += +virt_addr_t{ genesis_page.first }.pt_index;

		//
		// since the page has been accessed we need to reset this bit in the PTE.
		//
		page_accessed();
	}

	mem_ctx::~mem_ctx()
	{
		set_pte(genesis_page.first, genesis_page.second, true);
		set_pte(genesis_cursor.first, genesis_cursor.second, true);
	}

	void* mem_ctx::set_page(void* addr)
	{
		page_offset = virt_addr_t{ addr }.offset;
		genesis_cursor.first->pfn = reinterpret_cast<uint64_t>(addr) >> 12;
		page_accessed();
		return get_page();
	}

	void* mem_ctx::get_page() const
	{
		return reinterpret_cast<void*>((reinterpret_cast<std::uint64_t>(genesis_page.first)) + page_offset);
	}

	void mem_ctx::page_accessed() const
	{
		Sleep(5);
	}

	void* mem_ctx::get_dirbase(kernel_ctx& k_ctx, DWORD pid)
	{
		if (!pid)
			return NULL;

		const auto peproc =
			reinterpret_cast<std::uint64_t>(k_ctx.get_peprocess(pid));

		if (!peproc)
			return NULL;

		const auto dirbase = 
			k_ctx.rkm<pte>(
				reinterpret_cast<void*>(peproc + 0x28));

		return reinterpret_cast<void*>(dirbase.pfn << 12);
	}

	void mem_ctx::set_dirbase(void* dirbase)
	{
		this->dirbase = dirbase;
	}

	bool mem_ctx::hyperspace_entries(pt_entries& entries, void* addr)
	{
		if (!addr || !dirbase)
			return false;

		virt_addr_t virt_addr{ addr };
		entries.pml4.first = reinterpret_cast<ppml4e>(dirbase) + virt_addr.pml4_index;
		entries.pml4.second = k_ctx->rkm<pml4e>(
			k_ctx->get_virtual(entries.pml4.first));

		if (!entries.pml4.second.value)
			return false;

		entries.pdpt.first = reinterpret_cast<ppdpte>(entries.pml4.second.pfn << 12) + virt_addr.pdpt_index;
		entries.pdpt.second = k_ctx->rkm<pdpte>(
			k_ctx->get_virtual(entries.pdpt.first));

		if (!entries.pdpt.second.value)
			return false;

		entries.pd.first = reinterpret_cast<ppde>(entries.pdpt.second.pfn << 12) + virt_addr.pd_index;
		entries.pd.second = k_ctx->rkm<pde>(
			k_ctx->get_virtual(entries.pd.first));

		// if its a 2mb page
		if (entries.pd.second.page_size)
		{
			memcpy(
				&entries.pt.second,
				&entries.pd.second,
				sizeof(pte)
			);

			entries.pt.first = reinterpret_cast<ppte>(entries.pd.second.value);
			return true;
		}

		entries.pt.first = reinterpret_cast<ppte>(entries.pd.second.pfn << 12) + virt_addr.pt_index;
		entries.pt.second = k_ctx->rkm<pte>(
			k_ctx->get_virtual(entries.pt.first));

		if (!entries.pt.second.value)
			return false;

		return true;
	}

	std::pair<ppte, pte> mem_ctx::get_pte(void* addr, bool use_hyperspace)
	{
		if (!dirbase || !addr)
			return {};

		pt_entries entries;
		if (use_hyperspace ? hyperspace_entries(entries, addr) : (bool)virt_to_phys(entries, addr))
		{
			::pte pte;
			memcpy(&pte, &entries.pt.second, sizeof(pte));
			return { entries.pt.first, pte };
		}
		return {};
	}

	void mem_ctx::set_pte(void* addr, const ::pte& pte, bool use_hyperspace)
	{
		if (!dirbase || !addr)
			return;

		pt_entries entries;
		if (use_hyperspace)
			if (hyperspace_entries(entries, addr))
				k_ctx->wkm(
					k_ctx->get_virtual(entries.pt.first),
					pte
				);
			else
				if (virt_to_phys(entries, addr))
					write_phys(entries.pt.first, pte);
	}

	std::pair<ppde, pde> mem_ctx::get_pde(void* addr, bool use_hyperspace)
	{
		if (!dirbase || !addr)
			return {};

		pt_entries entries;
		if (use_hyperspace ? hyperspace_entries(entries, addr) : (bool)virt_to_phys(entries, addr))
		{
			::pde pde;
			memcpy(
				&pde,
				&entries.pd.second,
				sizeof(pde)
			);
			return { entries.pd.first, pde };
		}
		return {};
	}

	void mem_ctx::set_pde(void* addr, const ::pde& pde, bool use_hyperspace)
	{
		if (!dirbase || !addr)
			return;

		pt_entries entries;
		if (use_hyperspace)
			if (hyperspace_entries(entries, addr))
				k_ctx->wkm(
					k_ctx->get_virtual(entries.pd.first),
					pde
				);
			else
				if (virt_to_phys(entries, addr))
					write_phys(entries.pd.first, pde);
	}

	std::pair<ppdpte, pdpte> mem_ctx::get_pdpte(void* addr, bool use_hyperspace)
	{
		if (!dirbase || !addr)
			return {};

		pt_entries entries;
		if (use_hyperspace ? hyperspace_entries(entries, addr) : (bool)virt_to_phys(entries, addr))
		{
			::pdpte pdpte;
			memcpy(
				&pdpte,
				&entries.pdpt.second,
				sizeof(pdpte)
			);
			return { entries.pdpt.first, pdpte };
		}
		return {};
	}

	void mem_ctx::set_pdpte(void* addr, const ::pdpte& pdpte, bool use_hyperspace)
	{
		if (!dirbase || !addr)
			return;

		pt_entries entries;
		if (use_hyperspace)
			if (hyperspace_entries(entries, addr))
				k_ctx->wkm(
					k_ctx->get_virtual(entries.pdpt.first),
					pdpte
				);
			else
				if (virt_to_phys(entries, addr))
					write_phys(entries.pdpt.first, pdpte);
	}

	std::pair<ppml4e, pml4e> mem_ctx::get_pml4e(void* addr, bool use_hyperspace)
	{
		if (!dirbase || !addr)
			return {};

		pt_entries entries;
		if (use_hyperspace ? hyperspace_entries(entries, addr) : (bool)virt_to_phys(entries, addr))
		{
			::pml4e pml4e;
			memcpy(
				&pml4e,
				&entries.pml4.second,
				sizeof(pml4e)
			);
			return { entries.pml4.first, pml4e };
		}
		return {};
	}

	void mem_ctx::set_pml4e(void* addr, const ::pml4e& pml4e, bool use_hyperspace)
	{
		if (!dirbase || !addr)
			return;

		pt_entries entries;
		if (use_hyperspace)
			if (hyperspace_entries(entries, addr))
				k_ctx->wkm(
					k_ctx->get_virtual(entries.pml4.first),
					pml4e
				);
			else
				if (virt_to_phys(entries, addr))
					write_phys(entries.pml4.first, pml4e);
	}

	std::pair<void*, void*> mem_ctx::read_virtual(void* buffer, void* addr, std::size_t size)
	{
		if (!buffer || !addr || !size || !dirbase)
			return {};

		virt_addr_t virt_addr{ addr };
		if (size <= PAGE_SIZE - virt_addr.offset)
		{
			pt_entries entries;
			read_phys(
				buffer,
				virt_to_phys(entries, addr),
				size
			);
			return {
				reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(buffer) + size),
				reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(addr) + size)
			};
		}
		else
		{
			// cut remainder
			const auto [new_buffer_addr, new_addr] = read_virtual(
				buffer,
				addr,
				PAGE_SIZE - virt_addr.offset
			);
			// forward work load
			return read_virtual(
				new_buffer_addr,
				new_addr,
				size - (PAGE_SIZE - virt_addr.offset)
			);
		}
	}

	std::pair<void*, void*> mem_ctx::write_virtual(void* buffer, void* addr, std::size_t size)
	{
		if (!buffer || !addr || !size || !dirbase)
			return {};

		virt_addr_t virt_addr{ addr };
		if (size <= PAGE_SIZE - virt_addr.offset)
		{
			pt_entries entries;
			write_phys(
				buffer,
				virt_to_phys(entries, addr),
				size
			);

			return {
				reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(buffer) + size),
				reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(addr) + size)
			};
		}
		else
		{
			// cut remainder
			const auto [new_buffer_addr, new_addr] = write_virtual(
				buffer,
				addr,
				PAGE_SIZE - virt_addr.offset
			);

			// forward work load
			return write_virtual(
				new_buffer_addr,
				new_addr,
				size - (PAGE_SIZE - virt_addr.offset)
			);
		}
	}

	void mem_ctx::read_phys(void* buffer, void* addr, std::size_t size)
	{
		if (!buffer || !addr || !size)
			return;

		auto temp_page = set_page(addr);
		if (temp_page)
			memcpy(buffer, temp_page, size);
	}

	void mem_ctx::write_phys(void* buffer, void* addr, std::size_t size)
	{
		if (!buffer || !addr || !size)
			return;

		auto temp_page = set_page(addr);
		if (temp_page)
			memcpy(temp_page, buffer, size);
	}

	void* mem_ctx::virt_to_phys(pt_entries& entries, void* addr)
	{
		if (!addr || !dirbase)
			return {};

		virt_addr_t virt_addr{ addr };

		//
		// traverse paging tables
		//
		auto pml4e = read_phys<::pml4e>(
			reinterpret_cast<ppml4e>(dirbase) + virt_addr.pml4_index);

		entries.pml4.first = reinterpret_cast<ppml4e>(dirbase) + virt_addr.pml4_index;
		entries.pml4.second = pml4e;

		if (!pml4e.value)
			return NULL;

		auto pdpte = read_phys<::pdpte>(
			reinterpret_cast<ppdpte>(pml4e.pfn << 12) + virt_addr.pdpt_index);

		entries.pdpt.first = reinterpret_cast<ppdpte>(pml4e.pfn << 12) + virt_addr.pdpt_index;
		entries.pdpt.second = pdpte;

		if (!pdpte.value)
			return NULL;

		auto pde = read_phys<::pde>(
			reinterpret_cast<ppde>(pdpte.pfn << 12) + virt_addr.pd_index);

		entries.pd.first = reinterpret_cast<ppde>(pdpte.pfn << 12) + virt_addr.pd_index;
		entries.pd.second = pde;

		if (!pde.value)
			return NULL;

		auto pte = read_phys<::pte>(
			reinterpret_cast<ppte>(pde.pfn << 12) + virt_addr.pt_index);

		entries.pt.first = reinterpret_cast<ppte>(pde.pfn << 12) + virt_addr.pt_index;
		entries.pt.second = pte;

		if (!pte.value)
			return NULL;

		return reinterpret_cast<void*>((pte.pfn << 12) + virt_addr.offset);
	}

	unsigned mem_ctx::get_pid() const
	{
		return pid;
	}

	void* mem_ctx::get_dirbase() const
	{
		return dirbase;
	}
}