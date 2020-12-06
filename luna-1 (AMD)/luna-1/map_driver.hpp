#pragma once
#include <vector>

namespace nasa
{
	bool __cdecl map_driver(std::vector<std::uint8_t>& raw_driver, void** hook_handler);
	bool __cdecl map_driver(std::uint8_t* image, std::size_t size, void** hook_handler);
}