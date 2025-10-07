# CatNet Scanner (GUI, WIP)

CatNet Scanner is a Windows network scanner with a graphical UI built using Raygui (raylib).

## Features

- Scan local subnet and custom IP ranges.
- Identify devices: ping, reverse DNS, MAC via ARP.
- Check common TCP open ports (configurable list).
- Export results to a text file.

## Build (Windows, MSVC)

### Compile

```
powershell -ExecutionPolicy Bypass -File build.ps1 -Compiler MSVC
```

Produces `bin\catnet_scanner.exe`.

### Run

```
bin\catnet_scanner.exe
```

## Notes

- The GUI is created programmatically in `src\main_raygui.c`.
- MAC via ARP works only on the same subnet; ping uses ICMP; ports are checked via non-blocking TCP.
- WIP: UI and flow may change.

## Third-Party and Acknowledgements

- Uses `raylib` and `raygui` as Git submodules:
  - raylib: https://github.com/raysan5/raylib
  - raygui: https://github.com/raysan5/raygui
- Huge thanks to the authors and contributors of raylib and raygui.