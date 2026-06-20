# Pedal Buttons

Foot switcher management of music player through Arduino and CLI for Windows.

The following are required to run the system:

* An Arduino board (at least as capable as a Nano) flashed with `pedal-buttons/pedal-buttons.ino`
* Button signals are received on pins 2 and 3
* Build the console utility located in the `win/cli` folder and run it on the host machine, after connecting the Arduino via USB

## CLI

The utility requires a configuration file, such as `pedal-buttons.ini`, with approximately the following content:

```ini
[app]
app = winamp
; instead of winamp, you can specify the name of any existing section,
; then the configuration will be loaded from it
; there can be any number of sections

[winamp]
LEFT_CLICK = x
LEFT_HOLD = z
RIGHT_CLICK = v
RIGHT_HOLD = b
; the left side can only contain these strings
; the right side can contain from 0 to any number of key names
; separated by commas
; for example: LEFT_CLICK = ctrl,r
; below is a list of special key strings
```

Command line options:

* `-i` or `--ini` - (**required**) Path to the .ini configuration file
* `-l` or `--list` - (optional) Show available serial ports
* `-c` or `--portCount` - (optional, default 9) Number of ports to scan (from 1 to 20)
* `-p` or `--port` - (optional, default 9) Port number to connect to (from 1 to portCount)

If the file path is not specified, it will be searched for either in the launch folder (working directory) or next to the executable.

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
