# Old code from 6/xx/2020.

### Detection

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

The AMD varient of this project is already detected by EAC as its allocated in a kernel pool with no protections. Simple scans for `sub rsp, 28h`, `add rsp ?, ret` will
detect the mapped driver. You can also scan for `E8 ? ? ? ?` for calls that land inside of the same pool or land inside of a loaded kernel module. 

Both versions of this project are highly unstable due to the face that they both use an out dated version of PSKP, PTM (not created yet), and VDM (not created yet). 
This repo should serve as a reference rather then working code, after all it is luna-1, a probe to test how these theoretical concepts would play out.

### luna-1 (AMD)

Driver gets allocated inside of the kernel using a normal pool. The Nt headers of the driver are zero'ed. Communication with this driver happens via a process specific
syscall hook (meaning the hook cannot be seen in any other context). Detected on EAC, should be fine for BattlEye.


### luna-1 (INTEL)

Driver gets allocated inside of the current process (not the kernel itself) and makes a process specific syscall hook to communicate (just like the AMD one). The AMD luna-1
also works for intel. This project is using a super old version of PSKDM which is not stable, its also using an old version of PTM and its using physmeme instead of VDM.

Not sure if EAC is enumorating all processes PML4's yet, when they do this will be detected. Should be fine for BattlEye.

