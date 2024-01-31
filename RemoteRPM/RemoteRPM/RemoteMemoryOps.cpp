#include "RemoteMemoryOps.hpp"

int HandleGatewayServer::Init(DWORD target) {
    m_pipeHandle = CreateNamedPipe(m_lpszPipename, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, MAXPIPEFILESIZE, MAXPIPEFILESIZE, 0, NULL);
    if (m_pipeHandle == INVALID_HANDLE_VALUE) {
        return -1;
    }

    m_clientConnected = ConnectNamedPipe(m_pipeHandle, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
    if (m_clientConnected) {
        HandleGatewayServer::Gateway();
    } else {
        CloseHandle(m_pipeHandle); 
    }
    return 0;
}

int HandleGatewayServer::Gateway() {
    HANDLE hHeap = GetProcessHeap();
    void* request = HeapAlloc(hHeap, 0, MAXPIPEFILESIZE);
    void* reply = HeapAlloc(hHeap, 0, MAXPIPEFILESIZE);
    DWORD cbBytesRead = 0, cbReplyBytes = 0, cbWritten = 0;
    BOOL fSuccess = FALSE;

    while (1) { // Loop until done reading
        fSuccess = ReadFile(m_pipeHandle, request, MAXPIPEFILESIZE, &cbBytesRead, NULL);
        if (!fSuccess || cbBytesRead == 0) {
            if (GetLastError() == ERROR_BROKEN_PIPE) {
                CloseHandle(this->hProc);
            } 
            break;
        }

        ReqRPM rpmRequest;
        rpmRequest.address = ((uintptr_t*)request)[0];
        rpmRequest.size = ((SIZE_T*)request)[1];
        HandleGatewayServer::RemoteReadProcessMemory(rpmRequest);
    }

    FlushFileBuffers(m_pipeHandle);
    DisconnectNamedPipe(m_pipeHandle);
    CloseHandle(m_pipeHandle);
    HeapFree(hHeap, 0, request);
    HeapFree(hHeap, 0, reply); 
    return 1;
}

BOOL HandleGatewayServer::RemoteReadProcessMemory(ReqRPM request) {
    ResRPM response;
    response.status = ReadProcessMemory(this->hProc, (LPVOID)request.address, &response.bytes, request.size, &response.bytesRead);
    BOOL fSuccess = FALSE;
    DWORD bytesWritten = 0;
    fSuccess = WriteFile(m_pipeHandle, &response, sizeof(response), &bytesWritten, NULL);
    return TRUE;
}