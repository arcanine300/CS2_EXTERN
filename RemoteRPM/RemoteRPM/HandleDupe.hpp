#include <Windows.h> 
#include <iostream>
#include <TlHelp32.h>
#include <string>

#define SeDebugPriv 20
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004)
#define NtCurrentProcess ( (HANDLE)(LONG_PTR) -1 ) 
#define ProcessHandleType 0x7
#define SystemHandleInformation 16 

typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWCH   Buffer;
} UNICODE_STRING, * PUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES {
	ULONG           Length;
	HANDLE          RootDirectory;
	PUNICODE_STRING ObjectName;
	ULONG           Attributes;
	PVOID           SecurityDescriptor;
	PVOID           SecurityQualityOfService;
}  OBJECT_ATTRIBUTES, * POBJECT_ATTRIBUTES;

typedef struct _CLIENT_ID {
	PVOID UniqueProcess;
	PVOID UniqueThread;
} CLIENT_ID, * PCLIENT_ID;

typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO {
	ULONG ProcessId;
	BYTE ObjectTypeNumber;
	BYTE Flags;
	USHORT Handle;
	PVOID Object;
	ACCESS_MASK GrantedAccess;
} SYSTEM_HANDLE, * PSYSTEM_HANDLE;

typedef struct _SYSTEM_HANDLE_INFORMATION {
	ULONG HandleCount;
	SYSTEM_HANDLE Handles[1];
} SYSTEM_HANDLE_INFORMATION, * PSYSTEM_HANDLE_INFORMATION;

typedef NTSTATUS(NTAPI* _NtDuplicateObject)(
	HANDLE SourceProcessHandle,
	HANDLE SourceHandle,
	HANDLE TargetProcessHandle,
	PHANDLE TargetHandle,
	ACCESS_MASK DesiredAccess,
	ULONG Attributes,
	ULONG Options
	);

typedef NTSTATUS(NTAPI* _RtlAdjustPrivilege)(
	ULONG Privilege,
	BOOLEAN Enable,
	BOOLEAN CurrentThread,
	PBOOLEAN Enabled
	);

typedef NTSYSAPI NTSTATUS(NTAPI* _NtOpenProcess)(
	PHANDLE            ProcessHandle,
	ACCESS_MASK        DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PCLIENT_ID         ClientId
	);

typedef NTSTATUS(NTAPI* _NtQuerySystemInformation)(
	ULONG SystemInformationClass,
	PVOID SystemInformation,
	ULONG SystemInformationLength,
	PULONG ReturnLength
	);

SYSTEM_HANDLE_INFORMATION* hInfo;

namespace hj {
	OBJECT_ATTRIBUTES InitObjectAttributes(PUNICODE_STRING name, ULONG attributes, HANDLE hRoot, PSECURITY_DESCRIPTOR security) {
		OBJECT_ATTRIBUTES object;
		object.Length = sizeof(OBJECT_ATTRIBUTES);
		object.ObjectName = name;
		object.Attributes = attributes;
		object.RootDirectory = hRoot;
		object.SecurityDescriptor = security;
		return object;
	}

	bool IsHandleValid(HANDLE handle) {
		if (handle && handle != INVALID_HANDLE_VALUE) {
			return true;
		}
		else {
			return false;
		}
	}

	HANDLE HijackExistingHandle(DWORD dwTargetProcessId) {
		HANDLE procHandle = NULL;
		HANDLE HijackedHandle = NULL;
		HMODULE Ntdll = GetModuleHandleA("ntdll");
		_RtlAdjustPrivilege RtlAdjustPrivilege = (_RtlAdjustPrivilege)GetProcAddress(Ntdll, "RtlAdjustPrivilege");
		_NtQuerySystemInformation NtQuerySystemInformation = (_NtQuerySystemInformation)GetProcAddress(Ntdll, "NtQuerySystemInformation");
		_NtDuplicateObject NtDuplicateObject = (_NtDuplicateObject)GetProcAddress(Ntdll, "NtDuplicateObject");
		_NtOpenProcess NtOpenProcess = (_NtOpenProcess)GetProcAddress(Ntdll, "NtOpenProcess");
		NTSTATUS NtRet = NULL;
		DWORD size = sizeof(SYSTEM_HANDLE_INFORMATION);
		hInfo = (SYSTEM_HANDLE_INFORMATION*) new byte[size];
		ZeroMemory(hInfo, size);
		boolean OldPriv;
		RtlAdjustPrivilege(SeDebugPriv, TRUE, FALSE, &OldPriv);
		OBJECT_ATTRIBUTES Obj_Attribute = InitObjectAttributes(NULL, NULL, NULL, NULL);
		CLIENT_ID clientID = { 0 };

		do {
			delete[] hInfo; size *= 1.5;
			try {
				hInfo = (PSYSTEM_HANDLE_INFORMATION) new byte[size];
			} catch (std::bad_alloc) {
				procHandle ? CloseHandle(procHandle) : 0;
			}
			Sleep(1);
		} while ((NtRet = NtQuerySystemInformation(SystemHandleInformation, hInfo, size, NULL)) == STATUS_INFO_LENGTH_MISMATCH);

		if (!NT_SUCCESS(NtRet)) { procHandle ? CloseHandle(procHandle) : 0;	} 

		for (unsigned int i = 0; i < hInfo->HandleCount; ++i) {
			static DWORD NumOfOpenHandles;
			GetProcessHandleCount(GetCurrentProcess(), &NumOfOpenHandles); 
			if (NumOfOpenHandles > 150) { procHandle ? CloseHandle(procHandle) : 0; }

			if (!IsHandleValid((HANDLE)hInfo->Handles[i].Handle)) { continue; }
			if (hInfo->Handles[i].ObjectTypeNumber != ProcessHandleType) { continue; }

			clientID.UniqueProcess = (DWORD*)hInfo->Handles[i].ProcessId; 
			procHandle ? CloseHandle(procHandle) : 0;

			NtRet = NtOpenProcess(&procHandle, PROCESS_DUP_HANDLE, &Obj_Attribute, &clientID);
			if (!IsHandleValid(procHandle) || !NT_SUCCESS(NtRet)) { continue; }

			NtRet = NtDuplicateObject(procHandle, (HANDLE)hInfo->Handles[i].Handle, NtCurrentProcess, &HijackedHandle, PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, 0, 0);
			if (!IsHandleValid(HijackedHandle) || !NT_SUCCESS(NtRet)) { continue; }

			if (GetProcessId( HijackedHandle) != dwTargetProcessId) {
				procHandle ? CloseHandle(procHandle) : 0;
				continue;
			}
			break;
		}
		procHandle ? CloseHandle(procHandle) : 0;
		return HijackedHandle;
	}
}
