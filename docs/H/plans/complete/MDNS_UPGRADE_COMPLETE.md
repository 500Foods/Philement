<!-- markdownlint-disable MD007 MD024 -->
# mDNS Upgrade

## Purpose

Bring Hydrogen's mDNS **server** to RFC 6762 / 6763 feature parity
(probe/claim, selective answers, QU/legacy unicast, known-answer
suppression, NSEC, cache-flush, overflow-safe codec, name defense,
shared-record delay, probe tiebreak), replace the mDNS **client scaffold**
with browse/resolve + a queryable registry, and make Test 25 prove
discovery, goodbye, **duplicate-name rename**, and on-the-wire broadcasts.

This document is the **only** active mDNS plan. It absorbs the old TODO
client-runtime item, the launch_mdns_client archive note, and the useful
parts of
[`mdns_client_architecture.md`](/docs/H/core/reference/mdns_client_architecture.md)
(scanner / monitor / registry / TCP health). After it is in tree,
`elements/001-hydrogen/hydrogen/mdns-proj/` is disposable — algorithms
live in the appendix.

## How To Use This Document

- Work **one phase at a time**, top to bottom.
- **Do not start a phase until the previous phase Status is complete and
  its Exit gate is green.**
- Each phase has one **Done means** line — that is the testable state.
- Mark work items `[x]` only when that item's verification actually passed.
- Defer with `[~]` plus one-line rationale and the phase it moves to.
- After each phase: fill Status, append Working Log, **stop for review**.
- Build aliases: `zsh -ic 'mkq'` (iterative C), `zsh -ic 'mkt'` (full trial:
  new files / cmake / no build tree), `mku <base>`, `mkp`, `mka`, `mks`.
  See [INSTRUCTIONS.md](/docs/H/INSTRUCTIONS.md).

## Implementor Workflow (every phase)

Each phase is its **own conversation**:

1. Confirm the prior phase Status is complete; do not trust memory.
2. Discuss the current phase (Goal + Work + Done means + Exit gate).
   Grep/read code **before** writing anything.
3. Get explicit approval before editing source.
4. Ask questions rather than guessing.
5. Update Working Log at major milestones, not only at the end.
6. Record lessons learned.
7. Mark `[x]` / Status complete only after the named verification ran.
8. Never apply a database migration (none expected here).
9. Follow Unity one-file-per-function, no `static` in `src/`, `mkt`/`mkp`
   after C, `mks` after scripts. Do **not** create a new blackbox test
   number — extend [`test_25_mdns.sh`](/elements/001-hydrogen/hydrogen/tests/test_25_mdns.sh).

## Resuming Work

**CURRENT PAUSE POINT (as of 2026-08-31):** Phase 6a complete.
Next: **Phase 6b**.

### Resume here next session

1. Confirm the latest completed phase via Status blocks.
2. Re-read only the next phase Goal + Done means + Exit gate.
3. `zsh -ic 'mkq'` (or `mkt` if cmake/new files); relevant `mku`; Test 25 as
   named in that phase.
4. Implement that phase only → verify Exit gate → update this doc → stop.

---

## Priority

| | |
| --- | --- |
| **Band** | P2 — [TODO.md item 20](/docs/H/TODO.md) |
| **Effort** | L–XL |
| **Why** | Server announcements work (Test 25) but are not RFC-complete; client launch is a registry scaffold. Printers on a shared LAN will collide names and miss legacy/QU queriers. |

## Goals

1. Overflow-safe shared DNS/mDNS **codec** (write + parse) used by both
   server and client. No unbounded `write_dns_*` pointer walks.
2. Server **probes and claims** instance + hostname before announcing;
   renames on conflict (`Name (2)` …). No answers until claimed.
3. Server **answers the question**: selective records, additional A/AAAA,
   `_services._dns-sd._udp.local`, known-answer suppression, QU unicast +
   multicast, legacy unicast (non-5353 source).
4. Correct **TTLs** (shared vs host), **cache-flush** on unique records,
   **NSEC** when a family has no address, TTL-0 goodbye ×3 (already
   present — keep and align with new builder).
5. Real **client**: browse configured types, resolve SRV/TXT/A/AAAA,
   cache up to `max_services`, honor TTL-0 goodbye, IPv6 link-local
   scope from PKTINFO. Launch starts a worker; landing joins it.
6. **Registry** other subsystems can query (C snapshot +
   `/api/system/info` + Lua `H.mdns.list`). TCP connect health on the
   advertised port when `HealthCheck.Enabled`. `MonitoredServices`
   filters (own / printer types).
7. Test 25 asserts **log contract**, **duplicate-name claim** (two
   processes, same instance → `(2)`), and **wire contents** when tshark
   is present (PTR/SRV/TXT/A|AAAA, probe, TTL-0 goodbye).
8. After Phase 0, `mdns-proj/` is gone from the repo.

## Load balancer — decision

[`mdns_client_architecture.md`](/docs/H/core/reference/mdns_client_architecture.md)
draws a round-robin / health-weighted **job router** inside mDNS
(HTTP LBs, print-job distributors, failover). **Do not build that here.**

mDNS's job is discovery. Print-farm routing belongs in Print / Conduit
once a second consumer exists. This plan **does** ship the pieces that
doc actually needs from mDNS:

| Architecture box | In this plan |
| --- | --- |
| Scanner | Phase 6 browse/query |
| Monitor | cache + TTL refresh + goodbye |
| Registry | snapshot API, system/info, `H.mdns.list` (Phase 6a) |
| Health checker | TCP connect to advertised host:port (not HTTP) |
| Event dispatcher | log tokens + optional C callback `mdns_client_on_change` |
| Load balancer | **No.** Callers pick from the registry. JSON `MonitoredServices.LoadBalancers` is parsed and **ignored** until a later plan defines a service type; do not invent `_lb._tcp`. |

`AutoConnect` stays config-only (no HTTP client). HTTP health endpoints
are Non-Goal.

## Non-Goals

- OctoPrint / libmicrohttpd / gcode upload from the mini-stack.
- Replacing Hydrogen's **per-interface** sockets with one global fd.
- Emitting name compression (parse must accept it; write uncompressed).
- Dual-stack one-socket (`IPV6_V6ONLY` stays on).
- HTTP `auto_connect`, HTTP health URLs, round-robin/weighted job
  routing, Lithium UI.
- New blackbox test **number** (extend Test 25; a second Hydrogen
  process inside Test 25 is allowed).
- Avahi/Bonjour daemon replacement beyond coexistence on 5353
  (`SO_REUSEADDR`/`SO_REUSEPORT` already present).
- Cloud monitors, extra protocols, DNS-SD subtypes (`._sub._`),
  persistent registry across restart.

## Keep vs steal

### Keep (Hydrogen)

- Per-interface sockets + `SO_BINDTODEVICE` + failure backoff
  ([`mdns_server_socket.c`](/elements/001-hydrogen/hydrogen/src/mdns/mdns_server_socket.c),
  [`mdns_server_announce.c`](/elements/001-hydrogen/hydrogen/src/mdns/mdns_server_announce.c)).
- Multi-service config, device identity, secret key, launch/landing.
- Announce + responder threads, goodbye ×3 on shutdown.
- Unity layout and Test 25 harness (pcap/tshark cleanup, `set -e` traps).

