#pragma once
#include "../mem_ctx/mem_ctx.hpp"
#include "../pe_image/pe_image.h"
#include "../direct.h"

#define PML4_MAP_INDEX 153
#define SELF_REF_ENTRY 69

namespace nasa
{
	class mapper_ctx
	{
	public:
		explicit mapper_ctx(
			nasa::mem_ctx& map_into, 
			nasa::mem_ctx& map_from
		);
		std::pair<void*, void*> map(std::vector<std::uint8_t>& raw_image);
		bool call_entry(void* drv_entry, void** hook_handler) const;
	private:
		std::pair<void*, void*> allocate_driver(std::vector<std::uint8_t>& raw_image);
		void* create_self_ref(void* virt_alloc_base);
		void make_ptes_kernel_access(void* drv_base, std::size_t drv_size);
		nasa::mem_ctx map_into;
		nasa::mem_ctx map_from;
	};
}