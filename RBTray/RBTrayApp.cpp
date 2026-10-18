// ****************************************************************************
// 
// RBTray
// Copyright (C) 1998-2026  Nikolay Redko, J.D. Purcell, Ramesh Mokkapati
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// 
// ****************************************************************************
#include "RBTrayApp.h"
#include "rbtray.h"
#include "resource.h"
#include "SystemMenu.h"
#include "WindowInfo.h"

using namespace rbtray;

RBTrayApp* RBTrayApp::current = nullptr;

RBTrayApp::RBTrayApp(HINSTANCE instance)
    : instance(instance)
{
    current = this;
}

RBTrayApp::~RBTrayApp()
{
    // Normally done in OnDestroy; this covers Run failing part way
    if (winEventHook)
    {
        UnhookWinEvent(winEventHook);
    }

    if (hookLib)
    {
        UnRegisterHook();
        FreeLibrary(hookLib);
    }

    current = nullptr;
}

void RBTrayApp::ExitRunningInstance()
{
    HWND running = FindWindow(kWindowName, kWindowName);
    if (running)
    {
        SendMessage(running, WM_CLOSE, 0, 0);
    }
}

int RBTrayApp::Run()
{
    if (!LoadHook())
    {
        return 0;
    }

    if (!CreateMainWindow())
    {
        return 2;
    }

    taskbarCreatedMessage = RegisterWindowMessage(L"TaskbarCreated");
    hiddenWindows = std::make_unique<HiddenWindowList>(hwnd, WM_TRAY_NOTIFY);
    ownIcon = std::make_unique<TrayIcon>(hwnd, kOwnIconId, WM_TRAY_NOTIFY);

    RegisterTrayHotkey();
    WatchWindowEvents();
    AddOwnIcon();
    SystemMenu::AddToAllWindows();

    // No hwnd filter: WM_QUIT and menu/dialog messages must be retrieved too
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

bool RBTrayApp::LoadHook()
{
    hookLib = LoadLibrary(L"RBHook.dll");
    if (!hookLib)
    {
        MessageBox(NULL, L"Error loading RBHook.dll", L"RBTray++", MB_OK | MB_ICONHAND);
        return false;
    }

    if (!RegisterHook(hookLib))
    {
        MessageBox(NULL, L"Error setting hook procedure", L"RBTray++", MB_OK | MB_ICONHAND);
        FreeLibrary(hookLib);
        hookLib = NULL;
        return false;
    }

    return true;
}

bool RBTrayApp::CreateMainWindow()
{
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = kWindowName;
    if (!RegisterClass(&wc))
    {
        MessageBox(NULL, L"Error creating window class", L"RBTray++", MB_OK | MB_ICONHAND);
        return false;
    }

    // A normal (never shown) top-level window rather than a message-only one,
    // so it receives the "TaskbarCreated" broadcast when Explorer restarts
    if (!CreateWindow(kWindowName, kWindowName, WS_OVERLAPPED, 0, 0, 0, 0, NULL, NULL, instance, this))
    {
        MessageBox(NULL, L"Error creating window", L"RBTray++", MB_OK);
        return false;
    }

    return true;
}

void RBTrayApp::RegisterTrayHotkey()
{
    // Win+Shift+H, or Ctrl+Alt+Shift+H if another program already owns it.
    // Handled by RBTray++ itself, so it works for every app, including Office,
    // whose caption buttons the mouse hook can't detect.
    if (RegisterHotKey(hwnd, kHotkeyId, MOD_WIN | MOD_SHIFT | MOD_NOREPEAT, 'H'))
    {
        hotkeyName = L"Win+Shift+H";
    }
    else if (RegisterHotKey(hwnd, kHotkeyId, MOD_CONTROL | MOD_ALT | MOD_SHIFT | MOD_NOREPEAT, 'H'))
    {
        hotkeyName = L"Ctrl+Alt+Shift+H";
    }
}

void RBTrayApp::WatchWindowEvents()
{
    // Tells RBTray when a hidden window is shown again by its own application
    // (for example when the app is started again) or is destroyed, so its tray
    // icon goes away. Out of context, so it runs in RBTray++ and works for 32-bit
    // and 64-bit applications alike, with nothing injected.
    winEventHook = SetWinEventHook(EVENT_OBJECT_DESTROY, EVENT_OBJECT_SHOW, NULL, WinEventProc,
        0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
}

void RBTrayApp::AddOwnIcon()
{
    HICON icon = LoadIcon(instance, L"RBTRAYICO");
    if (!icon)
    {
        icon = LoadIcon(NULL, IDI_APPLICATION);
    }

    std::wstring tip = hotkeyName.empty()
        ? L"RBTray++ - right-click a window's minimize button to hide it"
        : L"RBTray++ - right-click a minimize button or press " + hotkeyName + L" to hide a window";
    ownIcon->Add(icon, tip);
}

LRESULT CALLBACK RBTrayApp::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // The RBTrayApp pointer arrives with WM_NCCREATE and is kept in the window
    if (msg == WM_NCCREATE)
    {
        RBTrayApp* app = (RBTrayApp*)((CREATESTRUCT*)lParam)->lpCreateParams;
        app->hwnd = hwnd;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)app);
    }

    RBTrayApp* app = (RBTrayApp*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (!app || !app->hiddenWindows)
    {
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return app->HandleMessage(msg, wParam, lParam);
}

void CALLBACK RBTrayApp::WinEventProc(HWINEVENTHOOK, DWORD event, HWND hwnd,
    LONG idObject, LONG idChild, DWORD, DWORD)
{
    // Only whole windows, and only the ones RBTray has hidden. Handled later
    // as a posted message, so it never runs in the middle of hiding or
    // restoring a window.
    if (idObject != OBJID_WINDOW || idChild != CHILDID_SELF || !hwnd || !current ||
        !current->hiddenWindows || !current->hiddenWindows->Contains(hwnd))
    {
        return;
    }

    PostMessage(current->hwnd, WM_WINDOW_EVENT, event, (LPARAM)hwnd);
}

LRESULT RBTrayApp::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Not a constant, so it can't be a case label
    if (msg == taskbarCreatedMessage)
    {
        OnTaskbarCreated();
        return 0;
    }

    switch (msg)
    {
    case WM_COMMAND:
        OnMenuCommand(LOWORD(wParam));
        return 0;
    case WM_RBTRAY_COMMAND:
        OnRBTrayCommand((UINT)wParam, (HWND)lParam);
        return 0;
    case WM_TRAY_NOTIFY:
        OnTrayNotify((UINT)wParam, (UINT)lParam);
        return 0;
    case WM_WINDOW_EVENT:
        hiddenWindows->OnWindowEvent((DWORD)wParam, (HWND)lParam);
        return 0;
    case WM_HOTKEY:
        if (wParam == kHotkeyId && GetForegroundWindow())
        {
            OnRBTrayCommand(IDM_TRAY, GetForegroundWindow());
        }
        return 0;
    case WM_DESTROY:
        OnDestroy();
        return 0;
    }

    // Includes WM_CLOSE (from "RBTray++.exe --exit"), which destroys the window
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void RBTrayApp::OnMenuCommand(UINT command)
{
    switch (command)
    {
    case IDM_EXIT:
        // OnDestroy restores windows and removes the hooks
        DestroyWindow(hwnd);
        return;
    case IDM_ABOUT:
        ShowAbout();
        return;
    }

    HiddenWindow* target = hiddenWindows->FindByIconId(menuIconId);
    if (!target)
    {
        return;
    }

    HWND window = target->Window();
    switch (command)
    {
    case IDM_RESTORE:
        hiddenWindows->Restore(window);
        break;
    case IDM_CLOSE:
        CloseHiddenWindow(window);
        break;
    case IDM_DESTROY:
        if (MessageBox(NULL, L"Kill the window?", L"RBTray++", MB_YESNO | MB_ICONWARNING) == IDYES)
        {
            KillHiddenWindow(window);
        }
        break;
    }
}

void RBTrayApp::OnRBTrayCommand(UINT command, HWND clicked)
{
    // Apps with custom title bars (Windows 11 Explorer, Notepad, WinUI apps)
    // report their caption buttons in a child window. Always act on the
    // top-level window, otherwise the child is trayed and the real window
    // is hidden with no way to restore it.
    HWND target = GetAncestor(clicked, GA_ROOT);
    if (!target)
    {
        target = clicked;
    }

    switch (command)
    {
    case IDM_SIZE:
        ResizeToWorkArea(target);
        break;
    case IDM_TRAY:
        if (CanHide(clicked, target))
        {
            hiddenWindows->Hide(target);
        }
        break;
    case IDM_ONTOP:
        SystemMenu::ToggleOnTop(target);
        break;
    }
}

void RBTrayApp::OnTrayNotify(UINT iconId, UINT notification)
{
    // Version 3 tray notifications: NIN_SELECT for a click, NIN_KEYSELECT for
    // Enter or Space, WM_CONTEXTMENU for a right-click or the menu key
    if (iconId == kOwnIconId)
    {
        // RBTray's own icon: every way of activating it opens its menu
        if (notification == NIN_SELECT || notification == NIN_KEYSELECT || notification == WM_CONTEXTMENU)
        {
            ShowTrayMenu(true);
        }
        return;
    }

    switch (notification)
    {
    case NIN_SELECT:
    case NIN_KEYSELECT:
    {
        // Enter can send NIN_KEYSELECT twice; the second finds no window
        HiddenWindow* target = hiddenWindows->FindByIconId(iconId);
        if (target)
        {
            hiddenWindows->Restore(target->Window());
        }
        break;
    }
    case WM_CONTEXTMENU:
        menuIconId = iconId;
        ShowTrayMenu(false);
        break;
    case WM_MOUSEMOVE:
        // Keeps the tooltip showing the window's current title, and drops the
        // icon if the window came back or went away
        hiddenWindows->Refresh(iconId);
        break;
    }
}

void RBTrayApp::OnDestroy()
{
    // Show every hidden window so nothing is left invisible after exit
    hiddenWindows->RestoreAll();

    if (winEventHook)
    {
        UnhookWinEvent(winEventHook);
        winEventHook = NULL;
    }

    UnregisterHotKey(hwnd, kHotkeyId);
    ownIcon->Remove();
    UnRegisterHook();

    SystemMenu::RemoveFromAllWindows();
    FreeLibrary(hookLib);
    hookLib = NULL;
    PostQuitMessage(0);
}

void RBTrayApp::OnTaskbarCreated()
{
    AddOwnIcon();
    hiddenWindows->AddAllIcons();
}

bool RBTrayApp::CanHide(HWND clicked, HWND target) const
{
    if (!IsWindow(target) || target == hwnd || hiddenWindows->Contains(target))
    {
        return false;
    }

    // Never an MDI document window: hiding its top-level window would hide
    // the whole application
    for (HWND w = clicked; w && w != target; w = GetParent(w))
    {
        if (IsMdiChild(w))
        {
            return false;
        }
    }

    // The taskbars, the Start menu, and the desktop itself (the hotkey can
    // fire while it is active)
    const wchar_t* const neverHide[] =
    {
        L"Shell_TrayWnd", L"Shell_SecondaryTrayWnd", L"DV2ControlHost", L"Progman", L"WorkerW",
    };

    std::wstring className = WindowInfo::ClassName(target);
    for (const wchar_t* name : neverHide)
    {
        if (_wcsicmp(name, className.c_str()) == 0)
        {
            return false;
        }
    }

    return true;
}

void RBTrayApp::ShowTrayMenu(bool ownIcon)
{
    HMENU menu = CreatePopupMenu();
    if (!menu)
    {
        MessageBox(NULL, L"Error creating menu", L"RBTray++", MB_OK);
        return;
    }

    if (ownIcon)
    {
        AppendMenu(menu, MF_STRING | (aboutOpen ? MF_GRAYED : 0), IDM_ABOUT, L"About RBTray++");
        AppendMenu(menu, MF_SEPARATOR, 0, NULL);
    }
    else
    {
        AppendMenu(menu, MF_SEPARATOR, 0, NULL);
        AppendMenu(menu, MF_STRING, IDM_RESTORE, L"Restore window");
        AppendMenu(menu, MF_STRING, IDM_CLOSE, L"Close window");
        AppendMenu(menu, MF_STRING, IDM_DESTROY, L"Kill window");
        AppendMenu(menu, MF_SEPARATOR, 0, NULL);
    }

    AppendMenu(menu, MF_STRING, IDM_EXIT, L"Exit RBTray++");

    POINT point;
    GetCursorPos(&point);

    // Needed so the menu closes when the user clicks elsewhere, and the
    // posted message after it makes the next click on an icon work
    SetForegroundWindow(hwnd);
    TrackPopupMenu(menu, TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_RIGHTALIGN | TPM_BOTTOMALIGN,
        point.x, point.y, 0, hwnd, NULL);
    PostMessage(hwnd, WM_NULL, 0, 0);
    DestroyMenu(menu);
}

void RBTrayApp::ShowAbout()
{
    if (aboutOpen)
    {
        return;
    }

    aboutOpen = true;
    DialogBoxParam(instance, MAKEINTRESOURCE(IDD_DIALOG1), hwnd, AboutDialogProc, 0);
    aboutOpen = false;
}

INT_PTR CALLBACK RBTrayApp::AboutDialogProc(HWND dialog, UINT msg, WPARAM wParam, LPARAM)
{
    switch (msg)
    {
    case WM_INITDIALOG:
#ifdef _WIN64
        SetDlgItemText(dialog, IDC_EDIT, L"64-bit build");
#else
        SetDlgItemText(dialog, IDC_EDIT, L"32-bit build");
#endif
        // RBTray's own window is never shown, so bring the dialog forward
        SetForegroundWindow(dialog);
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(dialog, LOWORD(wParam));
            return TRUE;
        }
        break;
    case WM_CLOSE:
        EndDialog(dialog, IDCANCEL);
        return TRUE;
    }

    return FALSE;
}

void RBTrayApp::CloseHiddenWindow(HWND target)
{
    // Posted, not sent: an application that asks "Save changes?" would
    // otherwise block RBTray until answered, and Explorer windows ignore a
    // sent WM_CLOSE. The tray icon goes when the window is actually
    // destroyed (see WatchWindowEvents).
    PostMessage(target, WM_CLOSE, 0, 0);
}

void RBTrayApp::KillHiddenWindow(HWND target)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(target, &pid);
    HANDLE process = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!process)
    {
        return;
    }

    if (TerminateProcess(process, 1))
    {
        hiddenWindows->Forget(target);
    }

    CloseHandle(process);
}

void RBTrayApp::ResizeToWorkArea(HWND target)
{
    // Fills the work area, leaving a small margin
    RECT rc;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);

    ShowWindow(target, SW_SHOW);
    int dx = (rc.right - rc.left) / 35;
    int dy = (rc.bottom - rc.top) / 28;
    MoveWindow(target, dx, dy, rc.right - rc.left - 2 * dx, rc.bottom - rc.top - 2 * dy, TRUE);
}