### Steal (ideas + appendix snippets — not a file copy)

- `mdns_buf` overflow writer; name decode with jump limit; `mdns_name_equal`.
- Probe in **authority** section; claim flag; conflict rename.
- Want-bit responder; known-answer strip; QU + legacy unicast.
- NSEC type bitmap; shared vs host TTL; cache-flush on unique RRs.
- Client: PTR → instance → SRV/TXT → A/AAAA; TTL-0 goodbye; ifindex scope.
- `IPV6_V6ONLY`, `IP_PKTINFO` / `IPV6_RECVPKTINFO` on existing sockets.
- Client sends its **first** browse query immediately at startup instead
  of waiting a full `scan_interval` — send the first browse query
  before the wait loop so newly-launched clients do not sit silent for
  up to `scan_interval` seconds.
- Client treats **TXT as optional** for "found" readiness: once
  instance + SRV (host/port) + at least one resolved address are in
  hand, a short grace window (3s) is given for a still-late
  TXT before falling back to a default value, rather than blocking
  indefinitely on TXT. Applies to the registry snapshot's "ready" state
  in Phase 6a, not just the one-shot client `main.c` this came from.
- Address-cache dedup key is **(name, family, address bytes)**, not just
  address — two RRs for the same host/family/addr must collapse to one
  cache entry even if they arrive in separate packets.
- Endpoint ordering is a real total order, not just family grouping:
  rank (IPv4 < routable IPv6 < link-local IPv6) then a deterministic
  tie-break by address bytes — needed so Unity assertions on cache
  order are stable and so TCP health / any future connect-attempt code
  tries candidates in the same reproducible sequence.

---

## Current Hydrogen facts

Paths relative to `/elements/001-hydrogen/hydrogen/` unless noted.

| Piece | State |
| --- | --- |
| Server | Announce + query → **full** re-announce. `strcmp` names. No probe. |
| Wire | /elements/001-hydrogen/hydrogen/src/mdns/mdns_dns_utils.c) — no cap, recursive compression, no jump limit |
| Sockets | Per-iface v4/v6, REUSEADDR/REUSEPORT, BINDTODEVICE. No V6ONLY, no PKTINFO |
| TTL | `MDNS_TTL 255` used as **both** IP multicast hop count and DNS RR TTL ([`globals.h`](/elements/001-hydrogen/hydrogen/src/globals.h)) |
| Flags | `MDNS_FLAG_RESPONSE 0x8400` already includes AA; ORed again with `0x0400` |
| Client | [`launch_mdns_client.c`](/elements/001-hydrogen/hydrogen/src/launch/launch_mdns_client.c) — config + registry only. Comment: "no browse/query worker" |
| Config | Client: `scan_interval`, `max_services`, `retry_count`, `health_check_*`, `service_types[]` already parsed |
| Tests | Unity under `tests/unity/src/mdns/`; blackbox [`tests/test_25_mdns.sh`](/elements/001-hydrogen/hydrogen/tests/test_25_mdns.sh) (v3.0.3) |
| Docs | [`/docs/H/core/subsystems/mdnsserver/mdnsserver.md`](/docs/H/core/subsystems/mdnsserver/mdnsserver.md), [`/docs/H/tests/test_25_mdns.md`](/docs/H/tests/test_25_mdns.md) |

Client config already exists; do not invent parallel JSON. Wire the worker
to `app_config->mdns_client`.

## Config debt (fix while wiring the client)

Test 25 JSON and the loader disagree today — the client worker will be
blind unless this is fixed in Phase 6 / 6a:

| Field | JSON / schema | Loader today |
| --- | --- | --- |
| `ServiceTypes` | Test 25: **array of strings**. Unity: array of **objects** `{Type, Required, AutoConnect}` | Objects only; string array is skipped → **zero types** |
| `ScanIntervalMs` | milliseconds | `PROCESS_INT` into `scan_interval` documented as **seconds** |
| `HealthCheck.IntervalMs` | milliseconds | same seconds confusion |
| `HealthCheck.TimeoutMs` / `RetryCount` | in JSON | not stored on `MDNSClientConfig` |
| `MonitoredServices.*` | in Test 25 + schema | **not parsed** |
| `MaxServices` | schema | parsed |

Accept **both** ServiceTypes shapes (string or object). Store intervals
in milliseconds; convert at use. Parse TimeoutMs into the struct.
Parse `OwnServices` / `PrinterServices` / `CustomServices`; ignore
`LoadBalancers` with a one-line debug log.

## Target architecture

```text
src/mdns/
  mdns_wire.h/.c          encode/decode only (shared)
  mdns_dns_utils.*        thin wrappers OR deleted once callers use wire
  mdns_server_*.c         announce / probe / responder / socket / shutdown
  mdns_client.h/.c        browse worker + cache (new)
  mdns_client_*.c         split if files grow (query, cache, thread)

launch_mdns_client.c      start client thread
landing_mdns_client.c     set shutdown, join thread
```

Server and client **share the codec**. They do **not** share a socket:
server keeps per-iface fds; client may use the same `create_multicast_socket`
helper (preferred) or its own fds with the same options. Same host must
hear itself (`IP_MULTICAST_LOOP` already on) so Test 25 can use one process.

### Log contract (Test 25 pass criteria)

Use `log_this` with these **stable substrings** (exact tokens, then details):

| Token | When |
| --- | --- |
| `MDNS_SERVER PROBE` | each probe query sent (`<instance>`) |
| `MDNS_SERVER CONFLICT` | name taken; will rename |
| `MDNS_SERVER CLAIMED` | name ours; announcements may start |
| `MDNS_SERVER GOODBYE` | TTL-0 burst (already logged; keep token) |
| `MDNS_CLIENT QUERY` | browse/resolve query sent (`<type or instance>`) |
| `MDNS_CLIENT FOUND` | new instance cached (`<instance>`) |
| `MDNS_CLIENT SRV` | port + target (`<instance> <host> <port>`) |
| `MDNS_CLIENT TXT` | at least one key (`<instance>` and `path=` if present) |
| `MDNS_CLIENT ADDR` | A or AAAA (`<host> <addr>`) |
| `MDNS_CLIENT GOODBYE` | TTL-0 for a cached instance |
| `MDNS_CLIENT HEALTH` | TCP check result (`ok`/`fail` `<instance> <addr>:<port>`) |
| `MDNS_CLIENT DROP` | evicted (TTL expiry or max_services) |

Test 25 greps these tokens in the **Hydrogen server log**, not tshark.
Keep pcap/avahi as diagnostic/optional subtests.

## RFC / constant table (lock in Phase 0, code in Phase 1–2)

