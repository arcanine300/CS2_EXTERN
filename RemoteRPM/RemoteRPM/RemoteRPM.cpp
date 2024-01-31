#include <Windows.h>
#include <stdio.h>
#include <TlHelp32.h>
#include "RemoteMemoryOps.hpp"
#include "HandleDupe.hpp"

DWORD FindPID(const char* process_name_cs_go) {
	DWORD pID;
	HANDLE hwdProcessID_ = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	PROCESSENTRY32 processEntryVar;
	processEntryVar.dwSize = sizeof(processEntryVar);
	do {
		if (!strcmp(processEntryVar.szExeFile, process_name_cs_go)) {
			pID = processEntryVar.th32ProcessID;
			CloseHandle(hwdProcessID_);
			return pID;
		}
	} while (Process32Next(hwdProcessID_, &processEntryVar));
	CloseHandle(hwdProcessID_);
	return NULL;
}

int main() {
	DWORD pID = FindPID("cs2.exe");
	HandleGatewayServer handleGatewayServer;
	handleGatewayServer.hProc = hj::HijackExistingHandle(pID);
	handleGatewayServer.Init(pID);
	return EXIT_SUCCESS;
}