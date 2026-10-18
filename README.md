# RBTray++

**Version 5.0.1**

RBTray++ is a small Windows utility that lets you minimize any window to the
notification area (system tray) instead of the taskbar.

It was inspired by, and is based on, the original **RBTray** by Nikolay Redko
([http://rbtray.sf.net](http://rbtray.sf.net)), including the 3.x changes by
J.D. Purcell ([http://www.moitah.net/](http://www.moitah.net/)) and the later
4.3 release. RBTray++ is a modernized C++ rewrite for current versions of Windows.

## Features

- **Right-click a window's minimize (or close) button** to send it to the tray.
- **Hotkey:** `Win+Shift+H` hides the active window. If another program
  already uses it, RBTray++ falls back to `Ctrl+Alt+Shift+H`. The hotkey works
  with every application, including ones whose caption buttons the mouse hook
  can't detect (for example Microsoft Office).
- **System menu items** (`Alt+Space`) added to every window:
  - *Minimize in tray*
  - *Always on top* (toggle, with a check mark)
  - *My size* (resizes the window to fill the work area with a small margin)
- **Tray icon for each hidden window:**
  - Left-click (or `Enter`/`Space`) restores the window.
  - Right-click opens a menu: *Restore window*, *Close window*, *Kill window*.
  - The tooltip shows the window's current title.
- **Works with modern apps:** windows with custom title bars (Windows 11
  Explorer, Notepad, WinUI apps) are handled by acting on the top-level window.
- **Self-cleaning:** if a hidden window is shown again by its own application,
  or is closed, its tray icon goes away automatically.
- **Survives Explorer restarts:** tray icons are re-added when the taskbar is
  recreated.
- **Safe exit:** all hidden windows are restored when RBTray++ exits.
- MDI document windows, the taskbar, the Start menu and the desktop are never
  hidden.

## Requirements

- Windows 10 or Windows 11 (32-bit and 64-bit builds are available).
- To build: Visual Studio 2022 with the *Desktop development with C++* workload.

## Usage

1. Keep `RBTray++.exe` and `RBHook.dll` in the same folder.
2. Run `RBTray++.exe`. An RBTray++ icon appears in the tray; only one instance
   runs at a time.
3. Right-click any window's minimize button, or press `Win+Shift+H`, to hide it.
4. Click its tray icon to bring it back.

To exit, use *Exit RBTray* from the RBTray++ tray icon menu, or run:

```bash
RBTray++.exe --exit
```

> Use the 64-bit build on 64-bit Windows. The mouse hook only reaches
> applications of the same bitness as RBTray++; the hotkey and system menu
> commands are handled by RBTray++ itself.

## Building

Open `RBTray.sln` in Visual Studio 2022 and build, or from a Developer
Command Prompt:

```bash
msbuild RBTray.sln -p:Configuration=Release -p:Platform=x64
```

Output is written to `bin\<Platform>\<Configuration>\`.

### Continuous integration

GitHub Actions ([`.github/workflows/build.yml`](.github/workflows/build.yml))
builds the x64 and Win32 Release configurations on every push and pull request,
and keeps each build as a downloadable artifact. The build fails on any compiler
warning, or if a packaged binary needs a DLL that isn't part of Windows or the
package (for example the Visual C++ runtime, which is linked statically).

Pushing a tag such as `v5.0.1` also publishes a GitHub release with a zip for
each platform. The tag must match the version in `RBTray/rbtray.rc`, otherwise
the build fails.

### Project layout

| Path               | Contents                                                        |
| ------------------ | --------------------------------------------------------------- |
| `RBTray/`          | `RBTray++.exe`: hidden window, tray icons, hotkey, About dialog |
| `RBHook/`          | `RBHook.dll`: global mouse/keyboard/message hooks               |
| `Common/`          | Definitions shared by the EXE and the DLL                       |

## Changelog

### 5.0.1 (2026)

- Version bump for publishing; README.md added.

### 5.0.0 (2026)

- Rewritten in C++17 (Visual Studio 2022) and built as Unicode, with x64 and
  Win32 builds.
- No limit on the number of hidden windows (was 64).
- Tray icons are removed as soon as a hidden window is closed or reshown by its
  application.
- Tray icons restore on a normal click and can be used from the keyboard.
- *Close window* no longer blocks on "Save changes?" prompts.
- MDI document windows are never sent to the tray.
- Hidden owner windows are no longer left invisible.
- *About RBTray* on RBTray's own tray icon; the About dialog shows the version
  and build.
- Version information added to the executable.

The full comparison with the previous C version (based on RBTray 3.3 / 4.3)
follows. For the history of the original RBTray (2.9 to 3.3), see
[readme.txt](readme.txt).

#### Summary

| Area                  | Previous C version                             | RBTray++ 5.0.0                                                |
| --------------------- | ---------------------------------------------- | ------------------------------------------------------------- |
| Language              | C, ANSI (`char`) strings                       | C++17, Unicode (`wchar_t`) strings                            |
| Hidden windows        | Fixed table of 64; more are silently ignored   | No limit                                                      |
| Restore from tray     | Mouse press and release within 1 second        | Normal click, or `Enter`/`Space` from the keyboard            |
| Stale tray icons      | Removed only when the mouse moves over them    | Removed as soon as the window is closed or reshown by its app |
| *Close window*        | Waits until the app answers (can hang RBTray)  | Returns immediately                                           |
| Right-click on button | Also passed on to the application              | Consumed by RBTray++                                          |
| MDI document windows  | Could hide the whole application               | Never sent to the tray                                        |
| RBTray's own icon     | Menu with *Exit* only                          | Menu with *About RBTray* and *Exit*                           |
| Version info          | None                                           | Version in the exe's properties and the About dialog          |

#### Tray icons

- **No limit on hidden windows.** The old version kept hidden windows in a
  fixed table of 64 entries (`MAXCOUNT`); hiding a 65th window did nothing.
  They are now kept in a list with no fixed size (`HiddenWindowList`).
- **Icons are removed as soon as they're stale.** RBTray++ now watches
  hidden windows with an out-of-process WinEvent hook (`EVENT_OBJECT_SHOW`,
  `EVENT_OBJECT_DESTROY`). When an application closes a hidden window, or shows
  it again itself (for example when you start the app a second time), its tray
  icon goes away right away. Before, this only happened when the mouse moved
  over the icon, and an icon was never removed for a window that was shown
  again.
- **Modern tray icon behaviour (`NOTIFYICON_VERSION` 3).**
  - A hidden window is restored by a normal click (`NIN_SELECT`). The old
    version timed the press and release with `GetTickCount` and ignored a click
    held for more than a second.
  - Icons can be used from the keyboard: `Enter`/`Space` restores the window,
    and the menu key (or `Shift+F10`) opens its menu.
  - The version is set again after every re-add, so icons keep working after
    Explorer restarts.
- **Owner windows are no longer left hidden.** When a window is hidden, a
  visible owner window is hidden with it. If the hidden window was then closed,
  killed or reshown by its application, the owner stayed invisible. It is now
  shown again in all of those cases (`HiddenWindow::ShowOwner`).

#### Window commands

- ***Close window* no longer blocks.** It used to `SendMessage(WM_CLOSE)`,
  which waits for the application. An app showing a "Save changes?" prompt
  froze RBTray until the prompt was answered, and some Explorer windows ignored
  the message. It now posts `WM_CLOSE`; the tray icon is removed when the window
  is actually destroyed. The internal `WM_DELTRAY` message is gone.
- **MDI document windows are never sent to the tray.** Older Office, Visual
  Studio and similar apps put a minimize/close button on each document window.
  Right-clicking one used to hide the whole application. RBTray++ now ignores
  those windows (`IsMdiChild`), and doesn't add its items to their system menus.
- **The right-click on a minimize/close button is consumed.** The mouse hook
  now swallows both the press and the release, so the application no longer
  also reacts to the right-click (for example by opening its own menu).
- **The hook reports the window that was actually clicked**, rather than its
  top-level window, so RBTray++ can tell when the click was on an MDI document
  window. All commands still act on the top-level window.
- Restoring a window no longer needs special handling for child windows, since
  only top-level windows are ever hidden.

#### RBTray's own icon and About dialog

- RBTray's own tray icon menu now has **About RBTray** above *Exit RBTray*
  (the item was commented out before). It is greyed out while the dialog is
  open, so only one About dialog can be open at a time.
- The About dialog:
  - shows the RBTray++ version, *Based on RBTray 3.3 and 4.3*, and the
    copyright line (Nikolay Redko, J.D. Purcell, Ramesh Mokkapati);
  - shows whether it's the **64-bit or 32-bit build** instead of an email
    address;
  - comes to the front when opened.
- The own icon opens its menu on a click, `Enter`/`Space`, or right-click.

#### Unicode

- Everything is built as Unicode. Window titles in non-Latin scripts now show
  correctly in tray tooltips; the ANSI version could garble them.

#### Startup and shutdown

- If `RBHook.dll` loads but the hooks can't be installed, the DLL is now
  unloaded before RBTray++ exits.
- Hooks and the WinEvent hook are always removed, even if startup fails part
  way (`RBTrayApp` destructor).

#### Unchanged

These features were already in the previous C version and work the same way:

- Right-click a window's minimize or close button to hide it, including in
  apps that capture the mouse (Chrome, Edge).
- Hotkey `Win+Shift+H`, or `Ctrl+Alt+Shift+H` if it's taken; `Win+T` through
  the keyboard hook where Windows allows it.
- System menu items *My size*, *Minimize in tray* and *Always on top*.
- Custom title bars (Windows 11 Explorer, Notepad, WinUI apps): the top-level
  window is the one hidden.
- Tray icon from the window, its class or its executable; tooltip with the
  current title, or the exe name if there's none. Hung apps can't freeze
  RBTray++ (500 ms timeouts).
- *Restore window*, *Close window*, *Kill window* menu on each tray icon.
- Taskbar, Start menu and desktop are never hidden.
- Icons are re-added when Explorer restarts.
- Hidden windows are restored on exit; `--exit` closes a running instance;
  only one instance runs at a time.
- Opera windows that hide themselves are sent to the tray.

#### Code structure

The single-file C program was split into classes:

| Old (C)                                       | New (C++)                                    |
| --------------------------------------------- | -------------------------------------------- |
| `rbtray.c`: `WinMain`, `HookWndProc`, globals | `main.cpp`, `RBTrayApp` (`RBTrayApp.cpp`)    |
| `list[]`, `owners[]`, `exeIcons[]` arrays     | `HiddenWindow`, `HiddenWindowList`           |
| `UpdateTray`, `AddOwnIcon`, `DelFromTray`     | `TrayIcon` (wrapper for `Shell_NotifyIcon`)  |
| `GetWindowIcon`, `GetExeIcon`, `GetTrayTip`   | `WindowInfo`                                 |
| `UpdMenu` and a copy in `rbhook.c`            | `rbtray::SystemMenu` (`Common/SystemMenu.h`) |
| `rbhook.c`: free functions and globals        | `Hooks` class in `RBHook/RBHook.cpp`         |
| `#define` IDs and messages                    | `rbtray::Command`, `rbtray::Message` enums   |

Other changes:

- Messages renamed: `WM_MYCMD` → `WM_RBTRAY_COMMAND`, `WM_TRAYCMD` →
  `WM_TRAY_NOTIFY`. `WM_WINDOW_EVENT` added; `WM_DELTRAY` removed.
- `rbtray::PostCommand` replaces repeated `PostMessage(FindWindow(...))` calls
  in the hook DLL.
- Exported functions use `extern "C"` with a `RBHOOK_API` macro.
- Unused code removed (the `IRIS.tedit` define, the unused window extra bytes).

#### Build and packaging

- Visual Studio 2022 solution `RBTray.sln`, C++17, Win32 and x64
  configurations.
- Output goes to `bin\<Platform>\<Configuration>\`; intermediate files to
  `obj\`. The executable is `RBTray++.exe`, next to `RBHook.dll`.
- `VERSIONINFO` resource added, with product name RBTray++.

## Credits

- **Nikolay Redko**: original RBTray ([http://rbtray.sf.net](http://rbtray.sf.net))
- **J.D. Purcell**: RBTray 3.0 changes ([http://www.moitah.net/](http://www.moitah.net/))
- **Ramesh Mokkapati**: RBTray++

## License

Copyright (C) 1998-2026 Nikolay Redko, J.D. Purcell, Ramesh Mokkapati

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.
