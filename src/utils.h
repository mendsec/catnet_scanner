#ifndef UTILS_H
#define UTILS_H

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include <winsock2.h>
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif
int ip_to_uint(const char* ip, unsigned long* out);
void uint_to_ip(unsigned long ip, char* buf, size_t buflen);
void trim_newline(char* s);
void safe_strcpy(char* dst, size_t dstsz, const char* src);
#ifdef __cplusplus
}
#endif

#endif // UTILS_H