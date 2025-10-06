#ifndef TUI_H
#define TUI_H

#include "app.h"

void tui_init(TuiState* st);
void tui_draw_frame(TuiState* st);
void tui_draw_header(TuiState* st, const char* title);
void tui_draw_results(TuiState* st, const DeviceList* list);
void tui_draw_logs(TuiState* st, const char* logline);
int tui_read_key(INPUT_RECORD* rec);
void tui_clear_area(int x, int y, int w, int h);
void tui_goto(int x, int y);

#endif // TUI_H