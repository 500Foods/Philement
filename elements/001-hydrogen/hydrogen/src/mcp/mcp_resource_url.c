/*
 * mcp_resource_url.c
 * Phase 8b: hosted MCP resource URL reachability check.
 *
 * The hosted MCP connector sends the mint's short-TTL token to
 * `server_url` (an external provider like xAI, which then calls
 * public MCP). If that URL is loopback or an internal IP, the
 * provider cannot reach it; a loopback smoke test would otherwise
 * look like a successful round-trip.
 *
 * This is a pure string/IP check; no DNS lookup. Operator's
 * responsibility to set a real external hostname in MCP.Resource.
 */
#include <src/hydrogen.h>
#include <src/mcp/mcp_resource_url.h>

#include <arpa/inet.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

bool mcp_mcp_resource_url_is_reachable(const char *url) {
    if (!url || !*url) return false;

    const char *p = strstr(url, "://");
    if (!p) return false;
    const char *host_start = p + 3;
    if (!*host_start) return false;

    const char *scheme_end = p;
    size_t scheme_len = (size_t)(scheme_end - url);
    bool is_https = (scheme_len == 5 &&
                     tolower((unsigned char)url[0]) == 'h' &&
                     tolower((unsigned char)url[1]) == 't' &&
                     tolower((unsigned char)url[2]) == 't' &&
                     tolower((unsigned char)url[3]) == 'p' &&
                     tolower((unsigned char)url[4]) == 's');
    if (!is_https) return false;

    const char *host_end;
    if (*host_start == '[') {
        host_start++;
        host_end = strchr(host_start, ']');
        if (!host_end) return false;
    } else {
        host_end = host_start;
        while (*host_end && *host_end != '/' && *host_end != ':' && *host_end != '?' && *host_end != '#') {
            host_end++;
        }
    }
    if (host_end == host_start) return false;

    char host[256];
    size_t host_len = (size_t)(host_end - host_start);
    if (host_len >= sizeof(host)) return false;
    memcpy(host, host_start, host_len);
    host[host_len] = '\0';

    if (host_len == 9 &&
        tolower((unsigned char)host[0]) == 'l' &&
        tolower((unsigned char)host[1]) == 'o' &&
        tolower((unsigned char)host[2]) == 'c' &&
        tolower((unsigned char)host[3]) == 'a' &&
        tolower((unsigned char)host[4]) == 'l' &&
        tolower((unsigned char)host[5]) == 'h' &&
        tolower((unsigned char)host[6]) == 'o' &&
        tolower((unsigned char)host[7]) == 's' &&
        tolower((unsigned char)host[8]) == 't') {
        return false;
    }

    struct in_addr v4;
    if (inet_pton(AF_INET, host, &v4) == 1) {
        uint32_t ip = ntohl(v4.s_addr);
        unsigned int o0 = (ip >> 24) & 0xffu;
        unsigned int o1 = (ip >> 16) & 0xffu;
        if (o0 == 127) return false;
        if (o0 == 10) return false;
        if (o0 == 172 && (o1 >= 16 && o1 <= 31)) return false;
        if (o0 == 192 && o1 == 168) return false;
        if (o0 == 169 && o1 == 254) return false;
        if (o0 == 0) return false;
        if (o0 == 100 && (o1 >= 64 && o1 <= 127)) return false;
        return true;
    }
    struct in6_addr v6;
    if (inet_pton(AF_INET6, host, &v6) == 1) {
        if (IN6_IS_ADDR_LOOPBACK(&v6)) return false;
        if (IN6_IS_ADDR_LINKLOCAL(&v6)) return false;
        if (IN6_IS_ADDR_SITELOCAL(&v6)) return false;
        return false;
    }
    return true;
}
