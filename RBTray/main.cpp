// RBTray.exe entry point
#include <windows.h>
#include <string.h>
#include "RBTrayApp.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR cmdLine, int)
{
    // Handle --exit before the single-instance check, which would otherwise
    // make this second instance quit without telling the running one
    if (strstr(cmdLine, "--exit"))
    {
        RBTrayApp::ExitRunningInstance();
        return 0;
    }

    // Only one RBTray++ at a time. The mutex is held until the process exits.
    CreateMutex(NULL, TRUE, L"RBTray++.exe");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        return 0;
    }

    RBTrayApp app(hInstance);
    return app.Run();
}
