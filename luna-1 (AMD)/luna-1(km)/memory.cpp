#include "memory.h"

namespace i6
{
    namespace memory
    {
        void read(void* addr, void* buffer, size_t size, unsigned pid)
        {
            if (!addr || !buffer || !size)
                return;

            char memcpy_buffer[0x1000];
            memset(memcpy_buffer, NULL, sizeof(memcpy_buffer));

            KAPC_STATE state;
            PEPROCESS peproc;
            if (NT_SUCCESS(PsLookupProcessByProcessId((HANDLE)pid, &peproc)) && peproc)
            {
                KeStackAttachProcess(peproc, &state);
                if (MmIsAddressValid(addr)) 
                    memcpy(memcpy_buffer, addr, size);

                KeUnstackDetachProcess(&state);
                if(MmIsAddressValid(buffer)) 
                    memcpy(buffer, memcpy_buffer, size);
                ObDereferenceObject(peproc);
            }
        }

        void write(void* addr, void* buffer, size_t size, unsigned pid)
        {
            if (!addr || !buffer || !size)
                return;

            char memcpy_buffer[0x1000];
            memset(memcpy_buffer, NULL, sizeof(memcpy_buffer));

            if (MmIsAddressValid(buffer)) 
                memcpy(memcpy_buffer, buffer, size);

            KAPC_STATE state;
            PEPROCESS peproc;
            if (NT_SUCCESS(PsLookupProcessByProcessId((HANDLE)pid, &peproc)) && peproc)
            {
                KeStackAttachProcess(peproc, &state);
                if (MmIsAddressValid(addr)) 
                    memcpy(addr, memcpy_buffer, size);

                KeUnstackDetachProcess(&state);
                ObDereferenceObject(peproc);
            }
        }

        void disable_wp()
        {
            _disable();
            auto cr0 = __readcr0();
            cr0 &= 0xfffffffffffeffff;
            __writecr0(cr0);
        }

        void enable_wp()
        {
            auto cr0 = __readcr0();
            cr0 |= 0x10000;
            __writecr0(cr0);
            _enable();
        }
    }
}