// Main window built with Raygui and C
// To build, configure environment variables (or build.ps1 parameters):
//   RAYLIB_INCLUDE: path to raylib headers (containing raylib.h)
//   RAYLIB_LIBS: path to raylib libraries (containing raylib.lib)
// build.ps1 has been extended to support -UI Raygui and link raylib.

#include "raylib.h"
#include <stdbool.h>

// Define raygui implementation header in a C file to avoid build issues
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "app.h"
#include "scan.h"
#include <string.h>

static void apply_theme(bool dark)
{
    Color bg = dark ? (Color){24,24,24,255} : RAYWHITE;
    Color text = dark ? (Color){235,235,235,255} : (Color){20,20,20,255};
    Color line = dark ? (Color){80,80,80,255} : (Color){180,180,180,255};
    Color base = dark ? (Color){60,60,60,255} : (Color){220,220,220,255};
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
static void gui_logger(const char* msg)
{
    if (!msg) { g_statusText[0] = '\0'; return; }
    strncpy(g_statusText, msg, sizeof(g_statusText) - 1);
    g_statusText[sizeof(g_statusText) - 1] = '\0';
}

static bool parse_range(const char* in, char* outStart, size_t szStart, char* outEnd, size_t szEnd)
{
    // Accepts formats: "A.B.C.D-E" (last octet only) or "A.B.C.D-E.F.G.H"
    const char* dash = NULL;
    for (const char* p = in; *p; ++p) { if (*p == '-') { dash = p; break; } }
    if (!dash) return false;
    size_t leftLen = (size_t)(dash - in);
    if (leftLen >= szStart) leftLen = szStart - 1;
    strncpy(outStart, in, leftLen); outStart[leftLen] = '\0';
    const char* right = dash + 1;
    // If the right side has no '.', assume only end last octet
    bool rightHasDot = false; for (const char* p = right; *p; ++p) { if (*p == '.') { rightHasDot = true; break; } }
    if (!rightHasDot) {
        // Build end using outStart prefix (up to last '.')
        const char* lastDot = NULL; for (const char* p = outStart; *p; ++p) { if (*p == '.') lastDot = p; }
        if (!lastDot) return false;
        size_t prefixLen = (size_t)(lastDot - outStart) + 1; // inclui '.'
        char buf[64];
        if (prefixLen >= sizeof(buf)) return false;
        strncpy(buf, outStart, prefixLen); buf[prefixLen] = '\0';
        strncat(buf, right, sizeof(buf) - strlen(buf) - 1);
        safe_strcpy(outEnd, szEnd, buf);
        // outStart already is A.B.C.D (left side)
    } else {
        safe_strcpy(outEnd, szEnd, right);
    }
    return true;
}

int main(void)
{
    // --- Window setup ---
    const int screenWidth = 900;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "catnet_scanner (Raygui)");
    SetTargetFPS(60);

    // --- Application state ---
    DeviceList results; device_list_init(&results);
    ScanConfig cfg; scan_config_init(&cfg);
    bool isScanning = false;
    apply_theme(true);
    int selectedIndex = -1;
    Vector2 scroll = (Vector2){0,0};
    scan_set_logger(gui_logger);

    // --- Main loop ---
    while (!WindowShouldClose())
    {
        // --- Interaction ---

        // --- UI drawing (per frame) ---
        BeginDrawing();
        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

        // --- 1. Toolbar (manual layout)
        // Rectangles define position and size for each control
        if (GuiButton((Rectangle){ 10, 10, 80, 25 }, "Scan")) {
            if (!isScanning) {
                isScanning = true;
                g_statusText[0] = '\0'; strncat(g_statusText, "Scanning...", sizeof(g_statusText)-1);
                net_init();
                device_list_clear(&results);
                // Parse ipRangeText: supports "A.B.C.D-E" or "A.B.C.D-E.F.G.H"
                static char ipRangeText[64] = "192.168.1.1-254";
                char startIp[64] = {0}, endIp[64] = {0};
                if (parse_range(ipRangeText, startIp, sizeof(startIp), endIp, sizeof(endIp))) {
                    scan_range(&results, &cfg, startIp, endIp);
                } else {
                    scan_subnet(&results, &cfg);
                }
                net_cleanup();
                isScanning = false;
                snprintf(g_statusText, sizeof(g_statusText), "Done. Devices: %zu", results.count);
            }
        }
        if (GuiButton((Rectangle){ 100, 10, 80, 25 }, "Stop")) { /* future: cancel scan */ }
        // Dark theme enforced by default; toggle removed.
        GuiLabel((Rectangle){ screenWidth - 320, 10, 60, 25 }, "IP Range:");
        // TextBox storage variable
        static char ipRangeText[64] = "192.168.1.1-254";
        GuiTextBox((Rectangle){ screenWidth - 250, 10, 240, 25 }, ipRangeText, sizeof(ipRangeText), false);
        
        // --- 2. Sidebar (TreeView simulation) ---
        GuiGroupBox((Rectangle){ 10, 45, 200, screenHeight - 95 }, "Navigation");
        
        // Use a button as a node to expand/collapse
        if (GuiButton((Rectangle){ 20, 60, 180, 20 }, favoritesExpanded ? "[-] Favorites" : "[+] Favorites")) {
            favoritesExpanded = !favoritesExpanded;
        }
        if (favoritesExpanded) {
            GuiLabel((Rectangle){ 35, 85, 100, 20 }, "- Servers");
            GuiLabel((Rectangle){ 35, 105, 100, 20 }, "- Printers");
        }
        GuiLabel((Rectangle){ 20, 130, 180, 20 }, "Scan History");
        GuiLabel((Rectangle){ 20, 150, 180, 20 }, "Scheduled Tasks");

        // --- 3. Main panel (ListView with columns) ---
        GuiGroupBox((Rectangle){ 220, 45, screenWidth - 230, screenHeight - 95 }, "Scan Results");
        
        // List header
        GuiLabel((Rectangle){ 230, 55, 50, 20 }, "Status");
        GuiLabel((Rectangle){ 290, 55, 200, 20 }, "Hostname");
        GuiLabel((Rectangle){ 490, 55, 120, 20 }, "IP");
        GuiLabel((Rectangle){ 620, 55, 150, 20 }, "Ports");
        GuiLabel((Rectangle){ 780, 55, 100, 20 }, "MAC");
        GuiLine((Rectangle){ 230, 75, screenWidth - 250, 1 }, NULL);

        // Scroll area for results
        Rectangle panelRec = { 220, 80, (float)(screenWidth - 230), (float)(screenHeight - 130) };
        Rectangle view = { 0 };
        GuiScrollPanel(panelRec, NULL, (Rectangle){ 0, 0, panelRec.width - 20, (float)results.count * 25 }, &scroll, &view);
        
        BeginScissorMode((int)view.x, (int)view.y, (int)view.width, (int)view.height);

        for (int i = 0; i < (int)results.count; i++)
        {
            // Calculate Y position of each row based on scroll
            float yPos = panelRec.y + (float)(i * 25) - scroll.y;

            // Draw selection rectangle if the row is selected
            if (selectedIndex == i) {
                DrawRectangle(panelRec.x, yPos, panelRec.width, 25, SKYBLUE);
            }
            
            // Check for click to select the row
            if (CheckCollisionPointRec(GetMousePosition(), (Rectangle){ panelRec.x, yPos, panelRec.width, 25 })) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    selectedIndex = i;
                }
            }

            // Draw data for each column manually
            const DeviceInfo* di = &results.items[i];
            DrawCircle(panelRec.x + 20, yPos + 12, 5, di->is_alive ? GREEN : RED);
            GuiLabel((Rectangle){ 290, yPos, 200, 25 }, di->hostname[0] ? di->hostname : "(unnamed)");
            GuiLabel((Rectangle){ 490, yPos, 120, 25 }, di->ip);
            char portsBuf[128] = {0};
            for (int p = 0; p < di->open_ports_count; ++p) {
                char tmp[16]; snprintf(tmp, sizeof(tmp), "%d%s", di->open_ports[p], (p<di->open_ports_count-1?",":""));
                strncat(portsBuf, tmp, sizeof(portsBuf)-strlen(portsBuf)-1);
            }
            GuiLabel((Rectangle){ 620, yPos, 150, 25 }, portsBuf);
            GuiLabel((Rectangle){ 780, yPos, 100, 25 }, di->mac);
        }

        EndScissorMode();

        // --- 4. Status bar ---
        char statusBar[160];
        snprintf(statusBar, sizeof(statusBar), "%s | Devices: %zu", g_statusText, results.count);
        GuiStatusBar((Rectangle){ 0, screenHeight - 30, screenWidth, 30 }, statusBar);

        EndDrawing();
    }

    // --- Shutdown ---
    device_list_clear(&results);
    CloseWindow();
    return 0;
}