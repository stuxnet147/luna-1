#pragma once
#include <ntifs.h>
#include <windef.h>

enum com_type
{
	READ,
	WRITE,
	READ_KERNEL_MEMORY,
	WRITE_KERNEL_MEMORY,
	GET_PROCESS_BASE,
	GET_MODULE_BASE
};

typedef struct _com_struct
{
	com_type type;
	unsigned pid;
	unsigned size;
	void* data_from;
	void* data_to;
} com_struct, * pcom_struct;

extern "C" PVOID PsGetProcessSectionBaseAddress(
	__in PEPROCESS Process
);

extern "C" NTKERNELAPI NTSTATUS KeFlushCurrentTbImmediately();
extern "C" PPEB PsGetProcessPeb(PEPROCESS process);

extern "C" NTSTATUS MmCopyVirtualMemory(
	_In_ PEPROCESS FromProcess,
	_In_ CONST VOID* FromAddress,
	_In_ PEPROCESS ToProcess,
	_Out_opt_ PVOID ToAddress,
	_In_ SIZE_T BufferSize,
	_In_ KPROCESSOR_MODE PreviousMode,
	_Out_ PSIZE_T NumberOfBytesCopied
);

typedef struct _PEB_LDR_DATA 
{
	BYTE Reserved1[8];
	PVOID Reserved2[3];
	LIST_ENTRY InMemoryOrderModuleList;
} PEB_LDR_DATA, * PPEB_LDR_DATA;

typedef struct _LDR_DATA_TABLE_ENTRY 
{
	PVOID Reserved1[2];
	LIST_ENTRY InMemoryOrderLinks;
	PVOID Reserved2[2];
	PVOID DllBase;
	PVOID Reserved3[2];
	UNICODE_STRING FullDllName;
	BYTE Reserved4[8];
	PVOID Reserved5[3];
#pragma warning(push)
#pragma warning(disable: 4201) // we'll always use the Microsoft compiler
	union 
	{
		ULONG CheckSum;
		PVOID Reserved6;
	} DUMMYUNIONNAME;
#pragma warning(pop)
	ULONG TimeDateStamp;
} LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;

typedef struct _RTL_USER_PROCESS_PARAMETERS {
	BYTE Reserved1[16];
	PVOID Reserved2[10];
	UNICODE_STRING ImagePathName;
	UNICODE_STRING CommandLine;
} RTL_USER_PROCESS_PARAMETERS, * PRTL_USER_PROCESS_PARAMETERS;

typedef
VOID
(NTAPI* PPS_POST_PROCESS_INIT_ROUTINE) (
	VOID
);

typedef struct _PEB {
	BYTE Reserved1[2];
	BYTE BeingDebugged;
	BYTE Reserved2[1];
	PVOID Reserved3[2];
	PPEB_LDR_DATA Ldr;
	PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
	PVOID Reserved4[3];
	PVOID AtlThunkSListPtr;
	PVOID Reserved5;
	ULONG Reserved6;
	PVOID Reserved7;
	ULONG Reserved8;
	ULONG AtlThunkSListPtr32;
	PVOID Reserved9[45];
	BYTE Reserved10[96];
	PPS_POST_PROCESS_INIT_ROUTINE PostProcessInitRoutine;
	BYTE Reserved11[128];
	PVOID Reserved12[1];
	ULONG SessionId;
} PEB, * PPEB;

typedef union _virt_addr_t
{
    PVOID value;
    struct
    {
        ULONG64 offset : 12;
        ULONG64 pt_index : 9;
        ULONG64 pd_index : 9;
        ULONG64 pdpt_index : 9;
        ULONG64 pml4_index : 9;
        ULONG64 reserved : 16;
    };
} virt_addr_t, * pvirt_addr_t;
static_assert(sizeof(virt_addr_t) == sizeof(PVOID), "Size mismatch, only 64-bit supported.");

typedef union _pml4e
{
    ULONG64 value;
    struct
    {
        ULONG64 present : 1;          // Must be 1, region invalid if 0.
        ULONG64 ReadWrite : 1;        // If 0, writes not allowed.
        ULONG64 user_supervisor : 1;   // If 0, user-mode accesses not allowed.
        ULONG64 PageWriteThrough : 1; // Determines the memory type used to access PDPT.
        ULONG64 page_cache : 1; // Determines the memory type used to access PDPT.
        ULONG64 accessed : 1;         // If 0, this entry has not been used for translation.
        ULONG64 Ignored1 : 1;
        ULONG64 page_size : 1;         // Must be 0 for PML4E.
        ULONG64 Ignored2 : 4;
        ULONG64 pfn : 36; // The page frame number of the PDPT of this PML4E.
        ULONG64 Reserved : 4;
        ULONG64 Ignored3 : 11;
        ULONG64 nx : 1; // If 1, instruction fetches not allowed.
    };
} pml4e, * ppml4e;
static_assert(sizeof(pml4e) == sizeof(PVOID), "Size mismatch, only 64-bit supported.");

