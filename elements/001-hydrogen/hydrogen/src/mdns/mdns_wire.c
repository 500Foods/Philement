/*
 * Shared DNS/mDNS wire codec (encode + decode only, no sockets).
 */

#include <src/hydrogen.h>

#include <src/mdns/mdns_wire.h>

#include <strings.h>

void mdns_buf_init(mdns_buf *b, uint8_t *storage, size_t cap)
{
    if (!b) {
        return;
    }
    b->buf = storage;
    b->cap = cap;
    b->len = 0;
    b->overflow = 0;
    if (!storage && cap > 0) {
        b->overflow = 1;
        b->cap = 0;
    }
}

int mdns_buf_room(mdns_buf *b, size_t n)
{
    if (!b || b->overflow || !b->buf || b->len + n > b->cap) {
        if (b) {
            b->overflow = 1;
        }
        return 0;
    }
    return 1;
}

int mdns_put_u8(mdns_buf *b, uint8_t v)
{
    if (!mdns_buf_room(b, 1)) {
        return -1;
    }
    b->buf[b->len++] = v;
    return 0;
}

int mdns_put_u16(mdns_buf *b, uint16_t v)
{
    if (!mdns_buf_room(b, 2)) {
        return -1;
    }
    b->buf[b->len++] = (uint8_t)(v >> 8);
    b->buf[b->len++] = (uint8_t)(v & 0xffu);
    return 0;
}

int mdns_put_u32(mdns_buf *b, uint32_t v)
{
    if (!mdns_buf_room(b, 4)) {
        return -1;
    }
    b->buf[b->len++] = (uint8_t)(v >> 24);
    b->buf[b->len++] = (uint8_t)(v >> 16);
    b->buf[b->len++] = (uint8_t)(v >> 8);
    b->buf[b->len++] = (uint8_t)(v & 0xffu);
    return 0;
}

int mdns_put_bytes(mdns_buf *b, const void *p, size_t n)
{
    if (n == 0) {
        return 0;
    }
    if (!p || !mdns_buf_room(b, n)) {
        if (b) {
            b->overflow = 1;
        }
        return -1;
    }
    memcpy(b->buf + b->len, p, n);
    b->len += n;
    return 0;
}

int mdns_put_name(mdns_buf *b, const char *name)
{
    const char *p;

    if (!b || !name) {
        if (b) {
            b->overflow = 1;
        }
        return -1;
    }

    p = name;
    while (*p) {
        const char *dot = strchr(p, '.');
        size_t len = dot ? (size_t)(dot - p) : strlen(p);

        if (len == 0) {
            if (!dot) {
                break;
            }
            p = dot + 1;
            continue;
        }
        if (len > 63) {
            b->overflow = 1;
            return -1;
        }
        if (mdns_put_u8(b, (uint8_t)len) < 0) {
            return -1;
        }
        if (mdns_put_bytes(b, p, len) < 0) {
            return -1;
        }
        if (!dot) {
            break;
        }
        p = dot + 1;
    }
    return mdns_put_u8(b, 0);
}

int mdns_rr_head(mdns_buf *b, const char *name, uint16_t type, uint32_t ttl,
                 int flush, size_t *rdlen_pos)
{
    uint16_t cls = (uint16_t)(DNS_CLASS_IN | (flush ? DNS_CACHE_FLUSH : 0u));

    if (!rdlen_pos) {
        if (b) {
            b->overflow = 1;
        }
        return -1;
    }
    if (mdns_put_name(b, name) < 0) {
        return -1;
    }
    if (mdns_put_u16(b, type) < 0) {
        return -1;
    }
    if (mdns_put_u16(b, cls) < 0) {
        return -1;
    }
    if (mdns_put_u32(b, ttl) < 0) {
        return -1;
    }
    *rdlen_pos = b->len;
    return mdns_put_u16(b, 0);
}

int mdns_rr_tail(mdns_buf *b, size_t rdlen_pos)
{
    size_t rdlen;

    if (!b || b->overflow || !b->buf) {
        return -1;
    }
    if (rdlen_pos + 2 > b->len) {
        return -1;
    }
    rdlen = b->len - rdlen_pos - 2;
    if (rdlen > 0xffffu) {
        return -1;
    }
    b->buf[rdlen_pos] = (uint8_t)(rdlen >> 8);
    b->buf[rdlen_pos + 1] = (uint8_t)(rdlen & 0xffu);
    return 0;
}

