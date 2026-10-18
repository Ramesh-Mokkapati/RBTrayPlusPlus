// RBTray's hidden window: receives commands from the hook DLL and the hotkey,
// and handles clicks on the tray icons
#pragma once

#include <windows.h>
#include <memory>
#include <string>
#include "HiddenWindow.h"
#include "TrayIcon.h"

class RBTrayApp
{
public:
    explicit RBTrayApp(HINSTANCE instance);
    ~RBTrayApp();

    RBTrayApp(const RBTrayApp&) = delete;
    RBTrayApp& operator=(const RBTrayApp&) = delete;

    // Loads the hook DLL, creates the window and runs until RBTray exits.
    // Returns the process exit code.
    int Run();

    // Tells an already running RBTray to exit ("RBTray.exe --exit")
    static void ExitRunningInstance();

private:
    // RBTray's own tray icon. Hidden windows' icon IDs are window handles,
    // which are never 0, so they can't collide with it.
    static const UINT kOwnIconId = 0;
    static const int kHotkeyId = 1;

    // The running instance, for the WinEvent callback, which has no user data
    static RBTrayApp* current;

    HINSTANCE instance;
    HWND hwnd = NULL;
    HMODULE hookLib = NULL;
    HWINEVENTHOOK winEventHook = NULL;
    UINT taskbarCreatedMessage = 0;

    std::unique_ptr<HiddenWindowList> hiddenWindows;
    std::unique_ptr<TrayIcon> ownIcon;

    UINT menuIconId = 0;          // icon whose popup menu is open
    bool aboutOpen = false;       // the About dialog is showing
    std::wstring hotkeyName;      // "" if no hotkey could be registered

    bool LoadHook();
    bool CreateMainWindow();
    void RegisterTrayHotkey();
    void WatchWindowEvents();
    void AddOwnIcon();

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    // Called by Windows when any window is shown or destroyed
    static void CALLBACK WinEventProc(HWINEVENTHOOK hook, DWORD event, HWND hwnd,
        LONG idObject, LONG idChild, DWORD thread, DWORD time);

    // Message handlers
    void OnMenuCommand(UINT command);
    void OnRBTrayCommand(UINT command, HWND target);
    void OnTrayNotify(UINT iconId, UINT notification);
    void OnDestroy();
    void OnTaskbarCreated();

    // Commands
    bool CanHide(HWND clicked, HWND target) const;
    void ShowTrayMenu(bool ownIcon);
    void ShowAbout();
    void CloseHiddenWindow(HWND target);
    void KillHiddenWindow(HWND target);
    static void ResizeToWorkArea(HWND target);

    static INT_PTR CALLBACK AboutDialogProc(HWND dialog, UINT msg, WPARAM wParam, LPARAM lParam);
};
