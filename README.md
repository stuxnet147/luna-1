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

### Simple Kernel Pool Scans (AMD Version)

The AMD varient of this project is already detected by EAC as its allocated in a kernel pool with no protections. Simple scans for `sub rsp, 28h`, `add rsp ?, ret` will
detect the mapped driver. You can also scan for `E8 ? ? ? ?` for calls that land inside of the same pool or land inside of a loaded kernel module. 

Both versions of this project are highly unstable due to the face that they both use an out dated version of PSKP, PTM (not created yet), and VDM (not created yet). 
This repo should serve as a reference rather then working code, after all it is luna-1, a probe to test how these theoretical concepts would play out.

# Project Info

This is an old project that tested many of my theoretical page table manipulation concepts. This project was created in part to find bugs/problems with my theories
and to be used as a reference for future projects. It was not ment to be used for anything else. From this project I have learned that PSKP (Process-Context Specific Kernel Patches) 
is not page guard friendly. Patch guard does indeed check the kernel PML4E's to ensure they are pointing at valid PDPT's. Although triggering patchguard has never been
done before personally. This leads me to my second patchguard related conclusion; using this to patch ntoskrnl.exe does not bypass patchguard as patch guard can run in
all address spaces. Reguardless I have never been able to trip patchguard on these detections, I've ran this is code in a VM for over 48 hours doing a simple patch to
ntoskrnl.exe.

Another note about PSKP is that when memory is allocated in the new PDPT, PD, and PT it is not being mapped into other processes kernel mappings. This means its possible
to crash the system by allocating memory, KeStackAttaching and then accessing that memory (since its not mapped into the process you KeStackAttached too). Any function
that uses KeStackAttachProcess can cause an access violation and thus a crash. MmCopyVirtualMemory allocates a pool and then calls KeStackAttachProcess. This was the 
reason i manually walk the paging tables and map the physical memory into virtual memory.

Since this project uses a very very old version of PTM, before PTM was every made, it uses a different technique to map physical memory into virtual memory. 
The code in this project changes a PTE of a VirtualAlloc'ed page to point at another VirtualAlloc'ed pages PT. This allows the library to change the second
VirtualAlloc'ed pages PFN from usermode. 

### luna-1 (AMD)

Driver gets allocated inside of the kernel using a normal pool. The Nt headers of the driver are zero'ed. Communication with this driver happens via a process specific
syscall hook (meaning the hook cannot be seen in any other context). Detected on EAC, should be fine for BattlEye.

Since with PSKP MmCopyVirtualMemory can cause issues with memory not being present in one address space and is present in another, the page tables are manually walked
and the physical page is directly written too. This works well, all memory is writeable since page protections are bypassed. 

### luna-1 (INTEL)

Driver gets allocated inside of the current process (not the kernel itself) and makes a process specific syscall hook to communicate (just like the AMD one). The AMD luna-1
also works for intel. This project is using a super old version of PSKDM which is not stable, its also using an old version of PTM and its using physmeme instead of VDM. Not sure if EAC is enumorating all processes PML4's yet, when they do this will be detected. Should be fine for BattlEye.

Since with PSKP MmCopyVirtualMemory can cause issues with memory not being present in one address space and is present in another, the page tables are manually walked
and the physical page is directly written too. This works well, all memory is writeable since page protections are bypassed. 
