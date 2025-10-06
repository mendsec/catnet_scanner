# Functions and Modules Manual (GUI, WIP)

This manual describes the network diagnostic modules and the current GUI structure using GacUI. The console/TUI mode has been removed.

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
- `void scan_subnet(DeviceList* out, const ScanConfig* cfg)`
  - Scan local subnet, filling `out`.
- `void scan_range(DeviceList* out, const ScanConfig* cfg, const char* start_ip, const char* end_ip)`
  - Iterate from `start_ip` to `end_ip` (inclusive), identifying devices.

## GUI (src/main_gacui.cpp)
- Main window with programmatic controls (buttons, list and dialogs).
- Supported actions:
  - Scan local subnet and IP ranges (input via dialogs).
  - Ping and reverse DNS for a given IP.
  - Export results to file.
- Text conversions/utilities: helper functions for UTF-8 and list refresh.

## Export (src/export.h, src/export.c)
- `int export_results_to_file(const char* path, const DeviceList* list)`
  - Output a simple CSV-like text file with header `IP;Hostname;MAC;Status;Ports`.

## Entry Point (src/main_gacui.cpp)
- Initialize Winsock (`net_init`) and configure the GUI window with GacUI.
- UI events trigger network operations and list updates.
- On exit, release resources (`device_list_clear`, `net_cleanup`).

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
- GacUI resource integration (XAML) with `GacGen`.