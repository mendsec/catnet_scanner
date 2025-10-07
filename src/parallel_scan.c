#include "parallel_scan.h"
#include "net.h"
#include "utils.h"
#include <string.h>
#include <stdio.h>
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include <windows.h>

typedef struct {
    unsigned long start_ip;
    unsigned long end_ip;
    volatile LONG next_ip; // atomic counter
    volatile LONG cancel;
    ScanConfig cfg;
    DeviceList results;
    CRITICAL_SECTION results_lock;
    ScanLogFn logger;
    int num_threads;
    HANDLE threads[64];
    // Simple rate limiter: devices per second
    volatile LONG rate_count;
    ULONGLONG rate_window_start;
    LONG rate_limit;
} ScanState;

static ScanState g_state = {0};

static DWORD WINAPI worker_proc(LPVOID lpParam) {
    ScanState* st = (ScanState*)lpParam;
    for (;;) {
        if (st->cancel) break;
        LONG ip = InterlockedIncrement(&st->next_ip) - 1;
        if ((unsigned long)ip > st->end_ip) break;
        // Rate limiting: allow up to rate_limit devices per second
        for (;;) {
            ULONGLONG now = GetTickCount64();
            if (now - st->rate_window_start >= 1000) {
                st->rate_window_start = now;
                InterlockedExchange(&st->rate_count, 0);
            }
            LONG cur = st->rate_count;
            if (cur < st->rate_limit) { InterlockedIncrement(&st->rate_count); break; }
            Sleep(1);
            if (st->cancel) break;
        }
        DeviceInfo di; memset(&di, 0, sizeof(di));
        uint_to_ip((unsigned long)ip, di.ip, sizeof(di.ip));
        if (st->logger) {
            char msg[96]; snprintf(msg, sizeof(msg), "Scanning %s", di.ip);
            st->logger(msg);
        }
        identify_device(&di, &st->cfg);
        EnterCriticalSection(&st->results_lock);
        device_list_push(&st->results, &di);
        LeaveCriticalSection(&st->results_lock);
    }
    return 0;
}

int parallel_scan_start(unsigned long start_ip_uint,
                        unsigned long end_ip_uint,
                        const ScanConfig* cfg,
                        ScanLogFn logger) {
    if (g_state.cancel == 0 && g_state.num_threads > 0) return 0; // already running
    memset(&g_state, 0, sizeof(g_state));
    g_state.start_ip = start_ip_uint;
    g_state.end_ip = end_ip_uint;
    g_state.next_ip = (LONG)start_ip_uint;
    g_state.cancel = 0;
    if (cfg) g_state.cfg = *cfg; else scan_config_init(&g_state.cfg);
    g_state.logger = logger;
    device_list_init(&g_state.results);
    InitializeCriticalSection(&g_state.results_lock);
    g_state.rate_count = 0;
    g_state.rate_window_start = GetTickCount64();
    g_state.rate_limit = 200; // devices per second (coarse)
    if (!net_init()) {
        if (g_state.logger) g_state.logger("Network init failed");
        DeleteCriticalSection(&g_state.results_lock);
        return 0;
    }
    int desired = 16; // default thread count
    SYSTEM_INFO si; GetSystemInfo(&si);
    int hw = (int)si.dwNumberOfProcessors;
    if (hw > 0) desired = (hw * 2);
    if (desired > 64) desired = 64;
    g_state.num_threads = desired;
    for (int i = 0; i < g_state.num_threads; ++i) {
        g_state.threads[i] = CreateThread(NULL, 0, worker_proc, &g_state, 0, NULL);
    }
    if (g_state.logger) {
        char msg[96]; snprintf(msg, sizeof(msg), "Workers started: %d", g_state.num_threads);
        g_state.logger(msg);
    }
    return 1;
}

void parallel_scan_stop(void) {
    if (g_state.num_threads <= 0) return;
    g_state.cancel = 1;
    WaitForMultipleObjects(g_state.num_threads, g_state.threads, TRUE, 5000);
    for (int i = 0; i < g_state.num_threads; ++i) {
        if (g_state.threads[i]) CloseHandle(g_state.threads[i]);
        g_state.threads[i] = NULL;
    }
    g_state.num_threads = 0;
    net_cleanup();
}

void parallel_scan_snapshot(DeviceList* out) {
    if (!out) return;
    EnterCriticalSection(&g_state.results_lock);
    device_list_clear(out);
    for (size_t i = 0; i < g_state.results.count; ++i) {
        device_list_push(out, &g_state.results.items[i]);
    }
    LeaveCriticalSection(&g_state.results_lock);
}

int parallel_scan_is_running(void) {
    return (g_state.num_threads > 0) && (g_state.cancel == 0);
}