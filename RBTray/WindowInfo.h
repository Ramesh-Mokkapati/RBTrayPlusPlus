// Information about other applications' windows
#pragma once

#include <windows.h>
#include <string>

class WindowInfo
{
public:
    // Icon the window itself provides, or NULL. Many modern apps (Windows 11
    // Explorer, Notepad, WinUI apps) don't answer WM_GETICON or set a class icon.
    // The icon belongs to the window; don't destroy it.
    static HICON GetIcon(HWND hwnd);

    // Small icon embedded in the window's executable, or NULL. Caller must DestroyIcon it.
    static HICON ExtractExeIcon(HWND hwnd);

    // Full path of the executable that owns the window, or "" if it can't be read
    static std::wstring GetExePath(HWND hwnd);

    // The window's current title, or the exe name if it has none. Never blocks
    // for long on a hung application.
    static std::wstring GetTitle(HWND hwnd);

    // Window class name
    static std::wstring ClassName(HWND hwnd);
};