| Name | Value | Use |
| --- | --- | --- |
| `MDNS_PORT` | 5353 | bind/send |
| `MDNS_GROUP_V4` / `V6` | `224.0.0.251` / `ff02::fb` | unchanged |
| `MDNS_MULTICAST_TTL` | 255 | **IP** hop / `IPV6_MULTICAST_HOPS` only |
| `MDNS_TTL_SHARED` | 4500 | PTR, TXT (RFC 6763 s10) |
| `MDNS_TTL_HOST` | 120 | SRV, A, AAAA, NSEC |
| `MDNS_MSG_MAX` | 9000 | recv cap (send still prefer ≤1500) |
| `MDNS_NAME_MAX` | 256 | decoded name |
| `MDNS_RR_MAX` | 64 | parsed questions/RRs |
| `DNS_CLASS_IN` | 0x0001 | |
| `DNS_CLASS_MASK` | 0x7fff | strip cache-flush / QU |
| `DNS_CACHE_FLUSH` / `DNS_QU_BIT` | 0x8000 | same bit, different section |
| `DNS_FLAG_RESPONSE` | 0x8400 | QR+AA; do not OR AA twice |
| `DNS_FLAG_QUERY` | 0x0000 | |
| `RR_NSEC` | 47 | |
| `PROBE_TRIES` | 3 | RFC 6762 s8.1 |
| `PROBE_GAP_MS` | 250 | |
| `MAX_NAME_ATTEMPTS` | 8 | then fail launch of that service |
| `GOODBYE_BURST` | 3 × 250ms | already implemented |

PTR is a **shared** record: **no** cache-flush. SRV/TXT/A/AAAA/NSEC are
**unique**: cache-flush set on multicast (clear for legacy unicast).

---

## Phase 0 — Lock + remove scratch tree

**Status:** complete

### Goal

Lock design decisions in this file; delete `mdns-proj/` so implementors
use only this plan.

### Work

- [x] Confirm keep vs steal and log contract with the user if anything
      in Goals/Non-Goals is wrong; otherwise treat this document as locked.
- [x] Delete `elements/001-hydrogen/hydrogen/mdns-proj/` entirely
      (no leftover CMake/solve.sh). Already absent on disk (`test -d` fails).
- [x] Grep the repo for `mdns-proj`; remaining hits are this plan (Purpose
      historical sentence, Phase 0, Goals item 8) plus completed TODO note.
- [x] `mkl` / Test 04 if any doc links pointed at `mdns-proj`.

### Done means

`mdns-proj/` is gone; this plan is the spec.

### Exit gate

`test -d elements/001-hydrogen/hydrogen/mdns-proj` fails; Test 04 clean
if docs changed.

---

## Phase 1 — Shared wire codec

**Status:** complete

Codec constants live in [`mdns_wire.h`](/elements/001-hydrogen/hydrogen/src/mdns/mdns_wire.h)
(`DNS_*`, `RR_NSEC`, `MDNS_MSG_MAX` / `NAME_MAX` / `RR_MAX`, section ids).
`globals.h` TTL split stays Phase 2. Extra RRs/questions past `MDNS_RR_MAX`
are truncated (walked, not stored); a malformed RR fails the whole packet.
`mdns_wire_keep_linked` is a Phase 1 linker shim from
`mdns_server_process_query_packet` so encode/accessors are reachable from
`main()`; delete it in Phase 2 when announce uses the codec.

### Goal

Add `src/mdns/mdns_wire.h` and `mdns_wire.c` (encode/decode **only** —
no sockets). No `static` functions. Hydrogen server still uses the old
writers until Phase 2.

### Work

- [x] Port appendix **Writer**, **Name decode**, **Parse**, **name_equal**,
      **NSEC**, **rr_head/rr_tail** into Hydrogen style:
      `#include <src/hydrogen.h>` first; prototypes in the header;
      `log_this` only if a function must report (prefer silent `-1`).
- [x] `mdns_put_name`: uncompressed; skip empty labels; reject label > 63.
- [x] `mdns_name_decode`: compression pointers, `jumps > 64` → fail;
      no recursion (Hydrogen `read_dns_name` is recursive today).
- [x] `mdns_parse`: flatten answer+authority+additional into `mdns_rr[]`
      with `section` = ANSWER / AUTHORITY / ADDITIONAL (needed for
      known-answer vs probe).
- [x] Accessors: `mdns_rdata_name`, `mdns_rdata_srv`, `mdns_txt_get`.
- [x] Unity (one file per function, unique names), e.g.:
      - `mdns_wire_test_mdns_buf_overflow.c`
      - `mdns_wire_test_mdns_put_name.c`
      - `mdns_wire_test_mdns_name_decode.c`
      - `mdns_wire_test_mdns_name_equal.c`
      - `mdns_wire_test_mdns_parse.c`
      - `mdns_wire_test_mdns_put_rr_nsec.c`
      - `mdns_wire_test_mdns_rdata_srv.c`
      - `mdns_wire_test_mdns_txt_get.c`
- [x] Round-trip test: put PTR+SRV+TXT+A+AAAA, parse, `name_equal`.
- [x] Compression-bomb test: pointer loop → decode fails.
- [x] `mkt` && `mkp`; `mku` each new test.

### Done means

Codec compiles, is unused by production paths yet, Unity green, `mkt` dead
functions **zero** for the new symbols (call them from Unity only is OK
because Unity links objects — **production** `mkt` dead-code gate is the
trial binary; if the linker flags new files as unused, add a single
`mdns_wire` reference from `mdns_server.c` or keep files out of the
server lib until Phase 2). **Prefer:** compile into the server lib and
call `mdns_name_equal` from `mdns_server_process_query_packet` immediately
so dead-code stays clean.

### Exit gate

`zsh -ic 'mkt'` green; `zsh -ic 'mkp'` green; all listed `mku` green.

---

## Phase 2 — Server uses codec; TTL split; socket flags

**Status:** complete

Builder `_mdns_server_build_interface_announcement` keeps its signature.
`ttl == 0` → all RR TTLs 0 (goodbye); any other value uses `MDNS_TTL_SHARED`
(PTR/TXT) and `MDNS_TTL_HOST` (SRV/A/AAAA) with cache-flush on unique RRs
only. Recv buffer is `MDNS_MSG_MAX` (9000); send still caps at
`MDNS_MAX_PACKET_SIZE` (1500) and skips on overflow. `mdns_dns_utils.c`/`.h`
deleted. `mdns_wire_keep_linked` remains for `mdns_put_rr_nsec` /
`mdns_rdata_*` / `mdns_txt_get` until Phase 5/6 call them.

### Goal

Announcement and query parse go through `mdns_wire`. Split multicast TTL
vs RR TTL. Add `IPV6_V6ONLY` and PKTINFO on **existing** per-iface sockets.

### Work

- [x] Replace `write_dns_*` / `read_dns_name` call sites in
      `mdns_server_announce.c`, `mdns_server_threads.c`,
      `mdns_server_shutdown.c` with `mdns_buf` / `mdns_parse`.
- [x] Keep `_mdns_server_build_interface_announcement` signature if tests
      depend on it, but implement via wire (or retarget Unity to the new
      builder). Update Unity that poke raw packet bytes.
- [x] `globals.h`: add `MDNS_MULTICAST_TTL`, `MDNS_TTL_SHARED`,
      `MDNS_TTL_HOST`, `MDNS_MSG_MAX`; stop using `MDNS_TTL` for DNS RRs.
      Socket hop count uses `MDNS_MULTICAST_TTL`.
- [x] Cache-flush on unique records; PTR flush=0.
- [x] `create_multicast_socket`: `IPV6_V6ONLY` on AF_INET6; `IP_PKTINFO`
      / `IPV6_RECVPKTINFO` (non-fatal if refused). Recv path should use
      `recvmsg` when ifindex is needed (client Phase 5; server can keep
      `recvfrom` until then).
