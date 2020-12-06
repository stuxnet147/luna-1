<div align="center">
    <img src="https://imgur.com/KWW5wuV.png"/>
</div>

# Project Info

This Project is over six months old now I think its time I release it as a reference. This project was created to test highly theoretical page table manipulation
ideas. It was the building blocks for projects such as: PTM, PSKP, PSKDM, pclone, reverse-injector, /proc/kmem, and unreleased at this point in time `hyperspace`. 
Although the project is unstable, it was an important learning project that helped put ideas in my head into functional code that runs well on windows.

### Patch Guard
This is an old project that tested many of my theoretical page table manipulation concepts. This project was created in part to find bugs/problems with my theories
and to be used as a reference for future projects. It was not ment to be used for anything else. From this project I have learned that PSKP (Process-Context Specific Kernel Patches) 
is not patch guard friendly. Patch guard does indeed check the kernel PML4E's to ensure they are pointing at valid PDPT's. Although triggering patchguard has never been
done before personally. This leads me to my second patchguard related conclusion; using this to patch ntoskrnl.exe does not bypass patchguard as patch guard can run in
all address spaces. Reguardless I have never been able to trip patchguard on these detections, I've ran this is code in a VM for over 48 hours doing a simple patch to
ntoskrnl.exe.

### Access Violations
Another note about PSKP is that when memory is allocated in the new PDPT, PD, and PT it is not being mapped into other processes kernel mappings. This means its possible
to crash the system by allocating memory, KeStackAttaching and then accessing that memory (since its not mapped into the process you KeStackAttached too). Any function
that uses KeStackAttachProcess can cause an access violation and thus a crash. MmCopyVirtualMemory allocates a pool and then calls KeStackAttachProcess. This was the 
reason i manually walk the paging tables and map the physical memory into virtual memory.

### TLB And Pre-PTM Page Table Library
Since this project uses a very very old version of PTM, before PTM was every made, it uses a different technique to map physical memory into virtual memory. 
The code in this project changes a PTE of a VirtualAlloc'ed page to point at another VirtualAlloc'ed pages PT. This allows the library to change the second
VirtualAlloc'ed pages PFN from usermode. If the original PFN is not restored before the program closes (and all virtual memory is unmapped), a crash will happen (PFN Corruption).
Also dealing with the TLB was a pain in the ass, the TLB was such an issue I created a new technique to get around it which is used in PTM. I generate new
virtual addresses on the fly now and ensure they are accessable.

# Detection

### Kernel PML4E PFN Discrepancies
Both projects can be detected by enumorating page tables for changes in kernel PML4E page frame numbers. All process-context kernel mappings point to the same PDPTs unless
explicitly changed (by PSKP which both projects use). You can also explicitly detect the intel varient of this project by enumorating all processes for extra kernel PML4E's
or kernel PML4E's in usermode part of the PML4. 

Kernel PML4E's all point to the same PDPT's besides the self referencing PML4E...
```
// notepad's kernel mappings....
//....
pml4e at -> 267 (0x0000000092A1D858)
        - pfn: 0xb579
        - writeable: 1
        - executable: 1
pml4e at -> 268 (0x0000000092A1D860)
        - pfn: 0xb57a
        - writeable: 1
        - executable: 1
pml4e at -> 269 (0x0000000092A1D868)
        - pfn: 0xb57b
        - writeable: 1
        - executable: 1
pml4e at -> 270 (0x0000000092A1D870)
        - pfn: 0xb57c
        - writeable: 1
        - executable: 1
pml4e at -> 271 (0x0000000092A1D878)
        - pfn: 0xb57d
        - writeable: 1
        - executable: 1
pml4e at -> 272 (0x0000000092A1D880)
        - pfn: 0xb57e
        - writeable: 1
        - executable: 1
//....
```

And here is PTM.exe kernel PML4E's:

```
// ...
pml4e at -> 267 (0x0000000127957858)
        - pfn: 0xb579 <============ same PFN as notepad only changes explicitly by PSKP
        - writeable: 1
        - executable: 1
pml4e at -> 268 (0x0000000127957860)
        - pfn: 0xb57a <============ same PFN as notepad only changes explicitly by PSKP
        - writeable: 1
        - executable: 1
pml4e at -> 269 (0x0000000127957868)
        - pfn: 0xb57b <============ same PFN as notepad only changes explicitly by PSKP
        - writeable: 1
        - executable: 1
pml4e at -> 270 (0x0000000127957870)
        - pfn: 0xb57c <============ same PFN as notepad only changes explicitly by PSKP
        - writeable: 1
        - executable: 1
pml4e at -> 271 (0x0000000127957878)
        - pfn: 0xb57d <============ same PFN as notepad only changes explicitly by PSKP
        - writeable: 1
        - executable: 1
pml4e at -> 272 (0x0000000127957880)
        - pfn: 0xb57e <============ same PFN as notepad only changes explicitly by PSKP
        - writeable: 1
        - executable: 1
// ...
```

