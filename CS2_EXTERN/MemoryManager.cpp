#include <chrono>
#include <thread>
#include "MemoryManager.hpp"
#include "resource.h"

MemoryManager::MemoryManager() {}
MemoryManager::~MemoryManager() { CloseHandle(m_pipeHandle); }

DWORD MemoryManager::FindPID(const char* process_name) {
	DWORD pID;
	HANDLE hwdProcessID_ = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	PROCESSENTRY32 processEntryVar;
	processEntryVar.dwSize = sizeof(processEntryVar);
	do {
		if (!strcmp(processEntryVar.szExeFile, process_name)) {
			pID = processEntryVar.th32ProcessID;
			CloseHandle(hwdProcessID_);
			return pID;
		}
	} while (Process32Next(hwdProcessID_, &processEntryVar));
	CloseHandle(hwdProcessID_);
	return NULL;
}

uintptr_t MemoryManager::GetModuleBaseAddress(const char* modName, DWORD procId) {
	HANDLE hsnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
	if (hsnap != INVALID_HANDLE_VALUE) {
		MODULEENTRY32 modEntry;
		modEntry.dwSize = sizeof(modEntry);
		if (Module32First(hsnap, &modEntry)) {
			do {
				if (!strcmp(modEntry.szModule, modName)) {
					CloseHandle(hsnap);
					return (uintptr_t)modEntry.modBaseAddr;
				}
			} while (Module32Next(hsnap, &modEntry));
		}
	}
	return (uintptr_t)nullptr;
}

void MemoryManager::Init() {
	try {
		this->pID = FindPID("cs2.exe");
		if (!this->pID)
			throw 1;

		STARTUPINFOEXA si;  //Create notepad process with explorer.exe as parent
		ZeroMemory(&si, sizeof(STARTUPINFOEXA));
		PROCESS_INFORMATION pi;
		SIZE_T attributeSize;
		HANDLE parentProcessHandle = OpenProcess(MAXIMUM_ALLOWED, false, FindPID("explorer.exe"));
		InitializeProcThreadAttributeList(NULL, 1, 0, &attributeSize);
		si.lpAttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(), 0, attributeSize);
		InitializeProcThreadAttributeList(si.lpAttributeList, 1, 0, &attributeSize);
		UpdateProcThreadAttribute(si.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_PARENT_PROCESS, &parentProcessHandle, sizeof(HANDLE), NULL, NULL);
		si.StartupInfo.cb = sizeof(STARTUPINFOEXA);
		CreateProcessA(NULL, (LPSTR)"notepad", NULL, NULL, FALSE, EXTENDED_STARTUPINFO_PRESENT, NULL, NULL, &si.StartupInfo, &pi);
		CloseHandle(parentProcessHandle);

		this->notepadPID = m->FindPID("notepad.exe"); //shellcode host
		if (!this->notepadPID)
			throw 1;

		this->moduleBase = GetModuleBaseAddress("client.dll", pID);
	}
	catch (...) {
		MessageBoxA(NULL, "Failed To Find CS2.exe or Notepad.exe", "Client.exe", MB_ICONSTOP | MB_OK);
		exit(0);
	}
}

void MemoryManager::InjectServer() {
	HRSRC shellcodeResource = FindResource(NULL, MAKEINTRESOURCE(IDR_SERVER_BIN1), "SERVER_BIN");
	DWORD shellcodeSize = SizeofResource(NULL, shellcodeResource);
	HGLOBAL payload = LoadResource(NULL, shellcodeResource);

	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, notepadPID);
	PVOID remoteBuffer = VirtualAllocEx(hProc, NULL, shellcodeSize, (MEM_RESERVE | MEM_COMMIT), PAGE_EXECUTE_READWRITE);
	WriteProcessMemory(hProc, remoteBuffer, payload, shellcodeSize, NULL);
	HANDLE remoteThread = CreateRemoteThread(hProc, NULL, 0, (LPTHREAD_START_ROUTINE)remoteBuffer, NULL, 0, NULL);
	CloseHandle(hProc);

	while (m_pipeHandle == INVALID_HANDLE_VALUE) {
		m_pipeHandle = this->ConnectPipe();
		Sleep(1);
	}
}

HANDLE MemoryManager::ConnectPipe() {
	HANDLE ret = INVALID_HANDLE_VALUE;
	while (1) { 
		ret = CreateFile(m_lpszPipename, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (ret != INVALID_HANDLE_VALUE) { break; } // Break if the pipe handle is valid. 
		if (GetLastError() != ERROR_PIPE_BUSY) { return INVALID_HANDLE_VALUE; }  // Exit if an error other than ERROR_PIPE_BUSY occurs. 
		if (!WaitNamedPipe(m_lpszPipename, 20000)) { return INVALID_HANDLE_VALUE; }
	}

	BOOL fSuccess = FALSE;
	fSuccess = SetNamedPipeHandleState(ret, PIPE_READMODE_BYTE, NULL, NULL);
	if (!fSuccess) {
		return NULL;
	}

	return ret;
}

bool MemoryManager::RequestReadProcessMemory(ReqRPM rpmRequest) {
	BOOL fSuccess = FALSE;
	DWORD bytesWritten = 0;
	fSuccess = WriteFile(m_pipeHandle, &rpmRequest, sizeof(rpmRequest), &bytesWritten, NULL);
	if (!fSuccess) {
		return false;
	}
	return true;
}

ResRPM MemoryManager::ReceiveReadProcessMemory() {
	ResRPM response;
	BOOL fSuccess = FALSE;
	DWORD bytesRead = 0;
	do { 
		fSuccess = ReadFile(m_pipeHandle, &response, sizeof(ResRPM), &bytesRead, NULL);
		if (!fSuccess && GetLastError() != ERROR_MORE_DATA) { break; }
	} while (!fSuccess); 
	return response;
}

ResRPM MemoryManager::RemoteReadProcessMemory(ReqRPM rpmRequest) {
	ResRPM response;
	if (RequestReadProcessMemory(rpmRequest)) {
		response = ReceiveReadProcessMemory();
	}
	return response;
}