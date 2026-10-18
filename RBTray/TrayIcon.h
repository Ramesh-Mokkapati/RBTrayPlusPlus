// Thin wrapper around Shell_NotifyIcon for the icons RBTray puts in the tray
#pragma once

#include <windows.h>
#include <string>

// Icons use NOTIFYICON_VERSION (version 3) behaviour: owner receives
// callbackMessage with the icon ID in wParam and, in lParam, NIN_SELECT for a
// click, NIN_KEYSELECT for Enter/Space, WM_CONTEXTMENU for a right-click or the
// menu key, plus WM_MOUSEMOVE and the other mouse messages.
class TrayIcon
{
public:
    TrayIcon(HWND owner, UINT id, UINT callbackMessage);

    // Icon and tooltip; the tooltip is cut to what the tray can show
    bool Add(HICON icon, const std::wstring& tip);
    bool Update(HICON icon, const std::wstring& tip);
    void Remove();

    UINT Id() const { return id; }

private:
    HWND owner;
    UINT id;
    UINT callbackMessage;

    bool Send(DWORD action, HICON icon, const std::wstring& tip);
};
