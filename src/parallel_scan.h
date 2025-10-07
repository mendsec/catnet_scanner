#ifndef PARALLEL_SCAN_H
#define PARALLEL_SCAN_H

#include "app.h"
#include "scan.h"

#ifdef __cplusplus
extern "C" {
#endif

// Starts a parallel scan across [start_ip_uint, end_ip_uint] inclusive.
// Returns 1 on success, 0 on failure.
int parallel_scan_start(unsigned long start_ip_uint,
                        unsigned long end_ip_uint,
                        const ScanConfig* cfg,
                        ScanLogFn logger);

// Requests cancellation and waits for workers to finish.
void parallel_scan_stop(void);

// Copies current results snapshot into 'out'.
// 'out' must be initialized; its contents will be replaced.
void parallel_scan_snapshot(DeviceList* out);

// Returns 1 if a scan is currently running.
int parallel_scan_is_running(void);

#ifdef __cplusplus
}
#endif

#endif // PARALLEL_SCAN_H