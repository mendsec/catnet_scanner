#include "scan.h"
#include "net.h"
#include "utils.h"
#include <string.h>
#include <stdio.h>

static ScanLogFn g_logger = NULL;
void scan_set_logger(ScanLogFn fn) { g_logger = fn; }

void scan_config_init(ScanConfig* cfg) {
    int ports[] = {22, 80, 443, 139, 445, 3389};
    cfg->default_ports_count = (int)(sizeof(ports)/sizeof(ports[0]));
    for (int i = 0; i < cfg->default_ports_count; ++i) cfg->default_ports[i] = ports[i];
    cfg->port_timeout_ms = 500;
}

void identify_device(DeviceInfo* info, const ScanConfig* cfg) {
    if (g_logger) { char msg[128]; snprintf(msg, sizeof(msg), "Ping %s...", info->ip); g_logger(msg); }
    info->is_alive = net_ping_ipv4(info->ip);
    if (info->is_alive) {
        if (g_logger) { char msg[128]; snprintf(msg, sizeof(msg), "DNS %s...", info->ip); g_logger(msg); }
        net_reverse_dns(info->ip, info->hostname, sizeof(info->hostname));
        if (g_logger) { char msg[160]; snprintf(msg, sizeof(msg), "MAC %s...", info->ip); g_logger(msg); }
        net_get_mac(info->ip, info->mac, sizeof(info->mac));
        info->open_ports_count = 0;
        if (g_logger) { char msg[160]; snprintf(msg, sizeof(msg), "Ports %s...", info->ip); g_logger(msg); }
        net_scan_ports(info->ip, cfg->default_ports, cfg->default_ports_count, cfg->port_timeout_ms, info->open_ports, &info->open_ports_count);
        if (g_logger) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Completed %s: %s, %s, %d ports", info->ip, (info->hostname[0]?info->hostname:"(unnamed)"), (info->mac[0]?info->mac:"MAC --"), info->open_ports_count);
            g_logger(msg);
        }
    } else {
        if (g_logger) { char msg[128]; snprintf(msg, sizeof(msg), "Ping failed %s", info->ip); g_logger(msg); }
    }
}

int scan_subnet(DeviceList* out, const ScanConfig* cfg) {
    SubnetV4 sn;
    if (!net_get_primary_subnet(&sn)) {
        if (g_logger) g_logger("Error: Failed to get primary subnet.");
        return 0;
    }
    for (unsigned long ip = sn.start_ip; ip <= sn.end_ip; ++ip) {
        DeviceInfo di; memset(&di, 0, sizeof(di));
        uint_to_ip(ip, di.ip, sizeof(di.ip));
        if (g_logger) { char msg[128]; snprintf(msg, sizeof(msg), "Processing %s", di.ip); g_logger(msg); }
        identify_device(&di, cfg);
        device_list_push(out, &di);
    }
    return 1;
}

int scan_range(DeviceList* out, const ScanConfig* cfg, const char* start_ip, const char* end_ip) {
    unsigned long start, end;
    if (!ip_to_uint(start_ip, &start)) {
        if (g_logger) { char msg[128]; snprintf(msg, sizeof(msg), "Error: Invalid start IP '%s'", start_ip); g_logger(msg); }
        return 0;
    }
    if (!ip_to_uint(end_ip, &end)) {
        if (g_logger) { char msg[128]; snprintf(msg, sizeof(msg), "Error: Invalid end IP '%s'", end_ip); g_logger(msg); }
        return 0;
    }
    if (end < start) {
        if (g_logger) g_logger("Error: End IP is smaller than start IP.");
        return 0;
    }
    for (unsigned long ip = start; ip <= end; ++ip) {
        DeviceInfo di; memset(&di, 0, sizeof(di));
        uint_to_ip(ip, di.ip, sizeof(di.ip));
        if (g_logger) { char msg[128]; snprintf(msg, sizeof(msg), "Processing %s", di.ip); g_logger(msg); }
        identify_device(&di, cfg);
        device_list_push(out, &di);
    }
    return 1;
}