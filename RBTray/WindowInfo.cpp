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

#include "WindowInfo.h"
#include <shellapi.h>

namespace
{
    // How long to wait for another application to answer a message, so a hung
    // application can't freeze RBTray
    const UINT kTimeoutMs = 500;

    bool SendWithTimeout(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, DWORD_PTR& result)
    {
        result = 0;
        return SendMessageTimeout(hwnd, msg, wParam, lParam, SMTO_ABORTIFHUNG, kTimeoutMs, &result) != 0;
    }
}

HICON WindowInfo::GetIcon(HWND hwnd)
{
    DWORD_PTR icon = 0;
    const WPARAM sizes[] = { ICON_SMALL, ICON_SMALL2, ICON_BIG };
    for (WPARAM size : sizes)
    {
        if (SendWithTimeout(hwnd, WM_GETICON, size, 0, icon) && icon)
        {
            return (HICON)icon;
        }
    }

    if (icon = GetClassLongPtr(hwnd, GCLP_HICONSM))
    {
        return (HICON)icon;
    }

    if (icon = GetClassLongPtr(hwnd, GCLP_HICON))
    {
        return (HICON)icon;
    }

    return NULL;
}

HICON WindowInfo::ExtractExeIcon(HWND hwnd)
{
    std::wstring path = GetExePath(hwnd);
    HICON smallIcon = NULL;
    if (!path.empty())
    {
        ExtractIconEx(path.c_str(), 0, NULL, &smallIcon, 1);
    }

    return smallIcon;
}

std::wstring WindowInfo::GetExePath(HWND hwnd)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process)
    {
        return std::wstring();
    }

    wchar_t path[MAX_PATH];
    DWORD size = MAX_PATH;
    BOOL ok = QueryFullProcessImageName(process, 0, path, &size);
    CloseHandle(process);
    return ok ? std::wstring(path, size) : std::wstring();
}

std::wstring WindowInfo::GetTitle(HWND hwnd)
{
    // WM_GETTEXT with a timeout instead of GetWindowText, which can block on a hung app
    wchar_t title[256] = L"";
    DWORD_PTR length = 0;
    if (SendWithTimeout(hwnd, WM_GETTEXT, _countof(title), (LPARAM)title, length) && title[0])
    {
        title[_countof(title) - 1] = 0;
        return title;
    }

    std::wstring path = GetExePath(hwnd);
    size_t slash = path.find_last_of(L'\\');
    return slash == std::wstring::npos ? path : path.substr(slash + 1);
}

std::wstring WindowInfo::ClassName(HWND hwnd)
{
    wchar_t name[256] = L"";
    GetClassName(hwnd, name, _countof(name));
    return name;
}
