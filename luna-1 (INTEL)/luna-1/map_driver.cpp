#include "map_driver.hpp"

namespace i6
{
	i6_error map_driver(std::uint8_t* drv_image, std::size_t image_size, void** entry_data)
	{
		std::vector<std::uint8_t> drv_buffer(drv_image, image_size + drv_image);
		if (!drv_buffer.size())
			return i6_error::image_invalid;

		nasa::kernel_ctx kernel;
		nasa::mem_ctx my_proc(kernel, GetCurrentProcessId());
		nasa::mapper_ctx mapper(my_proc, my_proc); // map driver into myself.

		const auto [drv_base, drv_entry] = mapper.map(drv_buffer);
		if (!drv_base || !drv_entry)
			return i6_error::init_failed;

		mapper.call_entry(drv_entry, entry_data);
		return i6_error::error_success;
	}
}