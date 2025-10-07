## Title

UI/Networking: Parallel scan, CIDR + Auto-range, alive filter, English UI

## Summary

This PR introduces a parallel scan engine, CIDR support with Auto-range, Quick Tools (Ping/DNS/Ports), and a cleaned-up English UI. The Scan Results list is filtered to alive devices by default. Status bar and debug logs are clearer, and the main window is resizable with responsive columns.

## Changes

- Add parallel scan engine and supporting data structures.
- Support CIDR in `IP Range/CIDR`; add `Auto-range` to populate primary subnet.
- Implement Quick Tools: `Ping`, `DNS`, `Ports` for the IP in the Quick Tools bar.
- Filter Scan Results to show only alive devices; status bar counts reflect found hosts.
- Add `LAN only` toggle to constrain scanning to the primary subnet.
- Remove legacy navigation sidebar; expand main content to full width.
- Translate user-facing strings and comments to English in `main_raygui.c`.
- UI tweaks: responsive columns, resizable window, improved row rendering.

## Validation

1. Build on Windows using:
   ```
   powershell -ExecutionPolicy Bypass -File build.ps1 -Compiler Clang
   ```
2. Run `bin/catnet_scanner.exe`.
3. Perform a subnet scan; confirm only alive hosts appear in the list.
4. Toggle `LAN only` and verify scope restriction.
5. Use Quick Tools (`Ping`, `DNS`, `Ports`) against a single IP.

## Screenshots

Please add `docs/screenshots/ui-v0.2.png` and link it in the README. Example placement:

```
![UI Screenshot](docs/screenshots/ui-v0.2.png)
```

## Notes

- Some compiler warnings (`C4244`) and linker warnings (`LNK4042`) remain; functional but slated for cleanup.
- No breaking API changes; behavior change is the filtered results list.

## Checklist

- [x] Code builds successfully on Windows (Clang).
- [x] README updated with features and screenshot reference.
- [x] Release notes added under `docs/RELEASE_NOTES_v0.2.0.md`.
- [x] Manual testing of scans and Quick Tools performed.