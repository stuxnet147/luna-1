#pragma once
#include "types.h"

namespace i6
{
	namespace com
	{
		void get_process_base(const pcom_struct com_data);
		void get_module_base(const pcom_struct com_data);
		void read_process_memory(const pcom_struct com_data);
		void write_process_memory(const pcom_struct com_data);
		void read_kernel_memory(const pcom_struct com_data);
		void write_kernel_memory(const pcom_struct com_data);
	}
}