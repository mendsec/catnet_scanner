// Main window built with Raygui and C
// Build via the project script: `build.ps1 -Compiler Clang -UI Raygui` (default).
// The script compiles raylib modules from `third_party/raylib/src` and integrates raygui,
// activating Windows SDK headers/libs when available. No prebuilt raylib binaries required.

#include "raylib.h"
#include <stdbool.h>

// Define raygui implementation header in a C file to avoid build issues
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "app.h"
#include "scan.h"
#include "utils.h"
#include "net.h"
#include "parallel_scan.h"
#include <string.h>

static Color g_bgColor = {24,24,24,255};
static bool quickToolsExpanded = true;
static bool lanOnly = true;
static char quickIpText[64] = "192.168.1.1";
static void apply_theme(bool dark)
{
    Color bg = dark ? (Color){24,24,24,255} : RAYWHITE;
    Color text = dark ? (Color){235,235,235,255} : (Color){20,20,20,255};
    Color line = dark ? (Color){80,80,80,255} : (Color){180,180,180,255};
    Color base = dark ? (Color){60,60,60,255} : (Color){220,220,220,255};
    g_bgColor = bg;
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, ColorToInt(bg));
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(text));
    GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED, ColorToInt(text));
    GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED, ColorToInt(text));
    GuiSetStyle(DEFAULT, LINE_COLOR, ColorToInt(line));
    GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, ColorToInt(base));
    GuiSetStyle(DEFAULT, BASE_COLOR_FOCUSED, ColorToInt(base));
    GuiSetStyle(DEFAULT, BASE_COLOR_PRESSED, ColorToInt(base));
}

static char g_statusText[128] = "Ready";
static char g_logLines[256][160];
static int g_logCount = 0;
static void gui_logger(const char* msg)
{
    if (!msg) { g_statusText[0] = '\0'; return; }
    strncpy(g_statusText, msg, sizeof(g_statusText) - 1);
    g_statusText[sizeof(g_statusText) - 1] = '\0';
    strncpy(g_logLines[g_logCount % 256], msg, sizeof(g_logLines[0]) - 1);
    g_logLines[g_logCount % 256][sizeof(g_logLines[0]) - 1] = '\0';
    g_logCount++;
}

static bool parse_range(const char* in, char* outStart, size_t szStart, char* outEnd, size_t szEnd)
{
    // Accepts formats:
    // 1) "A.B.C.D-E" (end last octet only)
    // 2) "A.B.C.D-E.F.G.H" (full range)
    // 3) "A.B.C.D/nn" (CIDR prefix length)
    const char* slash = NULL; for (const char* p = in; *p; ++p) { if (*p == '/') { slash = p; break; } }
    if (slash) {
        // CIDR: compute start/end from mask length
        size_t ipLen = (size_t)(slash - in);
        if (ipLen >= szStart) ipLen = szStart - 1;
        strncpy(outStart, in, ipLen); outStart[ipLen] = '\0';
        int prefix = atoi(slash + 1);
        if (prefix < 0 || prefix > 32) return false;
        unsigned long base = 0; if (!ip_to_uint(outStart, &base)) return false;
        unsigned long mask = (prefix == 0) ? 0 : (0xFFFFFFFFul << (32 - prefix));
        unsigned long network = base & mask;
        unsigned long broadcast = network | (~mask);
        char sBuf[64] = {0}, eBuf[64] = {0};
        uint_to_ip(network, sBuf, sizeof(sBuf));
        uint_to_ip(broadcast, eBuf, sizeof(eBuf));
        safe_strcpy(outStart, szStart, sBuf);
        safe_strcpy(outEnd, szEnd, eBuf);
        return true;
    }

    const char* dash = NULL; for (const char* p = in; *p; ++p) { if (*p == '-') { dash = p; break; } }
    if (!dash) return false;
    size_t leftLen = (size_t)(dash - in);
    if (leftLen >= szStart) leftLen = szStart - 1;
    strncpy(outStart, in, leftLen); outStart[leftLen] = '\0';
    const char* right = dash + 1;
    // If the right side has no '.', assume only end last octet
    bool rightHasDot = false; for (const char* p = right; *p; ++p) { if (*p == '.') { rightHasDot = true; break; } }
    if (!rightHasDot) {
        const char* lastDot = NULL; for (const char* p = outStart; *p; ++p) { if (*p == '.') lastDot = p; }
        if (!lastDot) return false;
        size_t prefixLen = (size_t)(lastDot - outStart) + 1; // includes '.'
        char buf[64]; if (prefixLen >= sizeof(buf)) return false;
        strncpy(buf, outStart, prefixLen); buf[prefixLen] = '\0';
        strncat(buf, right, sizeof(buf) - strlen(buf) - 1);
        safe_strcpy(outEnd, szEnd, buf);
    } else {
        safe_strcpy(outEnd, szEnd, right);
    }
    return true;
}

