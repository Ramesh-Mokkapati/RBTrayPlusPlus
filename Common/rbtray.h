// Definitions shared by RBTray.exe and RBHook.dll
#pragma once

#include <windows.h>

namespace rbtray
{
    // Class name and title of RBTray's hidden window. The hook DLL finds it by
    // this name to send it commands, and "RBTray.exe --exit" uses it to close it.
    constexpr wchar_t kWindowName[] = L"RBTrayHook";

    // Window messages RBTray's hidden window receives
    enum Message : UINT
    {
        // From the hook DLL (and the hotkey): wParam = Command, lParam = target HWND
        WM_RBTRAY_COMMAND = WM_USER + 1,
        // Tray icon notifications: wParam = icon ID, lParam = mouse or NIN_* message
        WM_TRAY_NOTIFY = WM_USER + 2,
        // A hidden window was shown or destroyed: wParam = WinEvent, lParam = HWND
        WM_WINDOW_EVENT = WM_USER + 3,
    };

    // Commands. They are also the IDs of the items RBTray adds to every
    // window's system menu, which is how the hook DLL recognises them in
    // WM_SYSCOMMAND.
    enum Command : UINT
    {
        IDM_CLOSE = 0x1001,
        IDM_EXIT = 0x1002,
        IDM_RESTORE = 0x1003,
        IDM_ABOUT = 0x1005,
        IDM_DESTROY = 0x1006,
        IDM_TRAY = 0x1007,
        IDM_ONTOP = 0x1009,
        IDM_SEPARATOR = 0x1010,
        IDM_SIZE = 0x1011,
    };

    // Sends a command to the running RBTray, if there is one
    inline void PostCommand(UINT command, HWND target)
    {
        HWND rbtray = FindWindow(kWindowName, kWindowName);
        if (rbtray)
        {
            PostMessage(rbtray, WM_RBTRAY_COMMAND, command, (LPARAM)target);
        }
    }

    // True for an MDI child window (a document window inside an app such as
    // an older Office or Visual Studio). Those are never sent to the tray:
    // hiding the top-level window would hide the whole application.
    inline bool IsMdiChild(HWND hwnd)
    {
        return (GetWindowLongPtr(hwnd, GWL_EXSTYLE) & WS_EX_MDICHILD) != 0;
    }
}

// Functions exported by RBHook.dll
#ifdef RBHOOK_EXPORTS
#define RBHOOK_API extern "C" __declspec(dllexport)
#else
#define RBHOOK_API extern "C" __declspec(dllimport)
#endif

// Installs the global hooks; hLib is RBHook.dll's module handle
RBHOOK_API BOOL RegisterHook(HMODULE hLib);
RBHOOK_API void UnRegisterHook(void);
