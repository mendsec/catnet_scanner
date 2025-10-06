#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <stdlib.h>

#include "app.h"
#include "tui.h"
#include "net.h"
#include "scan.h"
#include "utils.h"
#include "export.h"

// DeviceList helpers moved to src/list.c to be shared with GUI build

static void prompt_line(const char* title, char* buf, size_t bufsz) {
    printf("\n%s: ", title);
    fflush(stdout);
    if (fgets(buf, (int)bufsz, stdin)) {
        trim_newline(buf);
    }
}

static TuiState* g_tui_log_target = NULL;
static void scan_log_adapter(const char* msg) {
    if (g_tui_log_target) tui_draw_logs(g_tui_log_target, msg);
}

int main() {
    if (!net_init()) {
        fprintf(stderr, "Failed to initialize Winsock.\n");
        return 1;
    }

    TuiState tui; tui_init(&tui);
    DeviceList results; device_list_init(&results);
    ScanConfig cfg; scan_config_init(&cfg);

    tui_draw_frame(&tui);
    tui_draw_header(&tui, "Ready");
    tui_draw_logs(&tui, "Use F1..F5 to run actions. Q/Esc to quit.");

    INPUT_RECORD rec;
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    SetConsoleMode(hIn, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);

    int running = 1;
    while (running) {
        if (!tui_read_key(&rec)) continue;
        if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown) {
            WORD vk = rec.Event.KeyEvent.wVirtualKeyCode;
            switch (vk) {
            case VK_F1: {
                device_list_clear(&results);
                tui_draw_header(&tui, "Scanning local subnet...");
                tui_draw_logs(&tui, "Please wait: running ping/DNS/port scans...");
                g_tui_log_target = &tui; scan_set_logger(scan_log_adapter);
                scan_subnet(&results, &cfg);
                scan_set_logger(NULL); g_tui_log_target = NULL;
                tui_draw_frame(&tui);
                tui_draw_header(&tui, "Local subnet scanned");
                tui_draw_results(&tui, &results);
                tui_draw_logs(&tui, "Done.");
                break; }
            case VK_F2: {
                char start[64], end[64];
                tui_draw_logs(&tui, "Enter the range in the console (current window)\n");
                printf("\nEnter start IP: "); fgets(start, sizeof(start), stdin); trim_newline(start);
                printf("Enter end IP: "); fgets(end, sizeof(end), stdin); trim_newline(end);
                device_list_clear(&results);
                tui_draw_header(&tui, "Scanning range...");
                g_tui_log_target = &tui; scan_set_logger(scan_log_adapter);
                scan_range(&results, &cfg, start, end);
                scan_set_logger(NULL); g_tui_log_target = NULL;
                tui_draw_frame(&tui);
                tui_draw_header(&tui, "Range scanned");
                tui_draw_results(&tui, &results);
                tui_draw_logs(&tui, "Done.");
                break; }
            case VK_F3: {
                char ip[64];
                tui_draw_logs(&tui, "Enter IP for ping in the console\n");
                printf("\nEnter IP: "); fgets(ip, sizeof(ip), stdin); trim_newline(ip);
                int ok = net_ping_ipv4(ip);
                char msg[256];
                snprintf(msg, sizeof(msg), "Ping %s: %s", ip, ok?"OK":"Failed");
                tui_draw_logs(&tui, msg);
                break; }
            case VK_F4: {
                char ip[64], host[256];
                tui_draw_logs(&tui, "Enter IP for reverse DNS in the console\n");
                printf("\nEnter IP: "); fgets(ip, sizeof(ip), stdin); trim_newline(ip);
                if (net_reverse_dns(ip, host, sizeof(host))) {
                    char msg[512]; snprintf(msg, sizeof(msg), "DNS %s => %s", ip, host);
                    tui_draw_logs(&tui, msg);
                } else {
                    char msg[256]; snprintf(msg, sizeof(msg), "DNS %s => (no name)", ip);
                    tui_draw_logs(&tui, msg);
                }
                break; }
            case VK_F5: {
                char path[MAX_PATH];
                snprintf(path, sizeof(path), "results.txt");
                if (export_results_to_file(path, &results)) {
                    char msg[256]; snprintf(msg, sizeof(msg), "Exported to %s", path);
                    tui_draw_logs(&tui, msg);
                } else {
                    tui_draw_logs(&tui, "Failed to export results.");
                }
                break; }
            case VK_UP: {
                if (tui.selected_index > 0) {
                    tui.selected_index--;
                    if (tui.selected_index < tui.scroll_offset) tui.scroll_offset = tui.selected_index;
                    tui_draw_results(&tui, &results);
                }
                break; }
            case VK_DOWN: {
                if (tui.selected_index + 1 < (int)results.count) {
                    tui.selected_index++;
                    int area_h = tui.height - 7;
                    if (tui.selected_index >= tui.scroll_offset + area_h) tui.scroll_offset++;
                    tui_draw_results(&tui, &results);
                }
                break; }
            case VK_ESCAPE: running = 0; break;
            default: {
                // 'Q' key to quit
                CHAR ch = rec.Event.KeyEvent.uChar.AsciiChar;
                if (ch == 'q' || ch == 'Q') running = 0;
                break; }
            }
        }
    }

    device_list_clear(&results);
    net_cleanup();
    return 0;
}