#include "com_functions.h"

void hook_handler(pcom_struct com_data)
{
	if (!com_data)
		return;

	switch (com_data->type)
	{
	case READ:
		i6::com::read_process_memory(com_data);
		break;
	case WRITE:
		i6::com::write_process_memory(com_data);
		break;
	case WRITE_KERNEL_MEMORY:
		i6::com::write_kernel_memory(com_data);
		break;
	case READ_KERNEL_MEMORY:
		i6::com::read_kernel_memory(com_data);
		break;
	case GET_PROCESS_BASE:
		i6::com::get_process_base(com_data);
		break;
	case GET_MODULE_BASE:
		i6::com::get_module_base(com_data);
		break;
	default:
		break;
	}
}

NTSTATUS driver_entry(void** data_ptr)
{
	*data_ptr = &hook_handler;
	return STATUS_SUCCESS;
}