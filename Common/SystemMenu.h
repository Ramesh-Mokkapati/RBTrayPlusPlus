// RBTray's items in a window's system menu (Alt+Space). Used by RBTray.exe,
// which adds them to every window at startup, and by RBHook.dll, which adds
// them to windows created later.
#pragma once

#include "rbtray.h"

namespace rbtray
{
    class SystemMenu
    {
    public:
        // True if the window's system menu already has RBTray's items
        static bool HasItems(HWND hwnd)
        {
            HMENU menu = GetSystemMenu(hwnd, FALSE);
            if (!menu)
            {
                return false;
            }

            for (int i = 0; i < GetMenuItemCount(menu); i++)
            {
                if (GetMenuItemID(menu, i) == IDM_TRAY)
                {
                    return true;
                }
            }

            return false;
        }

        // Adds "My size", "Minimize in tray" and "Always on top" at the top
        static void AddItems(HWND hwnd)
        {
            // MDI document windows have a system menu too, but can't go to the tray
            HMENU menu = GetSystemMenu(hwnd, FALSE);
            if (!menu || IsMdiChild(hwnd) || HasItems(hwnd))
            {
                return;
            }

            // Each insert goes before the current first item, so they end up in reverse order
            InsertMenu(menu, GetMenuItemID(menu, 0), MF_SEPARATOR, IDM_SEPARATOR, NULL);
            UINT onTopFlags = (GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST) ? MF_CHECKED : 0;
            InsertMenu(menu, GetMenuItemID(menu, 0), MF_STRING | onTopFlags, IDM_ONTOP, L"Always on top");
            InsertMenu(menu, GetMenuItemID(menu, 0), MF_STRING, IDM_TRAY, L"Minimize in tray");
            InsertMenu(menu, GetMenuItemID(menu, 0), MF_STRING, IDM_SIZE, L"My size");
        }

        static void RemoveItems(HWND hwnd)
        {
            HMENU menu = GetSystemMenu(hwnd, FALSE);
            if (!menu || !HasItems(hwnd))
            {
                return;
            }

            DeleteMenu(menu, IDM_TRAY, MF_BYCOMMAND);
            DeleteMenu(menu, IDM_ONTOP, MF_BYCOMMAND);
            DeleteMenu(menu, IDM_SEPARATOR, MF_BYCOMMAND);
            DeleteMenu(menu, IDM_SIZE, MF_BYCOMMAND);
        }

        // Flips the window's "always on top" state and the menu check mark
        static void ToggleOnTop(HWND hwnd)
        {
            HMENU menu = GetSystemMenu(hwnd, FALSE);
            if (!menu)
            {
                return;
            }

            bool onTop = (GetMenuState(menu, IDM_ONTOP, MF_BYCOMMAND) & MF_CHECKED) != 0;
            CheckMenuItem(menu, IDM_ONTOP, MF_BYCOMMAND | (onTop ? MF_UNCHECKED : MF_CHECKED));
            SetWindowPos(hwnd, onTop ? HWND_NOTOPMOST : HWND_TOPMOST, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOREDRAW);
        }

        // Adds or removes the items on every top-level window
        static void AddToAllWindows()
        {
            EnumWindows(AddItemsProc, 0);
        }

        static void RemoveFromAllWindows()
        {
            EnumWindows(RemoveItemsProc, 0);
        }

    private:
        static BOOL CALLBACK AddItemsProc(HWND hwnd, LPARAM)
        {
            AddItems(hwnd);
            return TRUE;
        }

        static BOOL CALLBACK RemoveItemsProc(HWND hwnd, LPARAM)
        {
            RemoveItems(hwnd);
            return TRUE;
        }
    };
}
