# Old code from 6/xx/2020.

### Detection

Both projects can be detected by enumorating page tables for changes in kernel PML4E page frame numbers. All process-context kernel mappings point to the same PDPTs unless
explicitly changed (by PSKP which both projects use). You can also explicitly detect the intel varient of this project by enumorating all processes for extra kernel PML4E's
or kernel PML4E's in usermode part of the PML4. The AMD varient of this project is already detected by EAC as its allocated in a kernel pool with no protections. 

Both versions of this project are highly unstable due to the face that they both use an out dated version of PSKP, PTM (not created yet), and VDM (not created yet). 
This repo should serve as a reference rather then working code, after all it is luna-1, a probe to test how these theoretical concepts would play out.

### luna-1 (AMD)

Driver gets allocated inside of the kernel using a normal pool. The Nt headers of the driver are zero'ed. Communication with this driver happens via a process specific
syscall hook (meaning the hook cannot be seen in any other context). Detected on EAC, should be fine for BattlEye.


### luna-1 (INTEL)

Driver gets allocated inside of the current process (not the kernel itself) and makes a process specific syscall hook to communicate (just like the AMD one). The AMD luna-1
also works for intel. This project is using a super old version of PSKDM which is not stable, its also using an old version of PTM and its using physmeme instead of VDM.

Not sure if EAC is enumorating all processes PML4's yet, when they do this will be detected. Should be fine for BattlEye.

