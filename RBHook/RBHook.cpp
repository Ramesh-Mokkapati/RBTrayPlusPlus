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

#include <windows.h>
#include <wchar.h>
#include "rbtray.h"
#include "SystemMenu.h"

using namespace rbtray;

namespace
{
    // Hook procedures and their state. All members are static because Windows
    // calls the procedures directly, in every process the DLL is loaded into;
    // each process gets its own copy of this state.
    class Hooks
    {
    public:
        static bool Install(HMODULE hLib)
        {
            mouseHook = SetWindowsHookEx(WH_MOUSE, MouseProc, hLib, 0);
            messageHook = SetWindowsHookEx(WH_GETMESSAGE, GetMsgProc, hLib, 0);
            keyboardHook = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, hLib, 0);

            if (!mouseHook || !messageHook || !keyboardHook)
            {
                Uninstall();
                return false;
            }

            return true;
        }

        static void Uninstall()
        {
            Unhook(mouseHook);
            Unhook(messageHook);
            Unhook(keyboardHook);
        }

    private:
        static HHOOK mouseHook;
        static HHOOK messageHook;
        static HHOOK keyboardHook;

        // Top-level window whose minimize/close button got a right-button
        // press, until the release; NULL when no right-click is in progress
        static HWND pendingRightClick;
        // The window that actually received that press (may be a child, such
        // as an MDI document window); this is what RBTray is told about
        static HWND pendingClickedWindow;

        static void Unhook(HHOOK& hook)
        {
            if (hook)
            {
                UnhookWindowsHookEx(hook);
                hook = NULL;
            }
        }

        static bool IsTrayButton(UINT hitTest)
        {
            return hitTest == HTREDUCE || hitTest == HTCLOSE;
        }

        // Right-click on a window's minimize or close button sends it to the tray
        static LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
        {
            if (nCode < 0)
            {
                return CallNextHookEx(mouseHook, nCode, wParam, lParam);
            }

            MOUSEHOOKSTRUCT* mhs = (MOUSEHOOKSTRUCT*)lParam;
            HWND root = GetAncestor(mhs->hwnd, GA_ROOT);
            if (!root)
            {
                root = mhs->hwnd;
            }

            UINT hit = mhs->wHitTestCode;
            switch (wParam)
            {
            case WM_NCRBUTTONDOWN:
                if (IsTrayButton(hit))
                {
                    pendingRightClick = root;
                    pendingClickedWindow = mhs->hwnd;
                    return 1;  // swallowed: the app never sees this right-click
                }

                pendingRightClick = NULL;
                break;
            case WM_RBUTTONUP:
                // Some apps capture the mouse after a non-client button press,
                // so the release arrives as a client WM_RBUTTONUP. Ask the
                // window what is under the cursor now instead.
                if (!pendingRightClick || pendingRightClick != root)
                {
                    break;
                }

                hit = (UINT)SendMessage(root, WM_NCHITTEST, 0, MAKELPARAM(mhs->pt.x, mhs->pt.y));

                // fall through
            case WM_NCRBUTTONUP:
                if (pendingRightClick && pendingRightClick == root && IsTrayButton(hit))
                {
                    PostCommand(IDM_TRAY, pendingClickedWindow);
                    pendingRightClick = NULL;
                    return 1;  // swallowed, like the press
                }

                pendingRightClick = NULL;
                break;
            case WM_LBUTTONDOWN:
            case WM_NCLBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_NCMBUTTONDOWN:
            case WM_RBUTTONDOWN:
                pendingRightClick = NULL;  // a different click cancels a pending right-click
                break;
            }

            return CallNextHookEx(mouseHook, nCode, wParam, lParam);
        }

        // Win+T sends the active window to the tray. Windows 10/11 reserve
        // Win+T for the taskbar, so RBTray's own hotkey is the reliable way.
        static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
        {
            if (nCode < 0)
            {
                return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
            }

            bool winDown = (GetKeyState(VK_LWIN) & 0x80) || (GetKeyState(VK_RWIN) & 0x80);
            if (winDown && (GetKeyState('T') & 0x80) &&
                (GetWindowLong(GetForegroundWindow(), GWL_STYLE) & WS_CAPTION))
            {
                BYTE keyState[256];
                GetKeyboardState(keyState);
                keyState[VK_LWIN] = 0;
                keyState[VK_RWIN] = 0;
                keyState['T'] = 0;
                SetKeyboardState(keyState);
                PostCommand(IDM_TRAY, GetForegroundWindow());
                return 1;
            }

            return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
        }

        static LRESULT CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam)
        {
            if (nCode < 0)
            {
                return CallNextHookEx(messageHook, nCode, wParam, lParam);
            }

            MSG* msg = (MSG*)lParam;
            switch (msg->message)
            {
            case WM_SYSCOMMAND:
                // One of RBTray's system menu items was chosen
                switch (LOWORD(msg->wParam))
                {
                case IDM_TRAY:
                case IDM_ONTOP:
                case IDM_SIZE:
                    PostCommand(LOWORD(msg->wParam), msg->hwnd);
                    break;
                }
                break;
            case WM_PAINT:
                // Give windows created after RBTray started its system menu items
                SystemMenu::AddItems(msg->hwnd);
                break;
            case WM_SHOWWINDOW:
                // Opera hides its window instead of minimizing; send it to the tray
                if (msg->wParam == FALSE)
                {
                    wchar_t title[256];
                    GetWindowText(msg->hwnd, title, _countof(title));
                    if (wcsstr(title, L"Opera") != NULL)
                    {
                        PostCommand(IDM_TRAY, msg->hwnd);
                    }
                }
                break;
            }

            return CallNextHookEx(messageHook, nCode, wParam, lParam);
        }
    };

    HHOOK Hooks::mouseHook = NULL;
    HHOOK Hooks::messageHook = NULL;
    HHOOK Hooks::keyboardHook = NULL;
    HWND Hooks::pendingRightClick = NULL;
    HWND Hooks::pendingClickedWindow = NULL;
}

RBHOOK_API BOOL RegisterHook(HMODULE hLib)
{
    return Hooks::Install(hLib) ? TRUE : FALSE;
}

RBHOOK_API void UnRegisterHook(void)
{
    Hooks::Uninstall();
}
