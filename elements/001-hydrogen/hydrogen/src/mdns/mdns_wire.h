/*
 * Shared DNS/mDNS wire codec (encode + decode only, no sockets).
 */

#ifndef MDNS_WIRE_H
#define MDNS_WIRE_H

#include <stddef.h>
#include <stdint.h>

#define DNS_CLASS_IN      0x0001u
#define DNS_CLASS_MASK    0x7fffu
#define DNS_CACHE_FLUSH   0x8000u
#define DNS_QU_BIT        0x8000u
#define DNS_FLAG_RESPONSE 0x8400u
#define DNS_FLAG_QUERY    0x0000u
#define RR_NSEC           47u
#ifndef MDNS_MSG_MAX
#define MDNS_MSG_MAX      9000u
#endif
#define MDNS_NAME_MAX     256u
#define MDNS_RR_MAX       64u

#define MDNS_SEC_ANSWER      0
#define MDNS_SEC_AUTHORITY   1
#define MDNS_SEC_ADDITIONAL  2

typedef struct {
    uint8_t *buf;
    size_t cap;
    size_t len;
    int overflow;
} mdns_buf;

typedef struct {
    char name[MDNS_NAME_MAX];
    uint16_t type;
    uint16_t cls;
    uint32_t ttl;
    uint16_t rdlen;
    size_t rdoff;
    int section;
} mdns_rr;

typedef struct {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
    mdns_rr questions[MDNS_RR_MAX];
    size_t nquestions;
    mdns_rr rr[MDNS_RR_MAX];
    size_t nrr;
} mdns_msg;

void mdns_buf_init(mdns_buf *b, uint8_t *storage, size_t cap);
int mdns_buf_room(mdns_buf *b, size_t n);
int mdns_put_u8(mdns_buf *b, uint8_t v);
int mdns_put_u16(mdns_buf *b, uint16_t v);
int mdns_put_u32(mdns_buf *b, uint32_t v);
int mdns_put_bytes(mdns_buf *b, const void *p, size_t n);
int mdns_put_name(mdns_buf *b, const char *name);
int mdns_rr_head(mdns_buf *b, const char *name, uint16_t type, uint32_t ttl,
                 int flush, size_t *rdlen_pos);
int mdns_rr_tail(mdns_buf *b, size_t rdlen_pos);
int mdns_put_rr_nsec(mdns_buf *b, const char *name, const uint16_t *types,
                     size_t ntypes, uint32_t ttl, int flush);
int mdns_name_decode(const uint8_t *msg, size_t msglen, size_t off, char *out,
                     size_t outcap, size_t *next);
int mdns_name_equal(const char *a, const char *b);
int mdns_parse(const uint8_t *msg, size_t msglen, mdns_msg *out);
int mdns_rdata_name(const uint8_t *msg, size_t msglen, const mdns_rr *rr,
                    char *out, size_t outcap);
int mdns_rdata_srv(const uint8_t *msg, size_t msglen, const mdns_rr *rr,
                   uint16_t *priority, uint16_t *weight, uint16_t *port,
                   char *target, size_t targetcap);
int mdns_txt_get(const uint8_t *rdata, size_t rdlen, const char *key,
                 char *out, size_t outcap);
void mdns_wire_keep_linked(void);

#endif
