#include <windows.h> 
#define MAXPIPEFILESIZE 1024 

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

class HandleGatewayServer {
public:
    HANDLE hProc;
    HANDLE m_pipeHandle = INVALID_HANDLE_VALUE;
    int Init(DWORD target);
    int Gateway();
    BOOL RemoteReadProcessMemory(ReqRPM request);
protected:
    BOOL m_clientConnected = FALSE;
    LPCSTR m_lpszPipename = TEXT("\\\\.\\pipe\\imguipipe");
};