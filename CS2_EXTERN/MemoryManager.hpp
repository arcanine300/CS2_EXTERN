#pragma once
#include <Windows.h>
#include <TlHelp32.h>

typedef struct ReqRPM ReqRPM;
struct ReqRPM {
	uintptr_t address = 0;
	SIZE_T size = 100;
};

typedef struct ResRPM ResRPM;
struct ResRPM {
	BOOL status = FALSE;
	SIZE_T bytesRead = 0;
	char bytes[100];
};

class MemoryManager {
public:
	uintptr_t moduleBase;
	DWORD pID;
	DWORD notepadPID;
	HANDLE m_pipeHandle = INVALID_HANDLE_VALUE;
	LPCSTR m_lpszPipename = TEXT("\\\\.\\pipe\\imguipipe");

	template <class c>
	c ReadMem(DWORD64 dwAddress) {
		ResRPM rpmResponse;
		ReqRPM rpmRequest;
		rpmRequest.address = dwAddress;
		rpmRequest.size = sizeof(c);
		rpmResponse = RemoteReadProcessMemory(rpmRequest);
		return ((c*)rpmResponse.bytes)[0];
	}

	HANDLE ConnectPipe();
	bool RequestReadProcessMemory(ReqRPM rpmRequest);
	ResRPM ReceiveReadProcessMemory();
	ResRPM RemoteReadProcessMemory(ReqRPM rpmRequest);
	DWORD FindPID(const char* process_name);
	uintptr_t GetModuleBaseAddress(const char* modName, DWORD procId);
	void Init();
	void InjectServer();
	MemoryManager();
	~MemoryManager();
};

extern MemoryManager* m;

struct ViewMatrix {
	ViewMatrix() noexcept : data() {}
	float* operator[](int index) noexcept { return data[index]; }
	const float* operator[](int index) const noexcept { return data[index]; }
	float data[4][4];
};