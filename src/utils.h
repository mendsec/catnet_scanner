#ifndef UTILS_H
#define UTILS_H

// Keep this header free of Windows SDK includes to avoid symbol conflicts
// (e.g., CloseWindow from windows.h vs raylib CloseWindow).

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