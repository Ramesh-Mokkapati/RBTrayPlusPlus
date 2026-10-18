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

#include "TrayIcon.h"
#include <shellapi.h>

TrayIcon::TrayIcon(HWND owner, UINT id, UINT callbackMessage)
    : owner(owner)
    , id(id)
    , callbackMessage(callbackMessage)
{
}

bool TrayIcon::Add(HICON icon, const std::wstring& tip)
{
    if (!Send(NIM_ADD, icon, tip))
    {
        return false;
    }

    // Must follow every NIM_ADD, including the re-add after Explorer restarts
    NOTIFYICONDATA nid = { 0 };
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = owner;
    nid.uID = id;
    nid.uVersion = NOTIFYICON_VERSION;
    Shell_NotifyIcon(NIM_SETVERSION, &nid);
    return true;
}

bool TrayIcon::Update(HICON icon, const std::wstring& tip)
{
    return Send(NIM_MODIFY, icon, tip);
}

void TrayIcon::Remove()
{
    NOTIFYICONDATA nid = { 0 };
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = owner;
    nid.uID = id;
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

bool TrayIcon::Send(DWORD action, HICON icon, const std::wstring& tip)
{
    NOTIFYICONDATA nid = { 0 };
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = owner;
    nid.uID = id;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = callbackMessage;
    nid.hIcon = icon;
    lstrcpyn(nid.szTip, tip.c_str(), _countof(nid.szTip));
    return Shell_NotifyIcon(action, &nid) != FALSE;
}
