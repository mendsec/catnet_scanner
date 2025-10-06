#include "export.h"
#include <stdio.h>

int export_results_to_file(const char* path, const DeviceList* list) {
    FILE* f = fopen(path, "w");
    if (!f) return 0;
    fprintf(f, "IP;Hostname;MAC;Status;Ports\n");
    for (size_t i=0; i<list->count; ++i) {
        const DeviceInfo* di = &list->items[i];
        fprintf(f, "%s;%s;%s;%s;", di->ip, di->hostname[0]?di->hostname:"", di->mac[0]?di->mac:"", di->is_alive?"UP":"DOWN");
        for (int p=0; p<di->open_ports_count; ++p) {
            fprintf(f, "%d%s", di->open_ports[p], (p<di->open_ports_count-1?",":""));
        }
        fprintf(f, "\n");
    }
    fclose(f);
    return 1;
}