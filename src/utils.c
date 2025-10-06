#include "utils.h"
#include <string.h>
#include <stdio.h>

int ip_to_uint(const char* ip, unsigned long* out) {
    unsigned long a = inet_addr(ip);
    if (a == INADDR_NONE && strcmp(ip, "255.255.255.255") != 0) {
        return 0;
    }
    *out = ntohl(a);
    return 1;
}

void uint_to_ip(unsigned long ip, char* buf, size_t buflen) {
    struct in_addr addr;
    addr.S_un.S_addr = htonl(ip);
    const char* s = inet_ntoa(addr);
    if (s) safe_strcpy(buf, buflen, s);
    else if (buflen > 0) buf[0] = '\0';
}

void trim_newline(char* s) {
    if (!s) return;
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r')) {
        s[n-1] = '\0';
        n--;
    }
}

void safe_strcpy(char* dst, size_t dstsz, const char* src) {
    if (!dst || dstsz == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    strncpy(dst, src, dstsz - 1);
    dst[dstsz - 1] = '\0';
}