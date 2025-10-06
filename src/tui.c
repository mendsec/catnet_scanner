#include "tui.h"
#include <stdio.h>
#include <string.h>

static void get_console_size(HANDLE h, int* w, int* hgt) {
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(h, &info);
    *w = info.srWindow.Right - info.srWindow.Left + 1;
    *hgt = info.srWindow.Bottom - info.srWindow.Top + 1;
}

void tui_init(TuiState* st) {
    st->hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    get_console_size(st->hConsole, &st->width, &st->height);
    st->selected_index = 0;
    st->scroll_offset = 0;
    CONSOLE_CURSOR_INFO ci = {0};
    ci.bVisible = FALSE; ci.dwSize = 1;
    SetConsoleCursorInfo(st->hConsole, &ci);
}

void tui_goto(int x, int y) {
    COORD c = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

void tui_clear_area(int x, int y, int w, int h) {
    DWORD written = 0;
    HANDLE hd = GetStdHandle(STD_OUTPUT_HANDLE);
    for (int row = 0; row < h; ++row) {
        tui_goto(x, y + row);
        for (int col = 0; col < w; ++col) {
            WriteConsoleA(hd, " ", 1, &written, NULL);
        }
    }
}

void tui_draw_frame(TuiState* st) {
    HANDLE h = st->hConsole; DWORD w;
    // Top border
    tui_goto(0,0);
    for (int i=0; i<st->width; ++i) WriteConsoleA(h, "=", 1, &w, NULL);
    // Header area
    tui_goto(0,1);
    for (int i=0; i<st->width; ++i) WriteConsoleA(h, " ", 1, &w, NULL);
    // Split line
    tui_goto(0,2);
    for (int i=0; i<st->width; ++i) WriteConsoleA(h, "-", 1, &w, NULL);
    // Results area (3..height-4)
    for (int y=3; y<st->height-4; ++y) {
        tui_goto(0,y);
        for (int i=0; i<st->width; ++i) WriteConsoleA(h, " ", 1, &w, NULL);
    }
    // Split line
    tui_goto(0, st->height-4);
    for (int i=0; i<st->width; ++i) WriteConsoleA(h, "-", 1, &w, NULL);
    // Log area
    for (int y=st->height-3; y<st->height; ++y) {
        tui_goto(0,y);
        for (int i=0; i<st->width; ++i) WriteConsoleA(h, " ", 1, &w, NULL);
    }
}

void tui_draw_header(TuiState* st, const char* title) {
    HANDLE h = st->hConsole; DWORD w;
    tui_goto(1,1);
    char line[512];
    snprintf(line, sizeof(line), "[F1] Local  [F2] Faixa  [F3] Ping  [F4] DNS  [F5] Export  [Q] Sair   | %s", title);
    WriteConsoleA(h, line, (DWORD)strlen(line), &w, NULL);
}

void tui_draw_results(TuiState* st, const DeviceList* list) {
    HANDLE h = st->hConsole; DWORD w;
    int area_h = st->height - 7;
    int start = st->scroll_offset;
    int end = start + area_h;
    if (end > (int)list->count) end = (int)list->count;

    for (int i = start, row = 3; i < end; ++i, ++row) {
        tui_goto(1, row);
        char line[1024];
        const DeviceInfo* di = &list->items[i];
        char ports[256] = {0};
        for (int p=0; p<di->open_ports_count; ++p) {
            char tmp[32];
            snprintf(tmp, sizeof(tmp), "%s%d", (p==0?"":" "), di->open_ports[p]);
            strncat(ports, tmp, sizeof(ports)-strlen(ports)-1);
        }
        snprintf(line, sizeof(line), "%c %-15s  %-30s  MAC: %-17s  %s", (i==st->selected_index?'>' :' '), di->ip, (di->hostname[0]?di->hostname:"(desconhecido)"), (di->mac[0]?di->mac:"--"), (di->is_alive?"UP":"DOWN"));
        WriteConsoleA(h, line, (DWORD)strlen(line), &w, NULL);
        if (di->open_ports_count > 0) {
            tui_goto(60, row);
            char pline[300];
            snprintf(pline, sizeof(pline), "Ports: %s", ports);
            WriteConsoleA(h, pline, (DWORD)strlen(pline), &w, NULL);
        }
    }
}

void tui_draw_logs(TuiState* st, const char* logline) {
    HANDLE h = st->hConsole; DWORD w;
    tui_goto(1, st->height-2);
    WriteConsoleA(h, logline, (DWORD)strlen(logline), &w, NULL);
}

int tui_read_key(INPUT_RECORD* rec) {
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD count = 0;
    ReadConsoleInput(hIn, rec, 1, &count);
    if (count == 0) return 0;
    return 1;
}