### Simple Page Table Checks

You can simply call `NtQuerySystemInformation` - `SystemProcessInformation` and enumorate all running processes (BattlEye already does this) to obtain each processes PID.

```cpp
typedef struct _SYSTEM_PROCESS_INFORMATION {



  ULONG                   NextEntryOffset;
  ULONG                   NumberOfThreads;
  LARGE_INTEGER           Reserved[3];
  LARGE_INTEGER           CreateTime;
  LARGE_INTEGER           UserTime;
  LARGE_INTEGER           KernelTime;
  UNICODE_STRING          ImageName;
  KPRIORITY               BasePriority;
  HANDLE                  ProcessId;
  HANDLE                  InheritedFromProcessId;
  ULONG                   HandleCount;
  ULONG                   Reserved2[2];
  ULONG                   PrivatePageCount;
  VM_COUNTERS             VirtualMemoryCounters;
  IO_COUNTERS             IoCounters;
  SYSTEM_THREAD           Threads[0];

} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;
```

While looping over process information structures you can `PsLookupProcessByProcessId` to obtain the PEPROCESS, then add 0x28 to this PEPROCESS to get the DirectoryTableBase of
that process. You can KeStackAttachProcess to this process and call MmGetVirtualForPhysical to get the hyperspace mappings of this processes PML4. This hyperspace address
is the virtual address of the PML4, you can simply scan this for inconsistant PML4E->PFN's as described above. You can also scan for kernel memory in usermode.

This is a simple example of checking for kernel memory in usermode and for invalid PFN's. Keep in mind that this will not check KVA shadowing usermode PML4.
```cpp
// for loop enumorating over SYSTEM_PROCESS_INFORMATION results...
KeStackAttachProcess(DesiredProcess, &ApcState);
UINT64 ProcessPml4 = (UINT64)(*(PVOID*)(DesiredProcess + 0x28));
PPML4E HyperSpacePml4 = MmGetVirtualForPhysical(ProcessPml4);
{
    // check to see if there is kernel memory in usermode...
    for (UINT16 idx = 0; idx < 256; ++idx)
        if(!HyperSpacePml4[idx].UserSuperVisor)
            // kernel memory was found in usermode...
            
    // check to see if there is invalid PML4E->Pfn's
    for (UINT16 idx = 256; idx < 512; ++idx)
        // checks to see if the PFN is invalid & if the entry is not the self referencing entry...
        // you should also check if there is multiple self ref entries as that is invalid...
        if(HyperSpacePml4[idx].Pfn != KernelPml4es[idx].Pfn && HyperSpacePml4[idx].Pfn != ProcessPml4 >> 12)
            // PSKP detected...
}
KeUnstackDetachProcess(&ApcState);
```

### Simple Kernel Pool Scans (AMD Version)

The AMD varient of this project is already detected by EAC as its allocated in a kernel pool with no protections. Simple scans for `sub rsp, 28h`, `add rsp ?, ret` will
detect the mapped driver. You can also scan for `E8 ? ? ? ?` for calls that land inside of the same pool or land inside of a loaded kernel module. 

Both versions of this project are highly unstable due to the face that they both use an out dated version of PSKP, PTM (not created yet), and VDM (not created yet). 
This repo should serve as a reference rather then working code, after all it is luna-1, a probe to test how these theoretical concepts would play out.

You can `NtQuerySystemInformation` - `SystemBigPoolInformation` for allocated kernel pools which the AMD version of this project will be in.

```cpp
typedef struct _SYSTEM_BIGPOOL_ENTRY 
{
	union {
		PVOID VirtualAddress;
		ULONG_PTR NonPaged : 1;
	};
	ULONG_PTR SizeInBytes;
	union {
		UCHAR Tag[4];
		ULONG TagUlong;
	};
} SYSTEM_BIGPOOL_ENTRY, *PSYSTEM_BIGPOOL_ENTRY;
```

While enumorating over these pools you can scan the pools for `sub rsp, 28h`, and `add rsp ?, ret`. You can also scan for `E8 ? ? ? ?` (call xxxx) that lands inside of the pool
itself or lands inside of a loaded kernel module. 