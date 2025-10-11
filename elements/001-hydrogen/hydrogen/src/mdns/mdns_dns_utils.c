/*
 * mDNS DNS Utility Functions
 *
 * Contains DNS parsing and packet construction utilities for mDNS server.
 * Split from mdns_server.c to improve modularity and testability.
 */

#include <src/hydrogen.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "mdns_keys.h"
#include "mdns_server.h"

/**
 * Parse DNS name from packet buffer
 * Handles compressed names and converts to dotted format
 */
uint8_t *read_dns_name(uint8_t *ptr, const uint8_t *packet, char *name, size_t name_len) {
    size_t i = 0;
    while (*ptr) {
        if ((*ptr & 0xC0) == 0xC0) {
            uint16_t offset = ((*ptr & 0x3F) << 8) | *(ptr + 1);
            read_dns_name((uint8_t*)packet + offset, packet, name + i, name_len - i);
            return ptr + 2;
        }
        size_t len = *ptr++;
        if (i + len + 1 >= name_len) return NULL;
        memcpy(name + i, ptr, len);
        i += len;
        name[i++] = '.';
        ptr += len;
    }
    if (i > 0) name[i - 1] = '\0';
    else name[0] = '\0';
    return ptr + 1;
}

/**
 * Write DNS name to packet buffer
 * Converts dotted name format to DNS wire format
 */
uint8_t *write_dns_name(uint8_t *ptr, const char *name) {
    // Defensive check for NULL name
    if (!name) {
        *ptr++ = 0;  // Just write null terminator for NULL name
        return ptr;
    }

    const char *part = name;
    while (*part) {
        const char *end = strchr(part, '.');
        if (!end) end = part + strlen(part);
        size_t len = (size_t)(end - part);
        *ptr++ = (uint8_t)len;
        memcpy(ptr, part, len);
        ptr += len;
        if (*end == '.') part = end + 1;
        else part = end;
    }
    *ptr++ = 0;
    return ptr;
}

/**
 * Write DNS resource record to packet buffer
 */
uint8_t *write_dns_record(uint8_t *ptr, const char *name, uint16_t type, uint16_t class, uint32_t ttl, const void *rdata, uint16_t rdlen) {
    ptr = write_dns_name(ptr, name);
    *((uint16_t*)ptr) = htons(type); ptr += 2;
    *((uint16_t*)ptr) = htons(class); ptr += 2;
    *((uint32_t*)ptr) = htonl(ttl); ptr += 4;
    *((uint16_t*)ptr) = htons(rdlen); ptr += 2;
    memcpy(ptr, rdata, rdlen);
    ptr += rdlen;
    return ptr;
}

/**
 * Write DNS PTR record to packet buffer
 */
uint8_t *write_dns_ptr_record(uint8_t *ptr, const char *name, const char *ptr_data, uint32_t ttl) {
    ptr = write_dns_name(ptr, name);
    *((uint16_t*)ptr) = htons(MDNS_TYPE_PTR); ptr += 2;
    *((uint16_t*)ptr) = htons(MDNS_CLASS_IN); ptr += 2;
    *((uint32_t*)ptr) = htonl(ttl); ptr += 4;
    uint16_t data_len = (uint16_t)(strlen(ptr_data) + 2);
    *((uint16_t*)ptr) = htons(data_len); ptr += 2;
    ptr = write_dns_name(ptr, ptr_data);
    return ptr;
}

/**
 * Write DNS SRV record to packet buffer
 */
uint8_t *write_dns_srv_record(uint8_t *ptr, const char *name, uint16_t priority, uint16_t weight, uint16_t port, const char *target, uint32_t ttl) {
    ptr = write_dns_name(ptr, name);
    *((uint16_t*)ptr) = htons(MDNS_TYPE_SRV); ptr += 2;
    *((uint16_t*)ptr) = htons(MDNS_CLASS_IN); ptr += 2;
    *((uint32_t*)ptr) = htonl(ttl); ptr += 4;
    uint16_t data_len = (uint16_t)(6 + strlen(target) + 2);
    *((uint16_t*)ptr) = htons(data_len); ptr += 2;
    *((uint16_t*)ptr) = htons(priority); ptr += 2;
    *((uint16_t*)ptr) = htons(weight); ptr += 2;
    *((uint16_t*)ptr) = htons(port); ptr += 2;
    ptr = write_dns_name(ptr, target);
    return ptr;
}

/**
 * Write DNS TXT record to packet buffer
 */
uint8_t *write_dns_txt_record(uint8_t *ptr, const char *name, char **txt_records, size_t num_txt_records, uint32_t ttl) {
    ptr = write_dns_name(ptr, name);
    *((uint16_t*)ptr) = htons(MDNS_TYPE_TXT); ptr += 2;
    *((uint16_t*)ptr) = htons(MDNS_CLASS_IN); ptr += 2;
    *((uint32_t*)ptr) = htonl(ttl); ptr += 4;
    size_t total_len = 0;
    for (size_t i = 0; i < num_txt_records; i++) {
        total_len += strlen(txt_records[i]) + 1;
    }
    *((uint16_t*)ptr) = htons((uint16_t)total_len); ptr += 2;
    for (size_t i = 0; i < num_txt_records; i++) {
        size_t len = strlen(txt_records[i]);
        *ptr++ = (uint8_t)len;
        memcpy(ptr, txt_records[i], len);
        ptr += len;
    }
    return ptr;
}