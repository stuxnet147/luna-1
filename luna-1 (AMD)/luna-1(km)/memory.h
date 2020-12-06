#pragma once
#include <intrin.h>
#include "types.h"

namespace i6
{
	namespace memory
	{
		void disable_wp();
        void enable_wp();

		void read(void* addr, void* buffer, size_t size, unsigned pid);
		void write(void* addr, void* buffer, size_t size, unsigned pid);
	}
}