int mdns_put_rr_nsec(mdns_buf *b, const char *name, const uint16_t *types,
                     size_t ntypes, uint32_t ttl, int flush)
{
    uint8_t bitmap[32];
    size_t pos;
    size_t hi = 0;

    memset(bitmap, 0, sizeof bitmap);
    if (types) {
        size_t i;

        for (i = 0; i < ntypes; i++) {
            uint16_t t = types[i];

            if (t >= 256) {
                continue;
            }
            bitmap[t / 8] |= (uint8_t)(0x80u >> (t % 8));
            if ((size_t)(t / 8) + 1 > hi) {
                hi = (size_t)(t / 8) + 1;
            }
        }
    }
    if (hi == 0) {
        hi = 1;
    }

    if (mdns_rr_head(b, name, (uint16_t)RR_NSEC, ttl, flush, &pos) < 0) {
        return -1;
    }
    if (mdns_put_name(b, name) < 0) {
        return -1;
    }
    if (mdns_put_u8(b, 0) < 0) {
        return -1;
    }
    if (mdns_put_u8(b, (uint8_t)hi) < 0) {
        return -1;
    }
    if (mdns_put_bytes(b, bitmap, hi) < 0) {
        return -1;
    }
    return mdns_rr_tail(b, pos);
}

int mdns_name_decode(const uint8_t *msg, size_t msglen, size_t off, char *out,
                     size_t outcap, size_t *next)
{
    size_t o = off;
    size_t outlen = 0;
    int jumps = 0;
    int followed = 0;

    if (!msg || !out || outcap == 0) {
        return -1;
    }
    out[0] = '\0';

    for (;;) {
        uint8_t len;

        if (o >= msglen) {
            return -1;
        }
        len = msg[o];

        if ((len & 0xc0u) == 0xc0u) {
            size_t ptr;

            if (o + 1 >= msglen) {
                return -1;
            }
            ptr = ((size_t)(len & 0x3fu) << 8) | (size_t)msg[o + 1];
            if (!followed) {
                if (next) {
                    *next = o + 2;
                }
                followed = 1;
            }
            if (++jumps > 64 || ptr >= msglen) {
                return -1;
            }
            o = ptr;
            continue;
        }
        if (len & 0xc0u) {
            return -1;
        }
        if (len == 0) {
            if (!followed && next) {
                *next = o + 1;
            }
            break;
        }
        if (o + 1 + (size_t)len > msglen) {
            return -1;
        }
        if (outlen + (size_t)len + 2 > outcap) {
            return -1;
        }
        if (outlen > 0) {
            out[outlen++] = '.';
        }
        memcpy(out + outlen, msg + o + 1, len);
        outlen += len;
        o += 1 + (size_t)len;
    }
    out[outlen] = '\0';
    return 0;
}

int mdns_name_equal(const char *a, const char *b)
{
    size_t la;
    size_t lb;

    if (!a || !b) {
        return 0;
    }
    la = strlen(a);
    lb = strlen(b);

    while (la > 0 && a[la - 1] == '.') {
        la--;
    }
    while (lb > 0 && b[lb - 1] == '.') {
        lb--;
    }
    if (la != lb) {
        return 0;
    }
    return strncasecmp(a, b, la) == 0;
}

int mdns_parse(const uint8_t *msg, size_t msglen, mdns_msg *out)
{
    size_t off;
    uint16_t counts[3];
    int section;
    uint16_t qi;

    if (!msg || !out || msglen < 12) {
        return -1;
    }

    memset(out, 0, sizeof(*out));
    out->id = (uint16_t)(((uint16_t)msg[0] << 8) | msg[1]);
    out->flags = (uint16_t)(((uint16_t)msg[2] << 8) | msg[3]);
    out->qdcount = (uint16_t)(((uint16_t)msg[4] << 8) | msg[5]);
    out->ancount = (uint16_t)(((uint16_t)msg[6] << 8) | msg[7]);
    out->nscount = (uint16_t)(((uint16_t)msg[8] << 8) | msg[9]);
    out->arcount = (uint16_t)(((uint16_t)msg[10] << 8) | msg[11]);

    off = 12;
    for (qi = 0; qi < out->qdcount; qi++) {
        size_t next;
        char name[MDNS_NAME_MAX];
        uint16_t typ;
        uint16_t cls;

        if (mdns_name_decode(msg, msglen, off, name, sizeof name, &next) < 0) {
            return -1;
        }
        off = next;
        if (off + 4 > msglen) {
            return -1;
        }
        typ = (uint16_t)(((uint16_t)msg[off] << 8) | msg[off + 1]);
        cls = (uint16_t)(((uint16_t)msg[off + 2] << 8) | msg[off + 3]);
        off += 4;
        if (out->nquestions < MDNS_RR_MAX) {
            mdns_rr *q = &out->questions[out->nquestions++];

            memcpy(q->name, name, MDNS_NAME_MAX);
            q->type = typ;
            q->cls = cls;
        }
    }

    counts[0] = out->ancount;
    counts[1] = out->nscount;
    counts[2] = out->arcount;
    for (section = MDNS_SEC_ANSWER; section <= MDNS_SEC_ADDITIONAL; section++) {
        uint16_t ri;

        for (ri = 0; ri < counts[section]; ri++) {
            size_t next;
            char name[MDNS_NAME_MAX];
            uint16_t typ;
            uint16_t cls;
            uint32_t ttl;
            uint16_t rdlen;

            if (mdns_name_decode(msg, msglen, off, name, sizeof name, &next) < 0) {
                return -1;
            }
            off = next;
            if (off + 10 > msglen) {
                return -1;
            }
            typ = (uint16_t)(((uint16_t)msg[off] << 8) | msg[off + 1]);
            cls = (uint16_t)(((uint16_t)msg[off + 2] << 8) | msg[off + 3]);
            ttl = ((uint32_t)msg[off + 4] << 24) | ((uint32_t)msg[off + 5] << 16)
                | ((uint32_t)msg[off + 6] << 8) | (uint32_t)msg[off + 7];
            rdlen = (uint16_t)(((uint16_t)msg[off + 8] << 8) | msg[off + 9]);
            off += 10;
            if (off + rdlen > msglen) {
                return -1;
            }
            if (out->nrr < MDNS_RR_MAX) {
                mdns_rr *rr = &out->rr[out->nrr++];

                memcpy(rr->name, name, MDNS_NAME_MAX);
                rr->type = typ;
                rr->cls = cls;
                rr->ttl = ttl;
                rr->rdlen = rdlen;
                rr->rdoff = off;
                rr->section = section;
            }
            off += rdlen;
        }
    }

    return 0;
}

