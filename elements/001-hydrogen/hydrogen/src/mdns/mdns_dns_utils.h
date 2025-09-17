/*
 * mDNS DNS Utility Functions Header
 *
 * Function declarations for DNS parsing and packet construction utilities.
 */

#ifndef MDNS_DNS_UTILS_H
#define MDNS_DNS_UTILS_H

#include <stdint.h>
#include <stddef.h>

// DNS utility functions for parsing and constructing DNS packets
uint8_t *read_dns_name(uint8_t *ptr, const uint8_t *packet, char *name, size_t name_len);
uint8_t *write_dns_name(uint8_t *ptr, const char *name);
uint8_t *write_dns_record(uint8_t *ptr, const char *name, uint16_t type, uint16_t class, uint32_t ttl, const void *rdata, uint16_t rdlen);
uint8_t *write_dns_ptr_record(uint8_t *ptr, const char *name, const char *ptr_data, uint32_t ttl);
uint8_t *write_dns_srv_record(uint8_t *ptr, const char *name, uint16_t priority, uint16_t weight, uint16_t port, const char *target, uint32_t ttl);
uint8_t *write_dns_txt_record(uint8_t *ptr, const char *name, char **txt_records, size_t num_txt_records, uint32_t ttl);

#endif // MDNS_DNS_UTILS_H