- [x] Do not overflow: if `mdns_buf.overflow`, skip send and log ALERT.
- [x] Retarget existing Unity (`mdns_dns_utils_*`, announce, process_query)
      so they still pass. Deprecate `mdns_dns_utils.c` if unused — do not
      leave dead `write_dns_*`.
- [x] `mkt` `mkp`; Test 25 still green (behavior change: TTLs in pcap).

### Done means

Server packets are built/parsed with the codec; Test 25 still finds
announcements; no dead `write_dns_*`.

### Exit gate

`zsh -ic 'mkt'`; `mkp`; existing mdns `mku` tests; `tests/test_25_mdns.sh`.

---

## Phase 3 — Selective responder (QU, known-answer, dns-sd, legacy)

**Status:** complete

New [`mdns_server_respond.c`](/elements/001-hydrogen/hydrogen/src/mdns/mdns_server_respond.c).
`mdns_server_process_query_packet` takes `sockfd` + `src_addr`/`src_len`;
`sockfd < 0` skips send (Unity). No `claimed` field yet (always answer).
`W_NSEC` may be set; NSEC RRs are not emitted (Phase 5). Per-service want
bits so a PTR for one type does not dump every service.

### Goal

Stop blasting a full announcement on every matching question. Answer
what was asked; suppress known answers; honor QU and legacy unicast.

### Work

- [x] Want-bit mask (`W_PTR|W_SRV|W_TXT|W_A|W_AAAA|W_NSEC|W_SD`) per
      appendix **handle_query**. Match names with `mdns_name_equal`.
- [x] PTR query for a service type → PTR + additional SRV/TXT/A/AAAA.
- [x] `_services._dns-sd._udp.local` PTR → PTR to each advertised type.
- [x] `strip_known_answers`: only `MDNS_SEC_ANSWER` in a **query**;
      drop a bit if querier's TTL > ours/2. Authority section is **not**
      known-answer (that is probing).
- [x] Source port ≠ 5353 → **legacy**: echo questions, copy ID, TTL cap 10,
      no flush, **unicast only**.
- [x] QU bit → unicast **and** multicast (appendix `send_records`).
- [x] Reply on the **same socket** the query arrived on (v4 vs v6).
- [x] Do not answer until `claimed` (Phase 4 adds the flag; until then
      treat claimed=1 so Test 25 stays green).
- [x] Unity: known-answer strip; QU vs QM; legacy port; dns-sd name;
      case-insensitive type match (`_HTTP._TCP.local`).
- [x] Test 25 still green (more selective packets; tshark may see fewer
      RRs per reply — do not require full dump).

### Done means

Responder is question-shaped; Unity covers strip/QU/legacy; Test 25 green.

### Exit gate

`mkt` `mkp`; new `mku`; `test_25_mdns.sh`.

---

## Phase 4 — Probe / claim / conflict rename

**Status:** complete

Per-name `claimed` on each service plus `hostname_claimed`. Hostname conflict
renames the label with a hyphen: `host.local` → `host-2.local` (attempt N).
Instance conflict uses DNS-SD `Name (N)`. After 8 attempts on any name the
whole server fails (`probe_failed`, all claimed flags cleared, no announce /
answer / goodbye). Probe helpers live in
[`mdns_server_probe.c`](/elements/001-hydrogen/hydrogen/src/mdns/mdns_server_probe.c);
the 3×250 ms loop is `mdns_server_run_probe` in the announce thread (no
random first delay). Test 25 greps `MDNS_SERVER CLAIMED` (5 names in the
default fixture). Two-process duplicate-name run remains Phase 7.

### Goal

No multicast announcement until the instance name and hostname survive
probing. Conflicts rename the **instance** (`Friendly (2)`). Hostname
conflict: append `-2` to the host label (document the rule in Status).

### Work

- [x] `claimed` flag per server (or per service if multiple instances).
      Responder returns immediately if `!claimed`.
- [x] `build_probe`: QR=0, questions ANY+QU for instance and hostname,
      proposed SRV/TXT/A/AAAA in **authority** (appendix **build_probe**).
- [x] `conflicts_with_probe`: QR=1 and a live (TTL≠0) RR under those
      names. Ignore questions (including our own looped probe).
- [x] 3 probes, 250ms apart; listen between. On conflict: log
      `MDNS_SERVER CONFLICT`, `next_name`, retry up to 8. Success: log
      `MDNS_SERVER CLAIMED`, then existing announce burst (3 × 1s then 60s).
- [x] Announce loop must not send until claimed.
- [x] Shutdown goodbye only if we claimed (never goodbye a name we
      did not own).
- [x] Unity: conflict detector; next_name formatting; probe packet
      has nscount > 0 and qdcount ≥ 2.
- [x] Test 25 (single process): grep `MDNS_SERVER CLAIMED`. Two-process
      duplicate-name run is **Phase 7** (required, not optional).

### Done means

Single Hydrogen greps CLAIMED. Unity covers conflict detector and
`next_name`. Two-process proof waits for Phase 7.

### Exit gate

`mkt` `mkp`; `mku` probe tests; `test_25_mdns.sh` includes CLAIMED.

---

## Phase 5 — NSEC + missing-family honesty

**Status:** complete

Positive A/AAAA answers also carry NSEC (bitmap = families that exist on
the iface + NSEC). Missing family drops `W_A` / `W_AAAA` and sets
`W_NSEC`. `mdns_server_want_empty` counts NSEC so an NSEC-only reply is
not skipped. `mdns_put_rr_nsec` is reached from
`mdns_server_put_host_nsec`; `mdns_wire_keep_linked` still holds
`mdns_rdata_*` / `mdns_txt_get` until Phase 6. Test 25 tshark A/AAAA
counts are diagnostic; one family is enough.

### Goal

If the interface has no IPv4 (or no IPv6), do not stay silent on A/AAAA
queries — send NSEC listing types that **do** exist (appendix NSEC).

### Work

- [x] When `want & W_A` and no v4 addrs → drop W_A, set W_NSEC.
- [x] Same for AAAA / v6.
- [x] NSEC next-domain = hostname; bitmap window 0; types present + NSEC.
- [x] Unity: IPv4-only host answering AAAA query yields NSEC, not AAAA.
- [x] Test 25: if dual-stack, ADDR log may be v4 and/or v6; do not require
      both families.

### Done means

NSEC emitted on missing family; Unity proves it; Test 25 green.

### Exit gate

`mkt` `mkp`; `mku`; `test_25_mdns.sh`.

---

## Phase 6 — Real mDNS client

**Status:** complete

Landing order: readiness list is Print → MailRelay → **mDNS Server** → **mDNS Client**
(comments in `landing.c`: 13 Server, 12 Client). Launch stays Server then Client
(12/13). `scan_interval` / `health_check_interval` are milliseconds at use.
String-array `ServiceTypes` parsed in this phase; remaining config debt is 6a.
Sockets reuse `create_multicast_socket`. `mdns_wire_keep_linked` still holds
rdata accessors until 6a/6b if unused from the client path.

### Goal

`launch_mdns_client_subsystem` starts a browse thread. Cache discovered
services. Landing sets shutdown and joins. Logs match the contract.

