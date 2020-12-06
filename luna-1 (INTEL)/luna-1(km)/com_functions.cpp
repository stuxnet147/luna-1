#include "com_functions.h"
#include "memory.h"

namespace i6
{
	namespace com
	{
		void get_process_base(const pcom_struct com_data)
		{
			if (!com_data || !com_data->pid)
				return;

			PEPROCESS peproc;
			if (PsLookupProcessByProcessId((HANDLE)com_data->pid, &peproc) != STATUS_SUCCESS)
				return;

			com_data->data_from = PsGetProcessSectionBaseAddress(peproc);
			ObDereferenceObject(peproc);
		}

		void get_module_base(const pcom_struct com_data)
		{
			if (!com_data || !com_data->pid || !com_data->data_to)
				return;

			PEPROCESS peproc;
			if (PsLookupProcessByProcessId((HANDLE)com_data->pid, &peproc) != STATUS_SUCCESS)
				return;

			auto ppeb = PsGetProcessPeb(peproc);
			ObDereferenceObject(peproc);
			if (!ppeb)
				return;

			PEB peb;
			i6::memory::read(
				ppeb,
				&peb,
				sizeof(peb),
				com_data->pid
			);

			if (!peb.Ldr)
				return;

			PEB_LDR_DATA module_list_entry;
			i6::memory::read(
				peb.Ldr,
				&module_list_entry,
				sizeof(module_list_entry),
				com_data->pid
			);

			auto first_entry = (void*)module_list_entry.InMemoryOrderModuleList.Flink;
			unsigned char* current_entry;
			i6::memory::read(
				first_entry, 
				&current_entry,
				sizeof(current_entry),
				com_data->pid
			);

			WCHAR full_file_name[MAX_PATH];
			ULONGLONG module_base;
			ULONGLONG file_name_ptr;

			while (current_entry != first_entry)
			{
				i6::memory::read(
					(unsigned char*)(current_entry)+0x40,
					&file_name_ptr,
					sizeof(file_name_ptr),
					com_data->pid
				); // read full module unicode_string structure.

				i6::memory::read(
					(void*)file_name_ptr,
					full_file_name,
					MAX_PATH,
					com_data->pid
				); // read full file path.

				i6::memory::read(
					(unsigned char*)(current_entry)+0x20,
					&module_base,
					sizeof(module_base),
					com_data->pid
				);

				if (wcsstr(full_file_name, (wchar_t*)com_data->data_to))
				{
					com_data->data_from = reinterpret_cast<void*>(module_base);
					return;
				}

				i6::memory::read(
					current_entry,
					&current_entry,
					sizeof(current_entry),
					com_data->pid
				);
			}
		}

		void read_process_memory(const pcom_struct com_data)
		{
			if (!com_data || !com_data->pid || !com_data->data_from || !com_data->data_to || !com_data->size)
				return;

			i6::memory::read(
				com_data->data_from,
				com_data->data_to,
				com_data->size,
				com_data->pid
			);
		}

		void write_process_memory(const pcom_struct com_data)
		{
			if (!com_data || !com_data->pid || !com_data->data_from || !com_data->data_to || !com_data->size)
				return;

			i6::memory::write
			(
				com_data->data_to,
				com_data->data_from,
				com_data->size, 
				com_data->pid
			);
		}

		void read_kernel_memory(const pcom_struct com_data)
		{
			if (!com_data || !com_data->data_from || !com_data->data_to)
				return;

			memcpy
			(
				com_data->data_to,
				com_data->data_from,
				com_data->size
			);
		}

		void write_kernel_memory(const pcom_struct com_data)
		{
			if (!com_data || !com_data->data_from || !com_data->data_to)
				return;

			memcpy
			(
				com_data->data_to,
				com_data->data_from,
				com_data->size
			);
		}
	}
}