# Functions and Modules Manual (GUI, WIP)

This manual describes the network diagnostic modules and the current GUI structure using Raygui (raylib). The console/TUI mode has been removed.

## Types and Structures (src/app.h)
- `DeviceInfo`
  - Fields: `ip`, `hostname`, `mac`, `is_alive`, `open_ports[32]`, `open_ports_count`.
  - Represents a network host and its attributes.
- `DeviceList`
  - Fields: `items`, `count`, `capacity`.
  - Dynamic list of `DeviceInfo`.
- List functions (implemented in `src/list.c`):
  - `void device_list_init(DeviceList* list)`
  - `void device_list_clear(DeviceList* list)`
  - `void device_list_push(DeviceList* list, const DeviceInfo* info)`

## Utilities (src/utils.h, src/utils.c)
- `int ip_to_uint(const char* ip, unsigned long* out)`
  - Convert IPv4 text to host-order integer.
- `void uint_to_ip(unsigned long ip, char* buf, size_t buflen)`
  - Convert host-order integer to IPv4 string.
- `void trim_newline(char* s)`
  - Remove trailing `\n`/`\r`.
- `void safe_strcpy(char* dst, size_t dstsz, const char* src)`
  - Safe string copy with termination.

## Networking (src/net.h, src/net.c)
- `int net_init(void)` / `void net_cleanup(void)`
  - Initialize/cleanup Winsock2.
- `int net_ping_ipv4(const char* ip)`
  - ICMP Echo (`IcmpSendEcho`).
- `int net_reverse_dns(const char* ip, char* out, size_t outsz)`
  - Resolve hostname via `getnameinfo`.
- `int net_get_mac(const char* ip, char* out, size_t outsz)`
  - Get MAC via `SendARP`.

## Scanning (src/scan.h, src/scan.c)
### Configuration and logging
- `typedef struct ScanConfig { int default_ports[16]; int default_ports_count; int port_timeout_ms; }`
  - Default TCP ports to check, count, and per-port timeout (ms).
- `void scan_config_init(ScanConfig* cfg)`
  - Initialize sensible defaults.
- `void scan_set_logger(ScanLogFn fn)`
  - Optional logger callback to surface scan status/messages to the UI.

### Operations
- `void scan_subnet(DeviceList* out, const ScanConfig* cfg)`
  - Scan the primary local IPv4 subnet, filling `out`.
- `void scan_range(DeviceList* out, const ScanConfig* cfg, const char* start_ip, const char* end_ip)`
  - Iterate from `start_ip` to `end_ip` (inclusive), identifying devices.
- `void identify_device(DeviceInfo* info, const ScanConfig* cfg)`
  - Populate `is_alive`, `hostname`, `mac` and `open_ports` for a single IP.

## GUI (src/main_raygui.c)
- Main window built with Raygui; layout is programmatic (toolbar, sidebar, main panel, status bar).
- Implemented actions:
  - Scan local subnet or a custom IP range (input via the toolbar text box, formats "A.B.C.D-E" or "A.B.C.D-E.F.G.H").
  - Display results with columns: Status (ping), Hostname, IP, Ports, MAC.
- UI notes:
  - A "Stop" button is present but canceling an in-progress scan is not yet implemented.
  - Sidebar items (Favorites, Scan History, Scheduled Tasks) are placeholders for future features.

## Export (src/export.h, src/export.c)
- `int export_results_to_file(const char* path, const DeviceList* list)`
  - Output a simple CSV-like text file with header `IP;Hostname;MAC;Status;Ports`.
  - UI integration is WIP; export is available via the API.

## Entry Point (src/main_raygui.c)
- Initialize the Raylib window and Raygui styles.
- Set up `DeviceList`, `ScanConfig` and optional logger for UI status.
- On scan: call `net_init`, clear results, parse IP range, run `scan_range` or `scan_subnet`, then `net_cleanup`.
- On exit: `device_list_clear`, close the window.

## Considerations and Limitations
- MAC is only available for hosts in the same subnet (ARP).
- Ports checked are TCP and configurable via `ScanConfig`.
- Scans are sequential; consider threading for large networks.
- Ping timeout is fixed (1s); ports use configurable timeout.

## Future Extensions
- Concurrent scanning with a thread pool.
- Service detection (HTTP banners, SMB, basic RDP handshake).
- Additional exports (JSON/CSV with custom headers).
- IPv6 support (ping, DNS, port scanning).
- Cancel an in-progress scan; persist scan history and favorites.