### Work

- [x] New `src/mdns/mdns_client.c` (split files if > ~400 lines).
       No `static` functions.
- [x] Open sockets via existing per-iface helper (respect
       `enable_ipv4` / `enable_ipv6`).
- [x] On `scan_interval`: `send_query` for configured `service_types`
       (PTR). After FOUND, query SRV/TXT then A/AAAA (appendix
       **send_query** / **handle_response**).
- [x] Accept **unsolicited** announcements (QR=1) so order vs server
       start does not matter.
- [x] Cache: instance, host, port, TXT map, endpoints (v4 first, then
       routable v6, then link-local with `scope=ifindex`, tie-broken by
       address bytes for a stable order). Dedup endpoints by
       (name, family, address). Cap `max_services`.
- [x] Send the first browse query immediately on thread start; do not
       wait a full `scan_interval` before the first packet goes out.
- [x] TXT is optional for `MDNS_CLIENT FOUND`/ready state: do not block
       on TXT once instance + SRV + ≥1 address are known; allow it to
       arrive late (bounded grace, e.g. a few seconds) and default any
       TXT-derived field (e.g. `path`) if it never shows up.
- [x] TTL-0 → drop cache entry, log `MDNS_CLIENT GOODBYE`.
- [x] Health check: if enabled, re-query when age > `health_check_interval`
       (no HTTP).
- [x] Ignore questions (including our own).
- [x] `launch_mdns_client.c`: start thread, register real shutdown.
       `landing_mdns_client.c`: flag + join (today it only sets a flag).
- [x] Unity: handle_response PTR/SRV/TXT/A; goodbye; name_equal trailing
       dot; duplicate ADDR ignored.
- [x] Do **not** implement `auto_connect`.

### Done means

One Hydrogen with server+client enabled logs QUERY, FOUND, SRV, TXT, ADDR
for its own advertisement (loopback). Shutdown logs GOODBYE on client
when server goodbye is heard — the client thread must still be alive
(socket open, not yet joined) at the moment the server's goodbye burst
goes out over multicast loopback.

**Landing order today (confirmed in code):** [`landing.c`](/elements/001-hydrogen/hydrogen/src/landing/landing.c)
lists `13. mDNS Client` **before** `12. mDNS Server`, i.e. the client
lands (thread joined, sockets closed) **before** the server sends its
goodbye. Unchanged, this misses the client's own loopback goodbye every
time.

**Landing order decision (implement this — do not leave it open):**
Swap the two landing entries so **mDNS Server lands before mDNS Client**
(server priority becomes the later one to land). Concretely: renumber so
the server's landing step (goodbye burst + socket close) runs first,
then the client's landing step (which by then has seen the loopback
goodbye and logged `MDNS_CLIENT GOODBYE`) runs and joins its thread.
Update the ordering comments in
[`launch.c`](/elements/001-hydrogen/hydrogen/src/launch/launch.c) and
`landing.c` to match, and re-verify no other subsystem depends on the
old client-before-server landing order (grep both files for `mDNS` before
renumbering). Record the final numbers in Status once changed.

### Exit gate

`mkt` `mkp`; client `mku`; Test 25 not yet rewritten but must not regress
(client log greps already look for `mDNSClient` — they will still pass).

---

## Phase 6a — Registry, filters, TCP health, info, Lua

**Status:** complete

Config: `TimeoutMs` / `HealthCheck.RetryCount`, `MonitoredServices` (Own /
Printer / Custom; LoadBalancers logged once). Snapshot APIs + TCP health +
`mdns` object on `/api/system/info` + `H.mdns.list`. Test 25 info curl left
to Phase 7. Unity listen-socket TCP is `TEST_IGNORE` under `USE_MOCK_SYSTEM`.

### Goal

The contemplated "service registry" is a real in-memory table other
code can read. Health is TCP to the advertised port. No job router.

### Work

- [x] Fix config debt (ServiceTypes string-or-object, ms intervals,
      TimeoutMs, MonitoredServices). Unity for both JSON shapes.
- [x] `mdns_client_snapshot` / `mdns_client_count` / lookup-by-type
      (thread-safe copy; caller frees). Cap `max_services`; log
      `MDNS_CLIENT DROP` on eviction.
- [x] Filters: `OwnServices=false` skips instances we claimed;
      `PrinterServices` limits extra types to
      `_http._tcp`, `_octoprint._tcp`, `_hydrogen._tcp`, `_ipp._tcp`,
      `_printer._tcp` (plus configured ServiceTypes always);
      `CustomServices` appended to browse list;
      `LoadBalancers` ignored (debug log once).
- [x] TCP health: if `HealthCheck.Enabled`, connect to each cached
      endpoint with `TimeoutMs`, `RetryCount`. Log `MDNS_CLIENT HEALTH`.
      Mark cache entry unhealthy; do not HTTP GET. Unity with a local
      listening socket + refused port.
- [x] `/api/system/info` JSON: claimed instance/hostname, client cache
      count, up to N instances (name, type, port, addrs, healthy).
- [x] Lua `H.mdns.list()` → table of the snapshot (read-only). No
      `H.mdns.connect`. Unity or Test 43 only if scripting is already
      on in Test 25 — prefer Unity + a Test 25 log line that info was
      dumped, plus a curl of `/api/system/info` in Test 25.
- [x] Optional `mdns_client_on_change` function-pointer for in-process
      listeners (found/lost/health). Default NULL. Unity that it fires.

### Done means

system/info shows the test instance; TCP health logs ok against the
local web port; `OwnServices` filter has a Unity case.

### Exit gate

`mkt` `mkp`; new `mku`; Test 25 still green (may add info curl this
phase or Phase 7).

---

## Phase 6b — Defend, delay, tiebreak, split packets

**Status:** not started

### Goal

Remaining RFC 6762 behaviors that stop LAN storms and dual-boot
collisions.

### Work

- [ ] **Defend unique names (s9):** after claimed, a conflicting
      announcement (QR=1, TTL≠0, our unique name, different rdata) →
      immediate multicast of our unique records (no delay). Log
      `MDNS_SERVER DEFEND`. Unity with a crafted conflicting SRV.
- [ ] **Shared-record delay (s6):** PTR / dns-sd answers wait random
      20–120 ms (deterministic in Unity via injectable RNG). Unique
      records (SRV/A/AAAA) may answer immediately.
- [ ] **Unique-record rate limit:** at most one multicast of a given
      unique name per second (RFC 6762 s6).
- [ ] **Simultaneous probe tiebreak (s8.2):** if a probe (authority
      section) arrives while we are probing the same name, compare
      records lexicographically; loser delays 1s and probes again.
      Unity with two synthetic probes.
- [ ] **Overflow split:** if `mdns_buf.overflow` building a response,
      emit what fitted (or a second packet) — never send a truncated
      buffer, never silently drop the whole answer. Prefer additional
      section first to cut. Unity: many A records → two packets or
      overflow handled.
- [ ] **Interface change:** if network already notifies, re-announce
      (and re-probe hostname if addrs changed). If no hook exists,
      log a Phase 8 doc note and skip — do not invent a netlink
      monitor in this plan.

### Done means

Unity for defend, delay bounds, tiebreak loser, overflow split.
Test 25 still green (delays may need slightly longer wait).

