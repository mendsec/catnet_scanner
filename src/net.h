#ifndef NET_H
#define NET_H

#include "app.h"
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <icmpapi.h>


typedef struct {
    unsigned long network;
    unsigned long start_ip;
    unsigned long end_ip;
    unsigned long mask;
} SubnetV4;

#ifdef __cplusplus
extern "C" {
#endif
int net_init(void);
void net_cleanup(void);

int net_ping_ipv4(const char* ip);
int net_reverse_dns(const char* ip, char* hostname, size_t hostsz);
int net_get_mac(const char* ip, char* macbuf, size_t macsz);
int net_scan_ports(const char* ip, const int* ports, int ports_count, int timeout_ms, int* open_ports, int* open_count);

int net_get_primary_subnet(SubnetV4* out);
#ifdef __cplusplus
}
#endif

#endif // NET_H