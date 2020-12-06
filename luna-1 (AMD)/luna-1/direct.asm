_TEXT  SEGMENT
	__protect_virtual_memory proc
		mov     r10, rcx
		mov     eax, 050h
		syscall
		ret
	__protect_virtual_memory endp

	__write_virtual_memory proc
		mov     r10, rcx
		mov     eax, 03Ah
		syscall
		ret
	__write_virtual_memory endp

	__read_virtual_memory proc
		mov     r10, rcx
		mov     eax, 03Fh
		syscall
		ret
	__read_virtual_memory endp

	__alloc_virtual_memory proc
		mov     r10, rcx
		mov		eax, 018h
		syscall
		ret
	__alloc_virtual_memory endp
_TEXT  ENDS
end