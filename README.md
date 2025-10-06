# CatNet Scanner (GUI, WIP)

CatNet Scanner is a Windows network scanner focused on a graphical UI built with GacUI. The legacy console/TUI mode was removed to simplify the project and concentrate efforts on the GUI.

## Features

- Scan local subnet and custom IP ranges.
- Identify devices: ping, reverse DNS, MAC via ARP.
- Check common TCP open ports (configurable list).
- Export results to a text file.

## Build (Windows, MSVC)

### Dependency: GacUI

- Run `scripts\setup_gacui.ps1` to clone and build GacUI Release and set:
  - `GACUI_INCLUDE` (headers under `Import`)
  - `GACUI_LIBS` (libraries under `Lib\Release`)
- Alternatively, pass `-GacUIInclude` and `-GacUILibs` to `build.ps1`.

### Compile

```
powershell -ExecutionPolicy Bypass -File build.ps1 -Compiler MSVC
```

Produces `bin\catnet_gui_scanner.exe`.

### Run

```
bin\catnet_gui_scanner.exe
```

## Notes

- The GUI is created programmatically in `src\main_gacui.cpp` (optional `src\resources.xml`).
- MAC via ARP works only on the same subnet; ping uses ICMP; ports are checked via non-blocking TCP.
- WIP: UI and flow may change.