typedef union _pdpte
{
    ULONG64 value;
    struct
    {
        ULONG64 present : 1;          // Must be 1, region invalid if 0.
        ULONG64 rw : 1;        // If 0, writes not allowed.
        ULONG64 user_supervisor : 1;   // If 0, user-mode accesses not allowed.
        ULONG64 PageWriteThrough : 1; // Determines the memory type used to access PD.
        ULONG64 page_cache : 1; // Determines the memory type used to access PD.
        ULONG64 accessed : 1;         // If 0, this entry has not been used for translation.
        ULONG64 Ignored1 : 1;
        ULONG64 page_size : 1;         // If 1, this entry maps a 1GB page.
        ULONG64 Ignored2 : 4;
        ULONG64 pfn : 36; // The page frame number of the PD of this PDPTE.
        ULONG64 Reserved : 4;
        ULONG64 Ignored3 : 11;
        ULONG64 nx : 1; // If 1, instruction fetches not allowed.
    };
} pdpte, * ppdpte;
static_assert(sizeof(pdpte) == sizeof(PVOID), "Size mismatch, only 64-bit supported.");

typedef union _pde
{
    ULONG64 value;
    struct
    {
        ULONG64 present : 1;          // Must be 1, region invalid if 0.
        ULONG64 ReadWrite : 1;        // If 0, writes not allowed.
        ULONG64 user_supervisor : 1;   // If 0, user-mode accesses not allowed.
        ULONG64 PageWriteThrough : 1; // Determines the memory type used to access PT.
        ULONG64 page_cache : 1; // Determines the memory type used to access PT.
        ULONG64 Accessed : 1;         // If 0, this entry has not been used for translation.
        ULONG64 Ignored1 : 1;
        ULONG64 page_size : 1; // If 1, this entry maps a 2MB page.
        ULONG64 Ignored2 : 4;
        ULONG64 pfn : 36; // The page frame number of the PT of this PDE.
        ULONG64 Reserved : 4;
        ULONG64 Ignored3 : 11;
        ULONG64 nx : 1; // If 1, instruction fetches not allowed.
    };
} pde, * ppde;
static_assert(sizeof(pde) == sizeof(PVOID), "Size mismatch, only 64-bit supported.");

typedef union _pte
{
    ULONG64 value;
    struct
    {
        ULONG64 present : 1;          // Must be 1, region invalid if 0.
        ULONG64 ReadWrite : 1;        // If 0, writes not allowed.
        ULONG64 user_supervisor : 1;   // If 0, user-mode accesses not allowed.
        ULONG64 PageWriteThrough : 1; // Determines the memory type used to access the memory.
        ULONG64 page_cache : 1; // Determines the memory type used to access the memory.
        ULONG64 accessed : 1;         // If 0, this entry has not been used for translation.
        ULONG64 Dirty : 1;            // If 0, the memory backing this page has not been written to.
        ULONG64 PageAccessType : 1;   // Determines the memory type used to access the memory.
        ULONG64 Global : 1;           // If 1 and the PGE bit of CR4 is set, translations are global.
        ULONG64 Ignored2 : 3;
        ULONG64 pfn : 36; // The page frame number of the backing physical page.
        ULONG64 Reserved : 4;
        ULONG64 Ignored3 : 7;
        ULONG64 ProtectionKey : 4;  // If the PKE bit of CR4 is set, determines the protection key.
        ULONG64 nx : 1; // If 1, instruction fetches not allowed.
    };
} pte, * ppte;
static_assert(sizeof(pte) == sizeof(PVOID), "Size mismatch, only 64-bit supported.");

typedef union _cr3
{
    ULONG64 flags;
    struct
    {
        ULONG64 reserved1 : 3;
        ULONG64 page_level_write_through : 1;
        ULONG64 page_level_cache_disable : 1;
        ULONG64 reserved2 : 7;
        ULONG64 dirbase : 36;
        ULONG64 reserved3 : 16;
    };
} cr3;

typedef struct _CURSOR_PAGE
{
    void* page; // virtual address
    ppte  pte;
    unsigned org_pfn; // original pfn
} CURSOR_PAGE, *PCURSOR_PAGE;