int mdns_rdata_name(const uint8_t *msg, size_t msglen, const mdns_rr *rr,
                    char *out, size_t outcap)
{
    size_t next;

    if (!msg || !rr || !out) {
        return -1;
    }
    if (rr->rdoff > msglen || rr->rdoff + rr->rdlen > msglen) {
        return -1;
    }
    return mdns_name_decode(msg, msglen, rr->rdoff, out, outcap, &next);
}

int mdns_rdata_srv(const uint8_t *msg, size_t msglen, const mdns_rr *rr,
                   uint16_t *priority, uint16_t *weight, uint16_t *port,
                   char *target, size_t targetcap)
{
    size_t next;
    size_t off;

    if (!msg || !rr || !target) {
        return -1;
    }
    if (rr->rdlen < 6 || rr->rdoff + 6 > msglen || rr->rdoff + rr->rdlen > msglen) {
        return -1;
    }
    off = rr->rdoff;
    if (priority) {
        *priority = (uint16_t)(((uint16_t)msg[off] << 8) | msg[off + 1]);
    }
    if (weight) {
        *weight = (uint16_t)(((uint16_t)msg[off + 2] << 8) | msg[off + 3]);
    }
    if (port) {
        *port = (uint16_t)(((uint16_t)msg[off + 4] << 8) | msg[off + 5]);
    }
    return mdns_name_decode(msg, msglen, off + 6, target, targetcap, &next);
}

int mdns_txt_get(const uint8_t *rdata, size_t rdlen, const char *key,
                 char *out, size_t outcap)
{
    size_t off = 0;
    size_t keylen;

    if (!rdata || !key || !out || outcap == 0) {
        return -1;
    }
    keylen = strlen(key);
    out[0] = '\0';

    while (off < rdlen) {
        uint8_t len = rdata[off++];
        const char *chunk;
        int match;

        if (off + len > rdlen) {
            return -1;
        }
        chunk = (const char *)(rdata + off);
        match = 0;
        if (len == keylen) {
            match = (strncasecmp(chunk, key, keylen) == 0);
        } else if (len > keylen && chunk[keylen] == '=') {
            match = (strncasecmp(chunk, key, keylen) == 0);
        }
        if (match) {
            size_t voff = keylen;
            size_t vlen;

            if (voff < len && chunk[voff] == '=') {
                voff++;
            }
            vlen = (size_t)len - voff;
            if (vlen + 1 > outcap) {
                return -1;
            }
            {
                size_t i;

                for (i = 0; i < vlen; i++) {
                    out[i] = chunk[voff + i];
                }
            }
            out[vlen] = '\0';
            return 0;
        }
        off += len;
    }
    return -1;
}

void mdns_wire_keep_linked(void)
{
    uint8_t storage[64];
    mdns_buf b;
    uint16_t types[1];
    mdns_rr rr;
    char name[MDNS_NAME_MAX];
    char txt[8];
    uint16_t prio = 0;
    uint16_t weight = 0;
    uint16_t port = 0;

    types[0] = (uint16_t)RR_NSEC;
    mdns_buf_init(&b, storage, sizeof storage);
    (void)mdns_put_rr_nsec(&b, "h.local", types, 1, MDNS_TTL_HOST, 1);
    memset(&rr, 0, sizeof rr);
    (void)mdns_rdata_name(storage, b.len, &rr, name, sizeof name);
    (void)mdns_rdata_srv(storage, b.len, &rr, &prio, &weight, &port, name, sizeof name);
    (void)mdns_txt_get((const uint8_t *)"\x01x", 2, "x", txt, sizeof txt);
}