### Exit gate

`mkt` `mkp`; listed `mku`; `test_25_mdns.sh`.

---

## Phase 7 — Test 25 + Unity coverage for the new paths

**Status:** not started

### Goal

Test 25 fails closed on discovery, goodbye, duplicate names, and (when
tshark exists) on-the-wire records. Unity covers new functions.

### Work

#### Log contract (always gating)

- [ ] Wait for `MDNS_SERVER CLAIMED`, `MDNS_CLIENT QUERY`,
      `MDNS_CLIENT FOUND` (configured instance), `MDNS_CLIENT SRV`
      (correct port), `MDNS_CLIENT TXT` (`path=` if configured),
      `MDNS_CLIENT ADDR`.
- [ ] curl `/api/system/info` and `jq` the mDNS cache/claimed fields
      (no grep of JSON).
- [ ] Shutdown; wait for `MDNS_SERVER GOODBYE` and `MDNS_CLIENT GOODBYE`
      per Phase 6 landing order.
- [ ] All new greps use `grep -c ... || true` / `set -e` safety
      (Test 25 3.0.3 lesson).

#### Duplicate names (always gating — two processes)

- [ ] Second config: **same** instance/FriendlyName/service Name as the
      first, different web/API ports, `OwnServices` true.
- [ ] Start Hydrogen A, wait CLAIMED; start Hydrogen B with the second
      config; B must log `MDNS_SERVER CONFLICT` then
      `MDNS_SERVER CLAIMED` with `(2)` (or the documented hostname
      suffix).
- [ ] A's client (or B's) FOUNDs **both** instance names, or B's log
      proves the renamed instance was announced.
- [ ] Wire (if tshark): two different instance labels in PTR rdata.
- [ ] Stop B then A; no hang on 5353 (EXIT trap already kills leftover
      hydrogen/tshark/nc).

#### Broadcast / packet capture (gating if tshark exists; skip-not-fail if not)

- [ ] After CLAIMED, pcap must contain: PTR per advertised type, SRV
      with the configured port, TXT with a known key, at least one A or
      AAAA, QR=1 AA=1.
- [ ] Before CLAIMED: probe (ANY question, authority section or log
      `MDNS_SERVER PROBE` plus a query packet).
- [ ] After shutdown: RR with TTL=0 (goodbye).
- [ ] Flush tshark before read (3.0.2 lesson). Do not require avahi.
      avahi-browse remains diagnostic only.

#### Unity matrix (add any missing files; unique names)

- [ ] Codec: overflow, put_name, decode loop bomb, name_equal dots/case,
      parse sections, NSEC bitmap, txt_get, rdata_srv.
- [ ] Responder: want bits, known-answer strip, QU vs QM, legacy port,
      dns-sd, case-insensitive type.
- [ ] Probe: packet shape, conflict vs goodbye, next_name, claimed gate,
      tiebreak loser.
- [ ] Defend + rate limit + delay injection.
- [ ] Client: FOUND/SRV/TXT/ADDR order, goodbye, duplicate ADDR, filter
      OwnServices, TCP health ok/fail, snapshot free, max_services drop.
- [ ] Config: ServiceTypes string array vs object array; ms intervals.
- [ ] `add_coverage.sh` on `src/mdns/`; no new `static` functions.

- [ ] Bump `TEST_VERSION` + CHANGELOG.
- [ ] Update [`/docs/H/tests/test_25_mdns.md`](/docs/H/tests/test_25_mdns.md).
- [ ] `mks`; run Test 25 **3 consecutive** passes.

### Done means

Test 25 fails if FOUND/SRV missing, if the second process does not
rename, or (with tshark) if PTR/SRV/goodbye are absent. Passes without
avahi. Unity for new symbols green.

### Exit gate

`zsh -ic 'mks'`; `tests/test_25_mdns.sh` ×3; `mkt`/`mkp`; listed `mku`.

---

## Phase 8 — Docs + closeout

**Status:** not started

### Goal

Operator/subsystem docs match behavior. Plan complete.

### Work

- [ ] Update [`mdnsserver.md`](/docs/H/core/subsystems/mdnsserver/mdnsserver.md):
      probe, TTLs, dns-sd, NSEC, defend, delay, client worker, log tokens.
      Docs currently claim name compression on **write** — we do not emit it.
- [ ] Add `mdnsclient.md`: registry, filters, TCP health, Lua `H.mdns.list`,
      no job balancer, no auto_connect.
- [ ] Rewrite
      [`mdns_client_architecture.md`](/docs/H/core/reference/mdns_client_architecture.md)
      to match this plan (boxes that shipped vs Print-farm later).
- [ ] Fix [`mdns_configuration.md`](/docs/H/core/reference/mdns_configuration.md)
      intervals (ms vs seconds) and client section.
- [ ] Link from [`/docs/H/README.md`](/docs/H/README.md) / SITEMAP.
- [ ] `mkl` / Test 04.
- [ ] Move this plan to `plans/complete/MDNS_UPGRADE_COMPLETE.md`, drop
      TODO item 20, update [`plans/README.md`](/docs/H/plans/README.md).

### Done means

Docs + indexes match; TODO item gone.

### Exit gate

Test 04 / `mkl` green; TODO and plan index updated.

---

## Appendix A — Writer (overflow-safe)

Port as non-static functions. Names may keep `mdns_` prefix.

```c
typedef struct {
    uint8_t *buf;
    size_t cap;
    size_t len;
    int overflow;
} mdns_buf;

void mdns_buf_init(mdns_buf *b, uint8_t *storage, size_t cap)
{
    b->buf = storage;
    b->cap = cap;
    b->len = 0;
    b->overflow = 0;
}

int mdns_buf_room(mdns_buf *b, size_t n)
{
    if (b->overflow || b->len + n > b->cap) {
        b->overflow = 1;
        return 0;
    }
    return 1;
}
```

`mdns_put_u8/u16/u32/bytes` return `-1` on overflow. **Do not emit name
compression.** Skip empty labels (trailing/doubled dots). Label length > 63
→ overflow fail.

```c
int mdns_put_name(mdns_buf *b, const char *name)
{
    const char *p = name;

    while (*p) {
        const char *dot = strchr(p, '.');
        size_t len = dot ? (size_t)(dot - p) : strlen(p);

        if (len == 0) {
            if (!dot)
                break;
            p = dot + 1;
            continue;
        }
        if (len > 63) {
            b->overflow = 1;
            return -1;
        }
        if (mdns_put_u8(b, (uint8_t)len) < 0)
            return -1;
        if (mdns_put_bytes(b, p, len) < 0)
            return -1;
        if (!dot)
            break;
        p = dot + 1;
    }
    return mdns_put_u8(b, 0);
}
```

Cache-flush on unique records:

