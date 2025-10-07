#include "utils.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include <winsock2.h>

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

// --- Raylib compatibility shims ---
// Minimal implementations to satisfy symbols expected by raylib modules when building without raylib.lib

void TraceLog(int logType, const char* text, ...) {
    (void)logType;
    if (!text) return;
    va_list args; va_start(args, text);
    vprintf(text, args);
    printf("\n");
    fflush(stdout);
    va_end(args);
}

void* MemAlloc(unsigned int size) { return calloc(size, 1); }
void* MemRealloc(void* ptr, unsigned int size) { return realloc(ptr, size); }
void MemFree(void* ptr) { free(ptr); }

unsigned char* LoadFileData(const char* fileName, int* dataSize) {
    if (dataSize) *dataSize = 0;
    if (!fileName) return NULL;
    FILE* f = fopen(fileName, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long sz = ftell(f); if (sz < 0) { fclose(f); return NULL; }
    rewind(f);
    unsigned char* data = (unsigned char*)malloc((size_t)sz);
    if (!data) { fclose(f); return NULL; }
    size_t rd = fread(data, 1, (size_t)sz, f);
    fclose(f);
    if (dataSize) *dataSize = (int)rd;
    if (rd != (size_t)sz) {
        // Allow partial read; caller may handle
    }
    return data;
}

void UnloadFileData(unsigned char* data) { free(data); }

bool SaveFileData(const char* fileName, void* data, int dataSize) {
    if (!fileName || !data || dataSize < 0) return false;
    FILE* f = fopen(fileName, "wb");
    if (!f) return false;
    size_t wr = fwrite(data, 1, (size_t)dataSize, f);
    fclose(f);
    return (wr == (size_t)dataSize);
}

char* LoadFileText(const char* fileName) {
    if (!fileName) return NULL;
    FILE* f = fopen(fileName, "rt");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long sz = ftell(f); if (sz < 0) { fclose(f); return NULL; }
    rewind(f);
    char* text = (char*)malloc((size_t)sz + 1);
    if (!text) { fclose(f); return NULL; }
    size_t rd = fread(text, 1, (size_t)sz, f);
    text[rd] = '\0';
    fclose(f);
    return text;
}

void UnloadFileText(char* text) { free(text); }

bool SaveFileText(const char* fileName, const char* text) {
    if (!fileName || !text) return false;
    FILE* f = fopen(fileName, "wt");
    if (!f) return false;
    int r = fputs(text, f);
    fclose(f);
    return (r >= 0);
}