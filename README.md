# CatNet Scanner (C)

Windows console (TUI) network scanner written in C, similar to diagnostic tools. Features:

- Scan local subnet and arbitrary IP ranges
- Identify devices (ping, reverse DNS, MAC via ARP)
- Ping IPs
- Resolve reverse DNS
- Check open ports (default list)
- Display results and logs with keyboard navigation
- Export results to a text file

## Build

Prerequisites: Windows with a C compiler (MSVC `cl` from Visual Studio Build Tools or MinGW `gcc`).

### MSVC (cl)

Run in PowerShell:

```
powershell -ExecutionPolicy Bypass -File build.ps1 -Compiler MSVC
```

### MinGW (gcc)

```
powershell -ExecutionPolicy Bypass -File build.ps1 -Compiler MinGW
```

The executable will be generated at `bin\catnet_tui_scanner.exe`.

### GacUI (GUI)

To use the graphical interface with GacUI:

1. Obtain and build GacUI (including `GacUI.lib` and `Vlpp.lib`).
2. Set environment variables with the paths:
   - `GACUI_INCLUDE`: folder with GacUI headers.
   - `GACUI_LIBS`: folder with libs (`GacUI.lib`, `Vlpp.lib`).
3. Build with MSVC using `-UseGacUI`:

```
powershell -ExecutionPolicy Bypass -File build.ps1 -Compiler MSVC -UseGacUI -GacUIInclude "C:\\Dev\\GacUI\\Import" -GacUILibs "C:\\Dev\\GacUI\\Lib\\Release"
```

The executable will be generated at `bin\catnet_gui_scanner.exe`.

Note: If you hit errors compiling or linking GacUI on your machine, you can use `GacGen.exe` to generate code from GacUI resources (XAML). You will still need to link `GacUI.lib`/`Vlpp.lib`. If desired, I can integrate a generation flow via `GacGen` into the build.

## Usage

Run the program in the Windows console:

```
bin\catnet_tui_scanner.exe
```

Shortcuts:
- F1: Scan local subnet
- F2: Scan range (enter start and end)
- F3: Ping IP
- F4: Reverse DNS for IP
- F5: Export results to text
- Arrows: Navigate the list
- Q / Esc: Quit

Results include IP, hostname (if available), MAC (if available), and open ports (22, 80, 443, 139, 445, 3389 by default).

## Notes

- MAC via ARP (`SendARP`) works only for hosts on the same subnet.
- Port scanning uses non-blocking TCP connections with a timeout.
- Ping uses ICMP (`IcmpSendEcho`).

## Chat Log Utilities

This project keeps local chat history and daily snapshots in `docs\chat_logs` (ignored by Git). The `build.ps1` script provides tasks to help:

- Save a quick chat note (Markdown):
  - `powershell -ExecutionPolicy Bypass -File build.ps1 -Task chat-note -Text "My note"`
  - Optional: specific log name via `-LogName "project-x"`

- Save a daily snapshot (Markdown):
  - `powershell -ExecutionPolicy Bypass -File build.ps1 -Task daily-snapshot`
  - Optional: `-LogName "project-x"` to group by name

- Clean the logs folder, keeping only Markdown, README and common attachments:
  - `powershell -ExecutionPolicy Bypass -File build.ps1 -Task chat-clean`
  - Preserved: `*.md`, `README.md` and attachments `png, jpg, jpeg, gif, svg, pdf, txt`

For more details, see `docs\chat_logs\README.md`.