```c
int mdns_rr_head(mdns_buf *b, const char *name, uint16_t type, uint32_t ttl,
                 int flush, size_t *rdlen_pos)
{
    uint16_t cls = (uint16_t)(DNS_CLASS_IN | (flush ? DNS_CACHE_FLUSH : 0u));

    if (mdns_put_name(b, name) < 0)
        return -1;
    if (mdns_put_u16(b, type) < 0)
        return -1;
    if (mdns_put_u16(b, cls) < 0)
        return -1;
    if (mdns_put_u32(b, ttl) < 0)
        return -1;
    *rdlen_pos = b->len;
    return mdns_put_u16(b, 0);
}

int mdns_rr_tail(mdns_buf *b, size_t rdlen_pos)
{
    size_t rdlen;

    if (b->overflow)
        return -1;
    rdlen = b->len - rdlen_pos - 2;
    if (rdlen > 0xffffu)
        return -1;
    b->buf[rdlen_pos] = (uint8_t)(rdlen >> 8);
    b->buf[rdlen_pos + 1] = (uint8_t)(rdlen & 0xffu);
    return 0;
}
```

TTL 0 = goodbye for any of these RRs.

---

## Appendix B — Name decode (no recursion, jump limit)

Hydrogen `read_dns_name` is recursive and has no loop cap. Replace:

```c
int mdns_name_decode(const uint8_t *msg, size_t msglen, size_t off, char *out,
                     size_t outcap, size_t *next)
{
    size_t o = off, outlen = 0;
    int jumps = 0, followed = 0;

    if (outcap == 0)
        return -1;
    out[0] = '\0';

    for (;;) {
        uint8_t len;

        if (o >= msglen)
            return -1;
        len = msg[o];

        if ((len & 0xc0u) == 0xc0u) {
            size_t ptr;

            if (o + 1 >= msglen)
                return -1;
            ptr = ((size_t)(len & 0x3fu) << 8) | (size_t)msg[o + 1];
            if (!followed) {
                if (next)
                    *next = o + 2;
                followed = 1;
            }
            if (++jumps > 64 || ptr >= msglen)
                return -1;
            o = ptr;
            continue;
        }
        if (len & 0xc0u)
            return -1;
        if (len == 0) {
            if (!followed && next)
                *next = o + 1;
            break;
        }
        if (o + 1 + len > msglen)
            return -1;
        if (outlen + len + 2 > outcap)
            return -1;
        if (outlen > 0)
            out[outlen++] = '.';
        memcpy(out + outlen, msg + o + 1, len);
        outlen += len;
        o += 1 + (size_t)len;
    }
    out[outlen] = '\0';
    return 0;
}
```

Case-insensitive compare, one trailing dot ignored:

```c
int mdns_name_equal(const char *a, const char *b)
{
    size_t la = strlen(a), lb = strlen(b);

    while (la > 0 && a[la - 1] == '.')
        la--;
    while (lb > 0 && b[lb - 1] == '.')
        lb--;
    if (la != lb)
        return 0;
    return strncasecmp(a, b, la) == 0;
}
```

---

## Appendix C — NSEC (RFC 6762 s6.1)

"This name exists, but not with the type you asked for." Window 0 only
(types we use are < 256).

```c
int mdns_put_rr_nsec(mdns_buf *b, const char *name, const uint16_t *types,
                     size_t ntypes, uint32_t ttl, int flush)
{
    uint8_t bitmap[32];
    size_t pos, i, hi = 0;

    memset(bitmap, 0, sizeof bitmap);
    for (i = 0; i < ntypes; i++) {
        uint16_t t = types[i];

        if (t >= 256)
            continue;
        bitmap[t / 8] |= (uint8_t)(0x80u >> (t % 8));
        if ((size_t)(t / 8) + 1 > hi)
            hi = (size_t)(t / 8) + 1;
    }
    if (hi == 0)
        hi = 1;

    if (mdns_rr_head(b, name, RR_NSEC, ttl, flush, &pos) < 0)
        return -1;
    if (mdns_put_name(b, name) < 0)
        return -1;
    if (mdns_put_u8(b, 0) < 0)
        return -1;
    if (mdns_put_u8(b, (uint8_t)hi) < 0)
        return -1;
    if (mdns_put_bytes(b, bitmap, hi) < 0)
        return -1;
    return mdns_rr_tail(b, pos);
}
```

When `want & W_A` and `v4n == 0`: `want &= ~W_A; want |= W_NSEC`. Same for
AAAA. Bitmap types = families that **exist**, plus NSEC.

---

## Appendix D — Probe / claim

Probe is a **query** (flags 0) with ANY+QU questions for instance and
hostname; proposed unique records in **authority** (nscount patched after
write). Do not answer any question until `claimed`.

Conflict: incoming message has QR set and a RR with TTL ≠ 0 whose name
equals instance or hostname. TTL 0 is a goodbye, not a holder.

Rename instance (DNS-SD convention; space is a normal label byte):

```c
snprintf(instance, sizeof instance, "%s (%u)", instance_base, attempt);
```

Loop: up to `MAX_NAME_ATTEMPTS`; inner 3 probes, 250ms listen each.
Multicast loopback will return our own probe — that is a question, not a
conflict. After silence: `claimed = 1`, then announce.

Do **not** answer during probe (including our own probe via loopback).

---

## Appendix E — Query handling (server)

```text
if (!claimed) return;
legacy = (src_port != 5353);

for each question:
  cls = q.cls & DNS_CLASS_MASK
  if cls not IN/ANY: skip
  if q.cls & QU: qu = 1
  name == service type && (PTR|ANY) → want PTR|SRV|TXT|A|AAAA|NSEC
  name == instance && (SRV|ANY) → SRV|A|AAAA|NSEC
  name == instance && (TXT|ANY) → TXT
  name == hostname && (A|ANY) → A
  name == hostname && (AAAA|ANY) → AAAA|NSEC
  name == hostname && NSEC → NSEC
  name == _services._dns-sd._udp.local && (PTR|ANY) → W_SD

want = strip_known_answers(want, msg)  // ANSWER section only, TTL > ours/2
if want == 0: return

if legacy: unicast only, echo questions, id=query id, ttl_cap=10, no_flush
else: multicast always; also unicast if qu
```

Known-answer: a PTR in the query's answer section only suppresses **our**
PTR if rdata names **our** instance. Someone else's PTR is irrelevant.

Reply on the fd the datagram arrived on.

---

## Appendix F — Client browse/resolve

Do not require starting after the server. Handle QR=1 packets always.

Query construction:

```text
if instance empty: PTR service_type
else:
  if !have_srv: SRV instance
  if !have_txt: TXT instance
  if host set && no endpoints: A host, AAAA host
patch qdcount; send on every open family
```

Response handling order:

1. TTL 0 → goodbye (PTR to our instance, or SRV/TXT on instance).
2. PTR of configured type → FOUND instance (first or cache insert).
3. SRV/TXT on instance → port, host, path= from TXT (leading `/` optional).
4. A/AAAA → remember by **hostname**; collect endpoints once host known.
5. Link-local AAAA: store `ifindex` from `recvmsg` PKTINFO as scope.

Endpoint order: IPv4, then routable IPv6, then link-local; within a rank,
sort by address bytes for a stable, testable order. Dedup by
(name, family, address) so re-announcements do not duplicate cache rows.

TXT is optional: reaching FOUND-ready (instance + SRV + ≥1 address) does
not wait on it. Give it a short grace window, then proceed without it
and use a default for any field it would have supplied.

Ignore QR=0 (our queries looped back).

Health: re-send queries on `health_check_interval`; drop stale if TTL
expired (optional: use RR TTL from last packet).

---

## Appendix G — Socket extras (apply to Hydrogen helper, not global fds)

