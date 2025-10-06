#include "net.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

int net_init(void) {
    WSADATA wsa;
    int r = WSAStartup(MAKEWORD(2,2), &wsa);
    return r == 0;
}

void net_cleanup(void) {
    WSACleanup();
}

int net_ping_ipv4(const char* ip) {
    HMODULE hIcmpMod = LoadLibraryA("Icmp.dll");
    if (!hIcmpMod) return 0;
    typedef HANDLE (WINAPI *IcmpCreateFile_t)(void);
    typedef DWORD  (WINAPI *IcmpSendEcho_t)(HANDLE, IPAddr, LPVOID, WORD, PIP_OPTION_INFORMATION, LPVOID, DWORD, DWORD);
    typedef BOOL   (WINAPI *IcmpCloseHandle_t)(HANDLE);
    IcmpCreateFile_t pIcmpCreateFile = (IcmpCreateFile_t)GetProcAddress(hIcmpMod, "IcmpCreateFile");
    IcmpSendEcho_t   pIcmpSendEcho   = (IcmpSendEcho_t)  GetProcAddress(hIcmpMod, "IcmpSendEcho");
    IcmpCloseHandle_t pIcmpCloseHandle = (IcmpCloseHandle_t)GetProcAddress(hIcmpMod, "IcmpCloseHandle");
    if (!pIcmpCreateFile || !pIcmpSendEcho || !pIcmpCloseHandle) { FreeLibrary(hIcmpMod); return 0; }

    HANDLE hIcmp = pIcmpCreateFile();
    if (hIcmp == INVALID_HANDLE_VALUE) { FreeLibrary(hIcmpMod); return 0; }

    char SendData[] = "ping";
    DWORD ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData);
    char* ReplyBuffer = (char*)malloc(ReplySize);
    if (!ReplyBuffer) { pIcmpCloseHandle(hIcmp); FreeLibrary(hIcmpMod); return 0; }

    IPAddr addr = 0;
    struct in_addr inaddr;
    inaddr.S_un.S_addr = inet_addr(ip);
    if (inaddr.S_un.S_addr == INADDR_NONE && strcmp(ip, "255.255.255.255") != 0) { free(ReplyBuffer); IcmpCloseHandle(hIcmp); return 0; }
    addr = inaddr.S_un.S_addr;

    DWORD dwRet = pIcmpSendEcho(hIcmp, addr, SendData, sizeof(SendData), NULL, ReplyBuffer, ReplySize, 1000);
    int ok = (dwRet != 0);
    free(ReplyBuffer);
    pIcmpCloseHandle(hIcmp);
    FreeLibrary(hIcmpMod);
    return ok;
}

int net_reverse_dns(const char* ip, char* hostname, size_t hostsz) {
    struct in_addr inaddr;
    inaddr.S_un.S_addr = inet_addr(ip);
    if (inaddr.S_un.S_addr == INADDR_NONE && strcmp(ip, "255.255.255.255") != 0) { hostname[0] = '\0'; return 0; }
    struct hostent* he = gethostbyaddr((const char*)&inaddr, sizeof(inaddr), AF_INET);
    if (he && he->h_name) { safe_strcpy(hostname, hostsz, he->h_name); return 1; }
    hostname[0] = '\0';
    return 0;
}

int net_get_mac(const char* ip, char* macbuf, size_t macsz) {
    IPAddr destIp = 0;
    struct in_addr inaddr;
    inaddr.S_un.S_addr = inet_addr(ip);
    if (inaddr.S_un.S_addr == INADDR_NONE && strcmp(ip, "255.255.255.255") != 0) return 0;
    destIp = inaddr.S_un.S_addr;

    DWORD macLen = 6;
    BYTE mac[6] = {0};
    DWORD r = SendARP(destIp, 0, mac, &macLen);
    if (r == NO_ERROR && macLen == 6) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%02X-%02X-%02X-%02X-%02X-%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        safe_strcpy(macbuf, macsz, buf);
        return 1;
    }
    return 0;
}

static int connect_with_timeout(const char* ip, int port, int timeout_ms) {
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return 0;

    u_long mode = 1; // non-blocking
    ioctlsocket(s, FIONBIO, &mode);

    struct sockaddr_in sa = {0};
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.S_un.S_addr = inet_addr(ip);

    int r = connect(s, (struct sockaddr*)&sa, sizeof(sa));
    if (r == 0) { closesocket(s); return 1; }

    if (WSAGetLastError() == WSAEWOULDBLOCK) {
        fd_set wfds; FD_ZERO(&wfds); FD_SET(s, &wfds);
        struct timeval tv; tv.tv_sec = timeout_ms / 1000; tv.tv_usec = (timeout_ms % 1000) * 1000;
        r = select(0, NULL, &wfds, NULL, &tv);
        if (r > 0 && FD_ISSET(s, &wfds)) {
            int err = 0; int len = sizeof(err);
            getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)&err, &len);
            closesocket(s);
            return err == 0;
        }
    }
    closesocket(s);
    return 0;
}

int net_scan_ports(const char* ip, const int* ports, int ports_count, int timeout_ms, int* open_ports, int* open_count) {
    int found = 0;
    for (int i = 0; i < ports_count; ++i) {
        if (connect_with_timeout(ip, ports[i], timeout_ms)) {
            if (open_ports && open_count) {
                open_ports[*open_count] = ports[i];
                (*open_count)++;
            }
            found++;
        }
    }
    return found;
}

int net_get_primary_subnet(SubnetV4* out) {
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
    ULONG size = 0;
    GetAdaptersAddresses(AF_UNSPEC, flags, NULL, NULL, &size);
    IP_ADAPTER_ADDRESSES* addrs = (IP_ADAPTER_ADDRESSES*)malloc(size);
    if (!addrs) return 0;
    if (GetAdaptersAddresses(AF_UNSPEC, flags, NULL, addrs, &size) != NO_ERROR) { free(addrs); return 0; }

    IP_ADAPTER_ADDRESSES* cur = addrs;
    while (cur) {
        if (cur->OperStatus == IfOperStatusUp) {
            IP_ADAPTER_UNICAST_ADDRESS* uni = cur->FirstUnicastAddress;
            while (uni) {
                if (uni->Address.lpSockaddr->sa_family == AF_INET) {
                    struct sockaddr_in* sin = (struct sockaddr_in*)uni->Address.lpSockaddr;
                    DWORD prefix = uni->OnLinkPrefixLength;
                    unsigned long ip = ntohl(sin->sin_addr.S_un.S_addr);
                    unsigned long mask = (prefix == 0) ? 0 : 0xFFFFFFFFUL << (32 - prefix);
                    unsigned long network = ip & mask;
                    unsigned long start_ip = network + 1;
                    unsigned long end_ip = (network | (~mask)) - 1;
                    out->network = network;
                    out->mask = mask;
                    out->start_ip = start_ip;
                    out->end_ip = end_ip;
                    free(addrs);
                    return 1;
                }
                uni = uni->Next;
            }
        }
        cur = cur->Next;
    }
    free(addrs);
    return 0;
}