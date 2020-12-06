#include "memory.h"

namespace i6
{
    namespace memory
    {
        CURSOR_PAGE get_cursor()
        {
            const auto cursor = 
                ExAllocatePool(
                    NonPagedPool,
                    0x1000
                );

            memset(cursor, NULL, 0x1000);
            virt_addr_t addr_t{ cursor };

            const auto dirbase = 
                ::cr3{ __readcr3() }.dirbase;

            const auto pml4 =
                reinterpret_cast<ppml4e>(
                    MmGetVirtualForPhysical(
                        PHYSICAL_ADDRESS{ (LONGLONG)dirbase << 12 })) + addr_t.pml4_index;

            if (!MmIsAddressValid(pml4))
                return {};

            const auto pdpt =
                reinterpret_cast<ppdpte>(
                    MmGetVirtualForPhysical(
                        PHYSICAL_ADDRESS{ (LONGLONG)pml4->pfn << 12 })) + addr_t.pdpt_index;

            if (!MmIsAddressValid(pdpt))
                return {};

            const auto pd =
                reinterpret_cast<ppde>(
                    MmGetVirtualForPhysical(
                        PHYSICAL_ADDRESS{ (LONGLONG)pdpt->pfn << 12 })) + addr_t.pd_index;

            if (!MmIsAddressValid(pd))
                return {};

            const auto pt =
                reinterpret_cast<ppte>(
                    MmGetVirtualForPhysical(
                        PHYSICAL_ADDRESS{ (LONGLONG)pd->pfn << 12 })) + addr_t.pt_index;

            if (!MmIsAddressValid(pt))
                return {};

            return { cursor, pt, (unsigned)pt->pfn };
        }

        void* virt_to_phys(unsigned pid, void* addr)
        {
            if (!pid || !addr)
                return {};

            PEPROCESS peproc;
            if (NT_SUCCESS(PsLookupProcessByProcessId((HANDLE)pid, &peproc)) && peproc)
            {
                virt_addr_t addr_t{ addr };
                static const auto cursor = get_cursor();

                if (!cursor.pte || !cursor.org_pfn || !cursor.page)
                {
                    ObDereferenceObject(peproc);
                    return {};
                }

                const auto dirbase = // dirbase is a pfn
                    *reinterpret_cast<pte*>(
                        reinterpret_cast<ULONGLONG>(peproc) + 0x28);

                {
                    cursor.pte->pfn = dirbase.pfn;
                    KeFlushCurrentTbImmediately();
                }

                if (!MmIsAddressValid(reinterpret_cast<ppml4e>(cursor.page) + addr_t.pml4_index))
                {
                    cursor.pte->pfn = cursor.org_pfn;
                    KeFlushCurrentTbImmediately();
                    ObDereferenceObject(peproc);
                    return {};
                }

                KeFlushCurrentTbImmediately();
                const auto pml4e = 
                    *reinterpret_cast<::pml4e*>(
                        reinterpret_cast<ppml4e>(cursor.page) + addr_t.pml4_index);

                if (!pml4e.value || !pml4e.present || !pml4e.pfn)
                {
                    cursor.pte->pfn = cursor.org_pfn;
                    KeFlushCurrentTbImmediately();
                    ObDereferenceObject(peproc);
                    return {};
                }

                {
                    cursor.pte->pfn = pml4e.pfn;
                    KeFlushCurrentTbImmediately();
                }

                if (!MmIsAddressValid(reinterpret_cast<ppdpte>(cursor.page) + addr_t.pdpt_index))
                {
                    cursor.pte->pfn = cursor.org_pfn;
                    KeFlushCurrentTbImmediately();
                    ObDereferenceObject(peproc);
                    return {};
                }

                KeFlushCurrentTbImmediately();
                const auto pdpte =
                    *reinterpret_cast<::pdpte*>(
                        reinterpret_cast<ppdpte>(cursor.page) + addr_t.pdpt_index);

                if (!pdpte.value || !pdpte.present || !pdpte.pfn)
                {
                    cursor.pte->pfn = cursor.org_pfn;
                    KeFlushCurrentTbImmediately();
                    ObDereferenceObject(peproc);
                    return {};
                }

                {
                    cursor.pte->pfn = pdpte.pfn;
                    KeFlushCurrentTbImmediately();
                }

                if (!MmIsAddressValid(reinterpret_cast<ppde>(cursor.page) + addr_t.pd_index))
                {
                    cursor.pte->pfn = cursor.org_pfn;
                    KeFlushCurrentTbImmediately();
                    ObDereferenceObject(peproc);
                    return {};
                }

                KeFlushCurrentTbImmediately();
                const auto pde = 
                    *reinterpret_cast<::pde*>(
                        reinterpret_cast<ppde>(cursor.page) + addr_t.pd_index);

                if (!pde.value || !pde.present || !pde.pfn)
                {
                    cursor.pte->pfn = cursor.org_pfn;
                    KeFlushCurrentTbImmediately();
                    ObDereferenceObject(peproc);
                    return {};
                }

                {
                    cursor.pte->pfn = pde.pfn;
                    KeFlushCurrentTbImmediately();
                }

                if (!MmIsAddressValid(reinterpret_cast<ppte>(cursor.page) + addr_t.pt_index))
                {
                    cursor.pte->pfn = cursor.org_pfn;
                    KeFlushCurrentTbImmediately();
                    ObDereferenceObject(peproc);
                    return {};
                }

                KeFlushCurrentTbImmediately();
                const auto pte = 
                    *reinterpret_cast<::pte*>(
                        reinterpret_cast<ppte>(cursor.page) + addr_t.pt_index);

                if (!pte.value || !pte.present || !pte.pfn)
                {
                    cursor.pte->pfn = cursor.org_pfn;
                    KeFlushCurrentTbImmediately();
                    ObDereferenceObject(peproc);
                    return {};
                }
                
                //
                // reset pfn
                //
                {
                    cursor.pte->pfn = cursor.org_pfn;
                    KeFlushCurrentTbImmediately();
                }

                ObDereferenceObject(peproc);
                return reinterpret_cast<void*>((pte.pfn << 12) + addr_t.offset);
            }
            return {};
        }

        void read(void* addr, void* buffer, size_t size, unsigned pid)
        {
            if (!addr || !MmIsAddressValid(buffer) || !size || !pid)
                return;

            const auto virt_addr =
                MmMapIoSpace(
                    PHYSICAL_ADDRESS{ (LONGLONG)virt_to_phys(pid, addr) },
                    size + 0x1000,
                    MmNonCached
                );

            if (!MmIsAddressValid(virt_addr))
                return;

            memcpy(buffer, virt_addr, size);
            MmUnmapIoSpace(virt_addr, size + 0x1000);
        }

        void write(void* addr, void* buffer, size_t size, unsigned pid)
        {
            if (!addr || !MmIsAddressValid(buffer) || !size || !pid)
                return;

            const auto virt_addr =
                MmMapIoSpace(
                    PHYSICAL_ADDRESS{ (LONGLONG)virt_to_phys(pid, addr) },
                    size + 0x1000,
                    MmNonCached
                );

            if (!MmIsAddressValid(virt_addr))
                return;

            memcpy(virt_addr, buffer, size);
            MmUnmapIoSpace(virt_addr, size + 0x1000);
        }
    }
}