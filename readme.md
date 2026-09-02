# Pedal Buttons

Foot switcher management of music player through Arduino and CLI for Windows.

The following are required to run the system:

* An Arduino board (at least as capable as a Nano) flashed with `pedal-buttons/pedal-buttons.ino`
* Button signals are received on pins 2 and 3, wired to ground and read with the internal pull-ups
* Build the utility located in the `win` folder and run it on the host machine, after connecting the Arduino via USB

## Firmware

The sketch polls both pins without blocking and reports four events over the serial port at 9600 baud: `left-click`, `left-hold`, `right-click`, `right-hold`. A press shorter than 750 ms is a click; holding past that threshold reports a hold once, and no click follows on release. Contact bounce is filtered with a 25 ms window.

On start the board announces itself with a line such as `pedal-buttons 1.1.0`, and repeats that line whenever it receives a `?` character. This lets the host recognise the board among the serial ports of the system.

The firmware carries its own version, unrelated to the version of the host application: a board keeps whatever was flashed onto it while the application is updated independently. The host only looks at the `pedal-buttons` prefix to identify the board and never compares versions, so an application update never orphans an already flashed board.

## Building

Requirements: Visual Studio 2022 with the C++ desktop workload (toolset v143).

Third-party sources are expected in a `deps` folder next to the repository, inside the project folder:

```
<project>/
    repo/           this repository
    deps/imgui/     Dear ImGui, including backends/
    deps/glfw/      GLFW sources
```

Both are built from source by the `deps` project in the solution, so there is no separate CMake step and no DLLs to ship. Nothing else is needed beyond the Windows SDK.

Open `win/win.sln` and build the `Release|x64` configuration, or run `build.bat` from the project folder. The executable is written to the project folder as `pedal-buttons-<version>.exe`; the debug configuration produces `pedal-buttons-<version>-debug.exe` alongside it.

Only `x64` is supported. Common compiler settings and dependency paths live in `win/common.props`. If a machine keeps its dependencies elsewhere, override `DepsDir`, `ImGuiDir` or `GlfwDir` in `win/local.props` — it is imported automatically when present and is excluded from version control.

Source layout:

* `win/core` — serial port, port enumeration, config, key sending, logging. No UI dependencies.
* `win/ui` — everything that talks to the user.

## CLI

The utility requires a configuration file, such as `pedal-buttons.ini`, with approximately the following content:

```ini
[app]
app = winamp
; instead of winamp, you can name any other section below;
; that section becomes the active profile

[winamp]
LEFT_CLICK  = x
LEFT_HOLD   = ctrl+r
RIGHT_CLICK = v
RIGHT_HOLD  = ctrl+s,enter
match = process:winamp.exe
exe = C:\Program Files\Winamp\winamp.exe
```

The left side of a binding accepts only the four event names above. On the right side, `+` joins keys into a chord pressed at once, and a comma separates steps pressed one after another. So `ctrl+s,enter` presses Ctrl+S, then Enter.

`match` selects the window that the pedal drives. It is brought to the foreground before the keys are sent, which is what audio applications need — they only react to real keyboard focus. Three forms are accepted:

* `process:winamp.exe` — executable name, the default when no prefix is given
* `class:Winamp v1.x` — window class, useful when several applications share a name
* `title:Playlist` — a substring of the window title

`exe` is optional and only used when no matching window exists: the application is started instead. Leaving `match` empty keeps the old behaviour — keys go to whatever window currently has focus.

Both fields are easier to fill from the interface: **Привязки → Целевое приложение → Выбрать из запущенных** lists open windows and fills them in.

Command line options:

* `-i` or `--ini` - (optional) Path to the .ini configuration file
* `-l` or `--list` - (optional) List the serial ports present in the system, with device names
* `-c` or `--portCount` - (optional, default 9) Upper bound for `--port` (from 1 to 20)
* `-p` or `--port` - (optional, default 9) Port number to connect to (from 1 to portCount)
* `-h` or `--help` - (optional) Show help

Values may be given either as a separate argument (`-p 7`, `--port 7`) or attached (`-p7`, `--port=7`).

If the file path is not specified, the configuration is searched for in three places, in order: the data folder, next to the executable, then the working directory.

## Data folder

The log and, by default, the configuration live next to the executable, so the application stays portable. If that folder cannot be written to (for example after installing into Program Files), everything moves to `%LOCALAPPDATA%\pedal-buttons` instead.

The configuration is re-read automatically when the file changes, so bindings can be edited while the application is running.

Only one instance can run at a time — the serial port is opened exclusively, so a second instance would fail to connect. Listing ports (`--list`) and showing help are not affected.

## List of special key strings

* `ctrl` или `control` — Control
* `alt` или `menu` — Alt
* `shift` — Shift
* `space` — Space (Пробел)
* `enter` или `return` — Enter
* `tab` — Tab
* `esc` или `escape` — Esc
* `backspace` или `back` — Backspace
* `delete` или `del` — Delete
* `left` — Left (Стрелка влево)
* `right` — Right (Стрелка вправо)
* `up` — Up (Стрелка вверх)
* `down` — Down (Стрелка вниз)
* `home` — Home
* `end` — End
* `pageup` — Page Up
* `pagedown` — Page Down
* `f1`, `f2`, `f3`, `f4`, `f5`, `f6`, `f7`, `f8`, `f9`, `f10`, `f11`, `f12` — F1–F12
* `insert` — Insert
* `print` — Print Screen
* `capslock` — Caps Lock
* `numlock` — Num Lock
* `scrolllock` — Scroll Lock
