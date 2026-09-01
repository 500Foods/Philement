# mDNS Server

This document describes the multicast DNS (mDNS) **server** implemented in the Hydrogen server. It advertises the printer's services on the local network per RFC 6762 and RFC 6763, and answers mDNS queries from other hosts.

## Overview

The mDNS Server makes Hydrogen reachable without manual IP configuration. It probes for a unique instance name and hostname, claims them on the network, then periodically announces and selectively answers queries for the advertised services.

The server is **RFC 6762/6763 feature-complete** for a single-host appliance:

- Probe / claim with conflict rename (`Name (2)` …, up to 8 attempts)
- Selective answers (answer only what was asked)
- Known-answer suppression
- QU unicast + multicast replies
- Legacy unicast (non-5353 source port)
- NSEC when a family has no address
- Cache-flush on unique records (SRV/TXT/A/AAAA/NSEC); none on shared PTR
- TTL split: shared records 4500 s, host records 120 s
- Defend unique names on conflict
- Shared-record delay (20–120 ms)
- Unique-record rate limit (1/s per name)
- Probe tiebreak (RFC 6762 s8.2)
- Overflow-safe codec (partial send, never truncated)
- Goodbye burst (TTL=0 ×3) on shutdown, only for claimed names

## Configuration

The mDNS Server is configured through the `mDNSServer` section of `hydrogen.json`:

```json
{
  "mDNSServer": {
    "Enabled": true,
    "EnableIPv6": true,
    "DeviceId": "hydrogen_01",
    "FriendlyName": "My Hydrogen Printer",
    "Model": "Voron 2.4",
    "Manufacturer": "Self-built",
    "Version": "1.0",
    "Services": [
      {
        "Name": "hydrogen-printer",
        "Type": "_http._tcp",
        "Port": 5000,
        "TxtRecords": [
          "version=1.0",
          "api=/api",
          "path=/",
          "type=printer"
        ]
      }
    ]
  }
}
```

For full configuration options, see the [mDNS Configuration Guide](/docs/H/core/reference/mdns_configuration.md).

## Probe and Claim

Before any announcement, the server **probes** for its hostname and each service instance name:

1. Send 3 probe queries (ANY+QU for the name, proposed records in authority), 250 ms apart.
2. Listen between probes. A response (QR=1) with a live (TTL≠0) RR under the same name is a **conflict**.
3. On conflict: rename and retry, up to `MAX_NAME_ATTEMPTS` (8) per name.
   - Instance conflict: append `(N)` to the label (DNS-SD convention): `hydrogen-printer (2)`.
   - Hostname conflict: append `-N` to the host label: `host-2.local`.
4. After silence on all names: mark `claimed = 1`, then begin announcements.
5. If 8 attempts fail on any name: the server fails to launch (`probe_failed`); no announce, answer, or goodbye.

The server does **not** answer any query until claimed.

## Selective Responder

The responder answers only the records the question asks for, identified by name + type:

| Question name | Type | Want bits |
| --- | --- | --- |
| service type (e.g. `_http._tcp`) | PTR / ANY | PTR + SRV + TXT + A + AAAA + NSEC |
| instance name | SRV / ANY | SRV + A + AAAA + NSEC |
| instance name | TXT / ANY | TXT |
| hostname | A / ANY | A |
| hostname | AAAA / ANY | AAAA + NSEC |
| hostname | NSEC | NSEC |
| `_services._dns-sd._udp.local` | PTR / ANY | W_SD (PTR to each advertised type) |

Additional processing:

- **Known-answer suppression**: a PTR in the query's answer section suppresses our PTR only if rdata names our instance and the querier's TTL is more than half ours.
- **QU bit**: reply on both multicast and unicast.
- **Legacy unicast** (source port ≠ 5353): echo questions, copy query ID, cap TTL at 10, no cache-flush, unicast only.
- **NSEC**: if the interface has no address for the asked family, drop A/AAAA and set NSEC listing types that do exist.
- **Reply on the same socket** the query arrived on (v4 vs v6).

## TTLs and Cache-Flush

Two TTL classes (RFC 6763 s10):

| Class | Records | TTL |
| --- | --- | --- |
| Shared | PTR, dns-sd PTR | 4500 s |
| Host (unique) | SRV, TXT, A, AAAA, NSEC | 120 s |

Unique records set the cache-flush bit on multicast answers. PTR (shared) does not. Legacy unicast answers never set cache-flush.

## Defend, Delay, Rate Limit, Tiebreak

- **Defend (RFC 6762 s9)**: after claiming, a conflicting announcement (QR=1, TTL≠0, our unique name, different rdata) triggers an immediate multicast of our unique records. Logs `MDNS_SERVER DEFEND`.
- **Shared-record delay (s6)**: PTR / dns-sd answers wait a random 20–120 ms; unique records answer immediately.
- **Unique-record rate limit (s6)**: at most one multicast of a given unique name per second.
- **Probe tiebreak (s8.2)**: if a probe arrives while we are probing the same name, compare records lexicographically; the loser delays 1 s and probes again.

## Wire Codec

Server and client share an overflow-safe codec in `src/mdns/mdns_wire.c`. Names are written **uncompressed**; the parser accepts compression pointers with a jump limit. On overflow, the builder emits what fitted and never sends a truncated buffer.

## Thread Management

The mDNS Server runs two threads:

1. **Announcement thread** (`mdns_server_announce_loop`): runs the probe/claim sequence, then broadcasts service information at increasing intervals.
2. **Responder thread** (`mdns_server_responder_loop`): listens for incoming mDNS queries and generates selective responses.

## Shutdown Sequence

On shutdown the server sends RFC-compliant goodbye packets (TTL=0) for all claimed services, on all interfaces, 3 times with 250 ms delay. Names that were never claimed get no goodbye. Threads are then joined and sockets closed.

## Log Contract

Stable log tokens Test 25 greps in the Hydrogen server log:

| Token | When |
| --- | --- |
| `MDNS_SERVER PROBE <instance>` | each probe query sent |
| `MDNS_SERVER CONFLICT <name>` | name taken; will rename |
| `MDNS_SERVER CLAIMED <instance>` | name ours; announcements may start |
| `MDNS_SERVER GOODBYE` | TTL-0 burst |
| `MDNS_SERVER DEFEND` | immediate re-announce on conflict |

## Debugging and Monitoring

1. Use `/api/system/info` to see claimed instance/hostname and client cache count.
2. View mDNS server logs with the `mDNSServer` subsystem tag.
3. External verification: `avahi-browse -a` (Linux), `dns-sd -B _http._tcp` (macOS).

## References

- [mDNS Configuration Guide](/docs/H/core/reference/mdns_configuration.md)
- [mDNS Client](/docs/H/core/subsystems/mdnsclient/mdnsclient.md)
- [RFC 6762](https://datatracker.ietf.org/doc/html/rfc6762) — Multicast DNS
- [RFC 6763](https://datatracker.ietf.org/doc/html/rfc6763) — DNS-Based Service Discovery
