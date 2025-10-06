#ifndef APP_H
#define APP_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

typedef struct {
    char ip[64];
    char hostname[256];
    char mac[32];
    int is_alive; // ping OK
    int open_ports[32];
    int open_ports_count;
} DeviceInfo;

typedef struct {
    DeviceInfo* items;
    size_t count;
    size_t capacity;
} DeviceList;



#ifdef __cplusplus
extern "C" {
#endif
void device_list_init(DeviceList* list);
void device_list_clear(DeviceList* list);
void device_list_push(DeviceList* list, const DeviceInfo* info);
#ifdef __cplusplus
}
#endif

#endif // APP_H