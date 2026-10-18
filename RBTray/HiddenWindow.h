// Windows RBTray has hidden, each with its own tray icon
#pragma once

#include <windows.h>
#include <memory>
#include <vector>
#include "TrayIcon.h"

// One hidden window and its tray icon. The tray icon ID is the window handle
// (truncated to 32 bits, which is all a window handle uses), never 0.
class HiddenWindow
{
public:
    HiddenWindow(HWND window, HWND notifyWindow, UINT callbackMessage);
    ~HiddenWindow();  // removes the tray icon

    HiddenWindow(const HiddenWindow&) = delete;
    HiddenWindow& operator=(const HiddenWindow&) = delete;

    // Hides the window (and its owner, if visible) and adds the tray icon
    void Hide();

    // Shows the window (and its owner, if Hide hid it) again
    void Show();

    // Shows the owner again if Hide hid it; for when the window itself came
    // back or went away without RBTray's help
    void ShowOwner();

    // Brings the tray icon's picture and tooltip up to date with the window
    void UpdateIcon();

    // Adds the tray icon again, after Explorer restarted and lost it
    void AddIcon();

    HWND Window() const { return window; }
    UINT IconId() const { return trayIcon.Id(); }

    static UINT IconIdFor(HWND hwnd) { return (UINT)(UINT_PTR)hwnd; }

private:
    HWND window;
    HWND hiddenOwner = NULL;  // owner window Hide hid too (it may hold the taskbar button)
    HICON exeIcon = NULL;     // icon extracted from the window's exe; destroyed with this object
    TrayIcon trayIcon;

    // The window's own icon, else the one from its exe, else a generic one
    HICON CurrentIcon();
};

// All windows RBTray has hidden
class HiddenWindowList
{
public:
    HiddenWindowList(HWND notifyWindow, UINT callbackMessage);

    bool Contains(HWND hwnd) const;
    HiddenWindow* FindByIconId(UINT iconId) const;

    // Hides the window and adds its tray icon
    void Hide(HWND hwnd);

    // Shows the window again and removes its tray icon
    void Restore(HWND hwnd);

    // Removes the tray icon without showing the window, because it was closed
    // or killed, or its application showed it again. An owner window Hide hid
    // is shown again.
    void Forget(HWND hwnd);

    // Refreshes the tray icon, or removes it if the window no longer exists
    // or is visible again
    void Refresh(UINT iconId);

    // A hidden window was shown or destroyed (EVENT_OBJECT_SHOW or
    // EVENT_OBJECT_DESTROY); removes its icon if that's really the case
    void OnWindowEvent(DWORD event, HWND hwnd);

    // Adds every tray icon again, after Explorer restarted
    void AddAllIcons();

    // Shows every hidden window again; used when RBTray exits
    void RestoreAll();

private:
    HWND notifyWindow;
    UINT callbackMessage;
    std::vector<std::unique_ptr<HiddenWindow>> windows;

    std::vector<std::unique_ptr<HiddenWindow>>::iterator Find(HWND hwnd);
};
