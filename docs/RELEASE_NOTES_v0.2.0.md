# CatNet Scanner v0.2.0 – Parallel Scan, CIDR, Quick Tools, English UI

This release focuses on scan performance, usability, and a cleaner UI. All user-facing strings are now in English.

## Highlights

- Parallel scan engine with responsive UI updates during discovery.
- CIDR support in `IP Range/CIDR` plus `Auto-range` to populate your primary subnet.
- Quick Tools bar for one-off actions: `Ping`, `DNS`, and `Ports` against a single IP.
- Filtered results: the Scan Results list shows only alive devices by default.
- LAN-only toggle to constrain scanning scope to your primary subnet.
- Responsive columns and resizable window.
- Status bar and debug log improvements for clearer progress.

## Behavior Changes

- The device count in the status bar now reflects alive devices (found hosts).
- The Scan Results list is filtered to alive devices; total scanned addresses continue to be reported in the debug log.

## Fixes and Cleanups

- Removed the previous navigation sidebar and related state.
- Translated comments and messages to English in `src/main_raygui.c`.
- Minor code tidying to keep the main content full-width.

## Build

Windows build via PowerShell:

```
powershell -ExecutionPolicy Bypass -File build.ps1 -Compiler Clang
```

Run:

```
bin\catnet_scanner.exe
```

## Validation

- Run a scan on your LAN; alive hosts should appear in the list with a green status dot.
- Verify `LAN only` limits the scope to your primary subnet.
- Use Quick Tools (`Ping`, `DNS`, `Ports`) against a single IP to confirm functionality.

## Known Issues

- Some compiler warnings (`C4244`) about type conversions remain; functional but slated for cleanup.
- Linker warnings (`LNK4042`) about duplicate object specification observed; non-blocking, will be addressed.

## Screenshots

Add `docs/screenshots/ui-v0.2.png` and reference it in `README.md`.

## Credits

Thanks to the raylib and raygui communities. This project uses both as submodules.