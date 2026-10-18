#include "HiddenWindow.h"
#include <algorithm>
#include "SystemMenu.h"
#include "WindowInfo.h"

HiddenWindow::HiddenWindow(HWND window, HWND notifyWindow, UINT callbackMessage)
    : window(window)
    , trayIcon(notifyWindow, IconIdFor(window), callbackMessage)
{
}

HiddenWindow::~HiddenWindow()
{
    trayIcon.Remove();
    if (exeIcon)
    {
        DestroyIcon(exeIcon);
    }
}

void HiddenWindow::Hide()
{
    // Minimizing before hiding makes the window come back the way it was
    ShowWindow(window, SW_HIDE);
    ShowWindow(window, SW_MINIMIZE);
    AddIcon();
    ShowWindow(window, SW_HIDE);

    HWND owner = GetWindow(window, GW_OWNER);
    if (owner && IsWindowVisible(owner))
    {
        ShowWindow(owner, SW_HIDE);
        hiddenOwner = owner;
    }

    rbtray::SystemMenu::AddItems(window);
}

void HiddenWindow::Show()
{
    ShowOwner();
    ShowWindow(window, SW_SHOW);
    ShowWindow(window, SW_RESTORE);
    SetForegroundWindow(window);
}

void HiddenWindow::ShowOwner()
{
    if (hiddenOwner && IsWindow(hiddenOwner))
    {
        ShowWindow(hiddenOwner, SW_SHOW);
    }

    hiddenOwner = NULL;
}

void HiddenWindow::UpdateIcon()
{
    trayIcon.Update(CurrentIcon(), WindowInfo::GetTitle(window));
}

void HiddenWindow::AddIcon()
{
    trayIcon.Add(CurrentIcon(), WindowInfo::GetTitle(window));
}

HICON HiddenWindow::CurrentIcon()
{
    HICON icon = WindowInfo::GetIcon(window);
    if (icon)
    {
        return icon;
    }

    // Extracted once and kept, because the tooltip refresh asks again on every mouse move
    if (!exeIcon)
    {
        exeIcon = WindowInfo::ExtractExeIcon(window);
    }

    return exeIcon ? exeIcon : LoadIcon(NULL, IDI_APPLICATION);
}

HiddenWindowList::HiddenWindowList(HWND notifyWindow, UINT callbackMessage)
    : notifyWindow(notifyWindow)
    , callbackMessage(callbackMessage)
{
}

std::vector<std::unique_ptr<HiddenWindow>>::iterator HiddenWindowList::Find(HWND hwnd)
{
    return std::find_if(windows.begin(), windows.end(),
        [hwnd](const std::unique_ptr<HiddenWindow>& w) { return w->Window() == hwnd; });
}

bool HiddenWindowList::Contains(HWND hwnd) const
{
    return std::any_of(windows.begin(), windows.end(),
        [hwnd](const std::unique_ptr<HiddenWindow>& w) { return w->Window() == hwnd; });
}

HiddenWindow* HiddenWindowList::FindByIconId(UINT iconId) const
{
    for (const auto& w : windows)
    {
        if (w->IconId() == iconId)
        {
            return w.get();
        }
    }

    return nullptr;
}

void HiddenWindowList::Hide(HWND hwnd)
{
    if (Contains(hwnd))
    {
        return;
    }

    windows.push_back(std::make_unique<HiddenWindow>(hwnd, notifyWindow, callbackMessage));
    windows.back()->Hide();
}

void HiddenWindowList::Restore(HWND hwnd)
{
    auto it = Find(hwnd);
    if (it != windows.end())
    {
        (*it)->Show();
        windows.erase(it);  // removes the tray icon
    }
}

void HiddenWindowList::Forget(HWND hwnd)
{
    auto it = Find(hwnd);
    if (it != windows.end())
    {
        (*it)->ShowOwner();
        windows.erase(it);
    }
}

void HiddenWindowList::Refresh(UINT iconId)
{
    HiddenWindow* w = FindByIconId(iconId);
    if (!w)
    {
        return;
    }

    // Gone, or shown again by its application: the icon is no longer needed
    if (!IsWindow(w->Window()) || IsWindowVisible(w->Window()))
    {
        Forget(w->Window());
    }
    else
    {
        w->UpdateIcon();
    }
}

void HiddenWindowList::OnWindowEvent(DWORD event, HWND hwnd)
{
    if (!Contains(hwnd))
    {
        return;
    }

    // A show is checked against the window's state now rather than trusted:
    // Hide briefly shows the window minimized, and that event arrives after
    // the window is hidden again. A destroy is final, but can arrive while
    // the window is still being torn down (IsWindow still true), so it isn't
    // checked.
    if ((event == EVENT_OBJECT_SHOW && IsWindowVisible(hwnd)) || event == EVENT_OBJECT_DESTROY)
    {
        Forget(hwnd);
    }
}

void HiddenWindowList::AddAllIcons()
{
    for (const auto& w : windows)
    {
        w->AddIcon();
    }
}

void HiddenWindowList::RestoreAll()
{
    for (const auto& w : windows)
    {
        w->Show();
    }

    windows.clear();
}