On each v6 socket:

```c
on = 1;
setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof on);
```

Non-fatal:

```c
setsockopt(fd, IPPROTO_IP, IP_PKTINFO, &on, sizeof on);
setsockopt(fd, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on, sizeof on);
```

If `recvmsg` does not fill `sin6_scope_id` for link-local, copy ifindex
from PKTINFO into `sin6_scope_id`.

Keep `SO_BINDTODEVICE` + per-iface join. Do **not** switch to a single
host-wide socket. Skip loopback and down/tunnel stubs when enumerating
addresses (Hydrogen network subsystem already filters; do not advertise
`127.0.0.1`).

---

## Appendix H — Parse flattening

`mdns_parse` reads qdcount questions then three RR sections into one
`rr[]` with `section` 0/1/2. Cap `MDNS_RR_MAX`. Truncate extra RRs rather
than failing the whole packet if possible (document choice in Status).

`cls` stored raw so callers can see QU / cache-flush.

---

## Appendix I — Defend, delay, tiebreak

**Defend (s9):** After `claimed`, parse QR=1 packets. If a unique RR
(SRV/TXT/A/AAAA) uses our instance or hostname, TTL ≠ 0, and rdata is
**not** identical to ours, multicast our unique records immediately
(no shared-record delay). Ignore our own loopback (identical rdata).

**Shared delay (s6):** If the answer contains only shared records (PTR,
dns-sd), sleep 20–120 ms (Unity injects a fixed value). If it contains
any unique record, no delay.

**Tiebreak (s8.2):** While probing, an incoming query with authority RRs
for the same name is a simultaneous probe. Compare each RR as network
byte-order class, type, then rdata. Later/greater wins; we lose → wait
1s, increment attempt, probe again. Do not set `claimed`.

**TCP health:** non-blocking connect + poll(`TimeoutMs`); `RetryCount`
failures → unhealthy. Success on any cached endpoint → healthy. Never
send HTTP.

---

## Working Log

| Date | Phase | Note |
| --- | --- | --- |
| 2026-08-31 | — | Plan written from Hydrogen vs mini-stack comparison. Implementation not started. |
| 2026-08-31 | — | Expanded: no in-mDNS load balancer; registry/TCP health/Lua/info; config debt; Phase 6a/6b; Test 25 two-process duplicate names + tshark RR gating. |
| 2026-08-31 | — | Re-read scratch-tree source in full against this plan; nothing missing from Keep/Steal or the appendices except client resilience details (immediate first query, TXT-optional grace, endpoint dedup key). Added those to Phase 6/Appendix F. Resolved the Phase 6 landing-order paragraph, which hedged between three options, into one concrete decision (server lands before client). |
| 2026-08-31 | 0 | Plan locked (keep vs steal + log contract). Scratch tree already gone. TODO remaining starts at Phase 1. |
| 2026-08-31 | 1 | `mdns_wire.c`/`.h`; process_query uses parse + `mdns_name_equal`; `mdns_wire_keep_linked` for encode reachability. `mkt`/`mkp`/listed `mku` green. Zero new dead `mdns_*` symbols. |
| 2026-08-31 | 2 | Announce/goodbye via `mdns_buf`; RFC TTL split + cache-flush; V6ONLY/PKTINFO; recv 9000; deleted `mdns_dns_utils`. `mkt`/`mkp`/mdns `mku`/Test 25 green. |
| 2026-08-31 | 3 | Selective responder in `mdns_server_respond.c`; QU+legacy dest; known-answer strip; dns-sd. `mkt`/`mkp`/new `mku`/Test 25 green. |
| 2026-08-31 | 4 | Probe/claim in `mdns_server_probe.c`; per-name claimed; hostname `label-N.local`; fail-whole after 8. `mkt`/`mkp`/probe `mku`/Test 25 CLAIMED green. |
| 2026-08-31 | 5 | Missing-family NSEC; NSEC also on positive A/AAAA. `mkt`/`mkp`/NSEC `mku`/Test 25 3.0.5 green. |
| 2026-08-31 | 6 | Browse worker + cache; string ServiceTypes; land Server then Client; scan_interval as ms. `mkt`/`mkp`/client `mku`/Test 25 3.0.5 green. |
| 2026-08-31 | 6a | Registry snapshot/count/lookup; TCP health; MonitoredServices; info JSON `mdns`; `H.mdns.list`; on_change. `mkt`/`mkp`/new `mku` green. Test 25 log tests pass; tshark Hydrogen-name wait timed out on busy LAN (Phase 7). |

## Lessons learned

- Test 25 historically aborted on `set -e` + `grep -c` (fixed 3.0.3). New
  greps must use `|| true` / `grep -c ... || true`.
- Client landing-before-server would miss goodbye; Phase 6 must pick an
  order before Test 25 asserts `MDNS_CLIENT GOODBYE`.
- `mkt` dead-code gate: codec must be referenced from `src/` in the same
  phase it is added, or Unity-only objects never enter the trial binary —
  still add a production call (`mdns_name_equal` in the responder) in
  Phase 1.
- `name_equal` alone is not enough: `--gc-sections` reports every unused
  encode/accessor function. Phase 1 used `mdns_wire_keep_linked` from
  `process_query`; remove that shim when announce writes via the codec.
- Iterative C after the tree is configured: `mkq` (`make-trial.sh QUICK`).
  `mkt` only for new `src/` files, cmake changes, or a missing `build/`.
- Deleting `mdns_dns_utils` requires `mkt` (configure-time glob). Process-query
  Unity fixtures must use `mdns_put_name`, not the removed `write_dns_name`.
- Announce encode does not reach `mdns_put_rr_nsec` / rdata accessors; keep a
  reduced `mdns_wire_keep_linked` until Phase 5/6 or `--gc-sections` flags them.
- Phase 3 `process_query` extra args: `sockfd < 0` or NULL src means parse/want
  only (no send). New `src/mdns/*.c` still needs `mkt` (configure glob).
- Do not chase 75% Unity on `mdns_server_announce.c` / `mdns_server_respond.c`
  until Phase 7 (`add_coverage.sh` on `src/mdns/`). Both files still change in
  Phases 4–6b.
- New fields on `mdns_server_service_t` go at the **end**; positional Unity
  inits (`{name, type, port, …}`) break `-Wmissing-field-initializers` if a
  field is inserted in the middle.
- Keep `mdns_server_run_probe` in `mdns_server_threads.c` so `USE_MOCK_THREADS`
  maps `mdns_server_system_shutdown` and Unity announce-loop tests skip the
  750 ms wait. `probe.c` objects are compiled without that mock.
- `mdns_server_want_empty` must count `W_NSEC`. Masking it made explicit NSEC
  questions and missing-family-only replies look empty.
- Phase 6 string-array ServiceTypes must be parsed or Test 25 client types
  stay empty. Nested `mDNSClient.ServiceTypes` lookup is required in addition
  to the dotted-key Unity fixture.
- Landing Server-before-Client is a special case vs reverse-of-launch;
  launch order stays Server then Client.
- Unity `USE_MOCK_SYSTEM` mocks `socket`/`connect`; a real listen+connect TCP
  health test cannot pass there. Keep refused/null Unity and live TCP for
  Test 25 / Phase 7.