int main(void)
{
    // --- Window Configuration ---
    const int initialWidth = 1100;
    const int initialHeight = 700;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(initialWidth, initialHeight, "catnet_scanner (Raygui)");
    SetTargetFPS(60);
    // NEW: Layout constants for a consistent appearance
    const int padding = 10;         // Internal spacing for panels
    const int itemSpacing = 5;      // Spacing between buttons and other items
    const int toolbarH = 30;        // Height of the toolbar (reduced)
    const int statusH = 25;         // Height of the status bar
    const int sidebarW = 200;       // Width of the sidebar panel
    const int debugH = 160;         // Height of the debug panel
    int rowHeight = 25;             // Height of each row in the results list (scaled later)

    // --- Application state ---
    DeviceList results; device_list_init(&results);
    ScanConfig cfg; scan_config_init(&cfg);
    bool isScanning = false;
    apply_theme(true);
    // Increase global font size for readability
    GuiSetStyle(DEFAULT, TEXT_SIZE, 18);
    GuiSetStyle(DEFAULT, TEXT_SPACING, 2);
    // Adjust row height based on current text size
    rowHeight = GuiGetStyle(DEFAULT, TEXT_SIZE) + 12;
    int selectedIndex = -1;
    Vector2 scroll = (Vector2){0,0};
    Vector2 dbgScroll = (Vector2){0,0};
    scan_set_logger(gui_logger);
    // Shared IP range buffer used by toolbar TextBox and scan action
    static char ipRangeText[64] = "192.168.1.1-254";

    // --- Main loop ---
    while (!WindowShouldClose())
    {
        // --- Interaction ---

        // --- UI drawing (per frame) ---
        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();
        BeginDrawing();
        ClearBackground(g_bgColor);

        // --- 1. Toolbar (Fluid Layout) ---
        float currentX = (float)padding;
        if (GuiButton((Rectangle){ currentX, padding, 80, 25 }, "Scan")) {
            // Force range to primary subnet when LAN-only is enabled
            if (lanOnly) {
                SubnetV4 sn;
                if (net_get_primary_subnet(&sn)) {
                    char netBuf[64] = {0};
                    uint_to_ip(sn.network, netBuf, sizeof(netBuf));
                    unsigned long m = sn.mask; int prefix = 0; while (m) { prefix += (int)(m & 1u); m >>= 1; }
                    snprintf(ipRangeText, sizeof(ipRangeText), "%s/%d", netBuf, prefix);
                    gui_logger("LAN only: range forced to primary subnet");
                } else {
                    gui_logger("LAN only: failed to get primary subnet");
                }
            }
            if (!isScanning) {
                isScanning = true;
                g_statusText[0] = '\0'; strncat(g_statusText, "Scanning...", sizeof(g_statusText)-1);
                device_list_clear(&results);
                // Parse ipRangeText: supports "A.B.C.D-E" or "A.B.C.D-E.F.G.H"
                char startBuf[64] = {0}, endBuf[64] = {0};
                if (parse_range(ipRangeText, startBuf, sizeof(startBuf), endBuf, sizeof(endBuf))) {
                    unsigned long s = 0, e = 0;
                    if (ip_to_uint(startBuf, &s) && ip_to_uint(endBuf, &e) && e >= s) {
                        parallel_scan_start(s, e, &cfg, gui_logger);
                    } else {
                        SubnetV4 sn; if (net_get_primary_subnet(&sn)) { parallel_scan_start(sn.start_ip, sn.end_ip, &cfg, gui_logger); }
                        else { isScanning = false; gui_logger("Invalid IP range"); }
                    }
                } else {
                    SubnetV4 sn; if (net_get_primary_subnet(&sn)) { parallel_scan_start(sn.start_ip, sn.end_ip, &cfg, gui_logger); } else { isScanning = false; gui_logger("Invalid IP range"); }
                }
            }
        }
        currentX += 80 + itemSpacing;
        if (GuiButton((Rectangle){ currentX, padding, 80, 25 }, "Stop")) { if (isScanning) { parallel_scan_stop(); gui_logger("Scan cancelled by user"); isScanning = false; } }
        currentX += 80 + itemSpacing;
        if (GuiButton((Rectangle){ currentX, padding, 90, 25 }, "Clear Log")) { g_logCount = 0; }
        currentX += 90 + itemSpacing;
        if (GuiButton((Rectangle){ currentX, padding, 100, 25 }, quickToolsExpanded ? "Tools ▼" : "Tools ▲")) {
            quickToolsExpanded = !quickToolsExpanded;
        }
        currentX += 100 + itemSpacing;
        // Alternar LAN only
        GuiCheckBox((Rectangle){ currentX, padding, 22, 22 }, "LAN only", &lanOnly);
        currentX += 100 + itemSpacing;
        // Auto-detect local subnet and fill IP Range in CIDR
        if (GuiButton((Rectangle){ currentX, padding, 100, 25 }, "Auto-range")) {
            SubnetV4 sn;
            if (net_get_primary_subnet(&sn)) {
                char netBuf[64] = {0};
                uint_to_ip(sn.network, netBuf, sizeof(netBuf));
                // Count set bits in mask to get prefix length
                unsigned long m = sn.mask; int prefix = 0; while (m) { prefix += (int)(m & 1u); m >>= 1; }
                snprintf(ipRangeText, sizeof(ipRangeText), "%s/%d", netBuf, prefix);
                gui_logger("Auto-range: filled from primary subnet");
            } else {
                gui_logger("Auto-range failed: no primary subnet");
            }
        }
        currentX += 100 + itemSpacing;
        // IP controls aligned to the right (responsive) and support CIDR
        float ipRangeWidth = (float)(screenWidth - (int)currentX - padding - 120 - itemSpacing);
        if (ipRangeWidth < 260) ipRangeWidth = 260;
        GuiTextBox((Rectangle){ (float)(screenWidth - ipRangeWidth - padding), (float)padding, ipRangeWidth, 25 }, ipRangeText, sizeof(ipRangeText), false);
        GuiLabel((Rectangle){ (float)(screenWidth - ipRangeWidth - padding - 120 - itemSpacing), (float)(padding + 3), 120, 25 }, "IP Range/CIDR:");
        
        // Optional quick tools drawer below toolbar
        float toolsDrawerH = quickToolsExpanded ? 60.0f : 0.0f;
        // No navigation sidebar; use full width
        float effectiveSidebarW = 0.0f;
        if (quickToolsExpanded) {
            Rectangle quickArea = (Rectangle){ (float)(padding + effectiveSidebarW + padding), (float)(toolbarH + padding), (float)(screenWidth - effectiveSidebarW - padding*3), toolsDrawerH };
            GuiGroupBox(quickArea, "Quick Tools");
            float qx = quickArea.x + padding;
            float qy = quickArea.y + padding + 8;
            float tbW = 240.0f;
            GuiLabel((Rectangle){ qx, qy, 40, 22 }, "IP:");
            GuiTextBox((Rectangle){ qx + 40, qy, tbW, 22 }, quickIpText, sizeof(quickIpText), false);
            qx += 40 + tbW + itemSpacing;
            if (GuiButton((Rectangle){ qx, qy, 70, 22 }, "Ping")) {
                int ok = net_ping_ipv4(quickIpText);
                char msg[128]; snprintf(msg, sizeof(msg), ok ? "Ping %s: success" : "Ping %s: failed", quickIpText);
                gui_logger(msg);
            }
            qx += 70 + itemSpacing;
            if (GuiButton((Rectangle){ qx, qy, 80, 22 }, "DNS")) {
                char host[128] = {0};
                int ok = net_reverse_dns(quickIpText, host, sizeof(host));
                char msg[196]; snprintf(msg, sizeof(msg), ok && host[0] ? "DNS %s -> %s" : "DNS %s: not found", quickIpText, host);
                gui_logger(msg);
            }
            qx += 80 + itemSpacing;
            if (GuiButton((Rectangle){ qx, qy, 100, 22 }, "Ports")) {
                int open[64]; int openCount = 0;
                int ok = net_scan_ports(quickIpText, cfg.default_ports, cfg.default_ports_count, cfg.port_timeout_ms, open, &openCount);
                char msg[256] = {0};
                if (ok && openCount > 0) {
                    char buf[160] = {0};
                    for (int i = 0; i < openCount; ++i) {
                        char t[12]; snprintf(t, sizeof(t), "%d%s", open[i], (i<openCount-1?",":""));
                        strncat(buf, t, sizeof(buf) - strlen(buf) - 1);
                    }
                    snprintf(msg, sizeof(msg), "Open ports %s: %s", quickIpText, buf);
                } else {
                    snprintf(msg, sizeof(msg), "Open ports %s: none", quickIpText);
                }
                gui_logger(msg);
            }
        }

        // Navigation sidebar removed (not needed for current functionality)

        // --- 3. Main panel (ListView with columns) ---
        // Refresh results snapshot each frame
        if (isScanning) {
            parallel_scan_snapshot(&results);
            if (!parallel_scan_is_running()) {
                isScanning = false;
                // Contar apenas dispositivos encontrados (alive)
                size_t found = 0; for (size_t i = 0; i < results.count; ++i) { if (results.items[i].is_alive) ++found; }
                snprintf(g_statusText, sizeof(g_statusText), "Done. Devices: %zu", found);
            }
        }
        // Main content area (results)
        // AFTER: The layout is now calculated more robustly.
        Rectangle mainArea = (Rectangle){ (float)(padding + effectiveSidebarW + padding), (float)(toolbarH + padding * 2 + toolsDrawerH), (float)(screenWidth - effectiveSidebarW - padding * 3), (float)(screenHeight - toolbarH - statusH - debugH - padding * 4 - toolsDrawerH) };
        GuiGroupBox(mainArea, "Scan Results");

        // NEW: Dynamic Column System
        // Define widths and names here. The layout will adjust automatically.
        const char* headers[] = { "Status", "Hostname", "IP", "Open Ports", "MAC Address" };
        float innerW = mainArea.width - padding*2;
        float w0 = 70.0f;                 // Status icon column
        float w1 = innerW * 0.32f;        // Hostname (flex)
        float w2 = innerW * 0.20f;        // IP
        float w3 = innerW * 0.22f;        // Open Ports
        float w4 = innerW - (w0 + w1 + w2 + w3); // Remaining for MAC Address
        // Enforce minimums to avoid truncation
        const float minHost = 140.0f;
        const float minIP   = 160.0f;
        const float minPorts= 230.0f;
        const float minMAC  = 240.0f;
        if (w2 < minIP)   { float d = (minIP - w2);   w2 = minIP;   w1 = (w1 > minHost + d ? w1 - d : minHost); }
        if (w3 < minPorts){ float d = (minPorts - w3); w3 = minPorts; w1 = (w1 > minHost + d ? w1 - d : minHost); }
        if (w4 < minMAC)  { float d = (minMAC - w4);  w4 = minMAC;  w1 = (w1 > minHost + d ? w1 - d : minHost); }
        // If total still exceeds innerW, trim hostname down to min
        float totalW = w0 + w1 + w2 + w3 + w4;
        if (totalW > innerW) {
            float over = totalW - innerW;
            w1 = (w1 > minHost + over) ? (w1 - over) : minHost;
        }
        float columnWidths[] = { w0, w1, w2, w3, w4 };
        float columnOffsets[5];
        columnOffsets[0] = mainArea.x + padding;
        for (int i = 1; i < 5; i++) { columnOffsets[i] = columnOffsets[i-1] + columnWidths[i-1]; }

        // Draw the header using the column system
        for (int i = 0; i < 5; i++) {
            GuiLabel((Rectangle){ columnOffsets[i], mainArea.y + padding, columnWidths[i], 20 }, headers[i]);
        }
        GuiLine((Rectangle){ mainArea.x, mainArea.y + padding + 25, mainArea.width, 1 }, NULL);

        // Scroll area for results
        // Scrollable area for results
        Rectangle panelRec = { mainArea.x, mainArea.y + padding + 30, mainArea.width, mainArea.height - padding - 30 };
        Rectangle view = { 0 };
        // Mostrar apenas dispositivos com ping OK (is_alive)
        size_t visibleCount = 0; for (size_t i = 0; i < results.count; ++i) { if (results.items[i].is_alive) ++visibleCount; }
        GuiScrollPanel(panelRec, NULL, (Rectangle){ 0, 0, panelRec.width - 20, (float)visibleCount * rowHeight }, &scroll, &view);
        
        BeginScissorMode((int)view.x, (int)view.y, (int)view.width, (int)view.height);

        int displayIndex = 0;
        for (int i = 0; i < (int)results.count; i++)
        {
            const DeviceInfo* di = &results.items[i];
            if (!di->is_alive) continue; // filtrar apenas encontrados
            float yPos = panelRec.y + (float)(displayIndex * rowHeight) + scroll.y;

            // "Zebra Stripes" para melhor leitura
            if (displayIndex % 2 != 0) {
                DrawRectangle(panelRec.x, yPos, panelRec.width, (float)rowHeight, (Color){40, 40, 40, 255});
            }
            if (selectedIndex == displayIndex) {
                DrawRectangle(panelRec.x, yPos, panelRec.width, (float)rowHeight, (Color){0, 120, 215, 100});
            }

            if (CheckCollisionPointRec(GetMousePosition(), (Rectangle){ panelRec.x, yPos, panelRec.width, (float)rowHeight })) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) selectedIndex = displayIndex;
            }

            // Desenhar dados por coluna
            DrawCircle(columnOffsets[0] + 15, yPos + rowHeight/2, 5, LIME);
            GuiLabel((Rectangle){ columnOffsets[1], yPos, columnWidths[1], (float)rowHeight }, di->hostname[0] ? di->hostname : "(unnamed)");
            GuiLabel((Rectangle){ columnOffsets[2], yPos, columnWidths[2], (float)rowHeight }, di->ip);
            char portsBuf[128] = {0};
            for (int p = 0; p < di->open_ports_count; ++p) {
                char tmp[16]; snprintf(tmp, sizeof(tmp), "%d%s", di->open_ports[p], (p<di->open_ports_count-1?",":""));
                strncat(portsBuf, tmp, sizeof(portsBuf)-strlen(portsBuf)-1);
            }
            GuiLabel((Rectangle){ columnOffsets[3], yPos, columnWidths[3], (float)rowHeight }, portsBuf);
            GuiLabel((Rectangle){ columnOffsets[4], yPos, columnWidths[4], (float)rowHeight }, di->mac);
            displayIndex++;
        }

        EndScissorMode();

        // --- 4. Debug Terminal ---
        Rectangle dbgBox = (Rectangle){ mainArea.x, mainArea.y + mainArea.height + padding, mainArea.width, (float)(debugH) };
        GuiGroupBox(dbgBox, "Debug Log");
        // ... (debug panel logic is unchanged, but using padding) ...
        Rectangle dbgPanelRec = { dbgBox.x + padding, dbgBox.y + padding + 10, dbgBox.width - padding*2, dbgBox.height - padding*2 - 10 };
        Rectangle dbgView = { 0 };
        int lineH = GuiGetStyle(DEFAULT, TEXT_SIZE) + 8;
        float contentH = (float)((g_logCount < 256 ? g_logCount : 256) * lineH);
        GuiScrollPanel(dbgPanelRec, NULL, (Rectangle){ 0, 0, dbgPanelRec.width - 20, contentH }, &dbgScroll, &dbgView);
        BeginScissorMode((int)dbgView.x, (int)dbgView.y, (int)dbgView.width, (int)dbgView.height);
        int linesToShow = (g_logCount < 256 ? g_logCount : 256);
        for (int i = 0; i < linesToShow; ++i) {
            float y = dbgPanelRec.y + (float)(i * lineH) + dbgScroll.y;
            GuiLabel((Rectangle){ dbgPanelRec.x + 5, y, dbgPanelRec.width - 30, (float)lineH }, g_logLines[i]);
        }
        EndScissorMode();

        // --- 5. Status bar ---
        char statusBarText[160];
        // Mostrar contagem apenas dos dispositivos encontrados
        size_t foundStatus = 0; for (size_t i = 0; i < results.count; ++i) { if (results.items[i].is_alive) ++foundStatus; }
        snprintf(statusBarText, sizeof(statusBarText), "%s | Devices: %zu%s", g_statusText, foundStatus, (isScanning?" (scanning)":""));
        GuiStatusBar((Rectangle){ 0, screenHeight - statusH, screenWidth, statusH }, statusBarText);

        EndDrawing();
    }

    // --- Shutdown ---
    device_list_clear(&results);
    CloseWindow();
    return 0;
}