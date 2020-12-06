# Old code from 6/xx/2020.
### luna-1 (AMD)

Driver gets allocated inside of the kernel using a normal pool. The Nt headers of the driver are zero'ed. Communication with this driver happens via a process specific
syscall hook (meaning the hook cannot be seen in any other context). Detected on EAC, should be fine for BattlEye.


### luna-1 (INTEL)

Driver gets allocated inside of the current process (not the kernel itself) and makes a process specific syscall hook to communicate (just like the AMD one). The AMD luna-1
also works for intel. This project is using a super old version of PSKDM which is not stable, its also using an old version of PTM and its using physmeme instead of VDM.

Not sure if EAC is enumorating all processes PML4's yet, when they do this will be detected. Should be fine for BattlEye.

