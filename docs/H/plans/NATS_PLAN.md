<!-- markdownlint-disable MD007 MD024 -->
# NATS Subsystem Plan

## Phases

Each phase is self-contained with its own exit gate (V/Val/C) and is documented in detail in the [Phased Breakdown](#proposed-phased-breakdown) below. This table summarizes the individual phases:

| Phase | Focus | Status | Effort |
| --- | --- | --- | --- |
| 0 | Design approval — confirm config letter V, launch 22, message model, open questions | **Approved** 2026-10-05 | Easy |
| 1 | Config + launch + landing wiring | **Complete** 2026-10-05 | Medium |
| 2 | NATS connection + lifecycle (custom client) | **Complete** 2026-10-05 | Hard |
| 3 | Publish path (broadcast) | **Complete** 2026-10-05 | Medium |
| 4 | Subscribe + dispatch | **Complete** 2026-10-05 | Hard |
| 5 | Cache invalidation hooks | **In progress** 2026-10-05 | Hard |
| 6 | WebSocket relay | Not started | Hard |
| 7 | Instance presence & registry | Not started | Hard |
| 8 | Lua host API | Not started | Medium |
| 9 | Status + metrics | Not started | Easy |
| 10 | Unity unit tests | Not started | Hard |
| 11 | Blackbox Test 62 | Not started | Hard |
| 12 | Docs + indexes | Not started | Easy |

## Status

| Phase | Status | Last updated | Notes |
| --- | --- | --- | --- |
| 0 | **Approved** | 2026-10-05 | Locks signed off. Phase 1 has not started. No `src/` edits in the approval turn. |
| 1 | **Complete** | 2026-10-05 | `mkp` green, `mkt` green (4m 38s), five `mku` bases green. `test_17` was not in this run. |
| 2 | **Complete** | 2026-10-05 | `mkp` green (2,200 files). `mkt` green (4m 51s, 346 dead functions, no `nats_` symbol). Seven `mku` bases green. `test_17` was not in this run. |
| 3 | **Complete** | 2026-10-05 | `mkp` green (2,202 files). `mkt` green (2m 41s, 346 dead functions, no `nats_` symbol). `mku nats_publish_test_nats_broadcast` green (6). `test_17` was not in this run. |
| 4 | **Complete** | 2026-10-05 | `mkp` green (2,204 files). `mkt` green (2m 43s, 346 dead functions, no `nats_` symbol). `mku nats_dispatch_test_nats_dispatch_message` green (8). `test_17` was not in this run. |
| 5 | **In progress** | 2026-10-05 | Code is in the tree. The exit gate has not been run. |

## Purpose

Draft an implementation plan for a **NATS subsystem** in Hydrogen that serves as
a lightweight, cross-instance communication backchannel, consumed by Hydrogen
instances running in a Kubernetes cluster (DOKS) alongside a managed NATS server.

The original motivation is **cache invalidation without database triggers**.
Database triggers are notoriously difficult to build cross-engine (PostgreSQL,
MySQL/MariaDB, SQLite, DB2, Firebird, YugabyteDB, MSSQL) and painful to manage
during schema evolution. v1 does not hook the query executor. An explicit C
or Lua call names a database and a QueryRef. The caller deletes matching
result-cache entries, then publishes. Peers delete the same entries. The
next read misses and runs SQL. The same messages can also be forwarded to
connected WebSocket clients so a UI can show a live event. There is no
separate token table to refresh. OIDC reads through the same query path
as everyone else.

This document is a **phased plan**. Phase 0 was approved on 2026-10-05.
Phases 1, 2, 3, and 4 are complete. Phase 5 code is in the tree and its
exit gate has not been run. A 2026-10-05 review checked the locks below
against the tree and the DOKS NATS deployment; where an earlier paragraph
disagrees with [Verified constraints](#verified-constraints-2026-10-05),
the verified section wins.

## How To Use This Document

1. Review the design locks, the verified constraints, and the open questions.
2. Confirm the config letter, launch position, cache target, and client model.
3. Work **one phase per conversation**. Phases 0–4 are complete.
   Phase 5 code is in the tree and the exit gate has not been run.
   Do not start Phase 6 in the turn that closes Phase 5.
   Follow
   [`INSTRUCTIONS.md`](/docs/H/INSTRUCTIONS.md) and the gate template already
   in this file (the same shape as
   [`NOTIFICATIONS_PLAN.md`](/docs/H/plans/NOTIFICATIONS_PLAN.md)).

## Next session

Phase 5 code is in the tree (2026-10-05). The exit gate has not been
run. Read [Phase 5](#phase-5--result-cache-invalidation). Leave the
boxes open until these commands pass, in order:

```bash
zsh -ic 'mkp'
zsh -ic 'mkt'
zsh -ic 'mku query_result_cache_test_query_result_cache_invalidate_template'
zsh -ic 'mku nats_publish_test_nats_broadcast'
zsh -ic 'mku nats_dispatch_test_nats_dispatch_message'
```

The new Unity file is invisible until `mkt` reconfigures. Do not start
Phase 6 in the turn that runs the gate. Do not relay WebSocket, track
peers, or register Lua. `nats-server` is not on `PATH` on this
workstation (checked 2026-10-05). Phase 11 needs a real `nats-server`
on port 5620. The live DOKS broker is not the Phase 11 target.

## Implementation order

NATS is implemented **before** NOTIFICATIONS/SUBSCRIBERS. NATS holds config
letter **V**, launch **22**, blackbox Test **62** (ports 562x). NOTIFICATIONS
shifts to **W** / **23** / **63** (563x) — see
[NOTIFICATIONS_PLAN.md](/docs/H/plans/NOTIFICATIONS_PLAN.md) for its own plan.

## Current observed state

Do not re-implement these; they are constraints.

| Area | Status | Where |
| --- | --- | --- |
| Config letters | **A–U in use. V = NATS (held). W is the next free letter** for any future plan (e.g., NOTIFICATIONS/SUBSCRIBERS). | `src/config/config.h`, `src/hydrogen.h` `AppConfig` |
| Launch list | 21 subsystems (Registry … MCP). Chat is config-only. | `src/launch/launch_readiness.c` |
| `MAX_SUBSYSTEMS` | **24** / `INITIAL_REGISTRY_CAPACITY` **24**. Launch registers **21** today. NATS is the 22nd, Subscribers the planned 23rd. One slot remains. Do not bump 24. The readiness writer does not bounds-check the index. | `src/globals.h`, `src/launch/launch_readiness.c` |
| Source glob | `file(GLOB_RECURSE … "../src/*.c")` in `cmake/CMakeLists-init.cmake`. New `src/*.c` and new Unity files are picked up only after CMake reconfigures. `mkt` does that. `mkq` alone does not. | `cmake/CMakeLists-init.cmake` |
| Query templates (QTC) | Per-DQM catalog of SQL templates loaded by the bootstrap SELECT (`ref`, `type`, `query`, `name`, `queue`, `timeout`). `QueryCacheEntry.timeout_seconds` is the **execution budget** and is already applied. It is not a result TTL. `query_cache_clear` wipes the whole template catalog. Bootstrap does **not** load `collection`. | `src/database/database_cache.h`, `src/database/database_bootstrap.c` |
| Query **result** cache | The cross-instance problem. Global, indefinite, server-lifetime cache. Used only when the query's queue type is cache (Lookup 58 value 3). Key is `database_name` + SHA-256(SQL template) + SHA-256(normalized params). API today is get / put / clear-all. There is **no** per-ref or per-template delete. | `src/database/dbqueue/query_result_cache.h` |
| WebSocket broadcast | One vhost, name `"hydrogen"`, on `websocket.port`. Session data is per-connection libwebsockets user data (`WebSocketSessionData.authenticated` exists). Hydrogen does **not** keep a session list, and there is no `subscribed_events` field. `ws_write_json_response` writes one `wsi`. `lws_write` from a NATS thread is not safe. | `src/websocket/websocket_server_internal.h`, `websocket_server_message.c` |
| Network subsystem | Interface discovery, port scan, and ping. It does **not** expose a client socket, epoll loop, or TLS stack. NATS opens its own TCP sockets. | `src/network/network.h` |
| API service | `api_service.c` mounts REST handlers; `json_endpoints` for body parsing; `extract_and_validate_jwt` for auth. Mail Relay checks JWT inside handlers (not via `protected_endpoints` middleware). | `src/api/` |
| Status metrics | MCP is both a `ServiceMetrics mcp` member of `SystemMetrics` and a `specific.mcp` union arm. Terminal is also a member. Mail uses `QueueMetrics mail_relay_queue`, not a `ServiceMetrics` arm. NATS follows the MCP pair: member and union arm. | `src/status/status_core.h` |
| Landing dispatch | `landing_readiness.c` holds the shutdown table. First entry lands first. Comments in that file and in `landing.c` say "reverse launch order"; `land_approved_subsystems` walks the readiness results in table order and skips Registry. MCP is near the end of the table (after Threads), with Scripting and Reporting after MCP. | `src/landing/` |
| Env var substitution | `${env.NAME}` resolved in `src/config/config.c:load_config`. | `config.c` |
| Blackbox slots | **62 is free.** 63–69 are also free. 61 is inbound mail. NATS takes **62**. NOTIFICATIONS takes **63**. | `tests/test_*.sh` |
| Helium | Latest file is `acuranzo_1385.lua` (it rewrites QueryRef **#154**). Highest product QueryRef is **#155** (`acuranzo_1381.lua`). Next free migration **1386**, next free QueryRef **#156**. NATS v1 stores nothing. | `elements/002-helium/acuranzo/migrations/` |
| DOKS NATS | Image `nats:2.11.4`, three pods, client port **4222** plaintext, monitor **8222**, cluster routes **6222**. `nats.conf` has no authorization block, no TLS, and no JetStream. Service DNS `nats.nats.svc.cluster.local`. The NATS cluster name is `festival`. That name is server routing, not Hydrogen's subject prefix. | namespace `nats`, 2026-10-05 |
| `landing_plan.c` | `expected_order[]` is only the Go/No-Go log. It already omits MCP, Scripting, and Reporting. A name missing from it still lands. Do **not** rewrite the list. Add `SR_NATS` before `SR_PRINT` so the log has a Go line. Do not place it next to MCP; MCP is not in the array. | `src/landing/landing_plan.c` |
| `INSTRUCTIONS.md` | Lists A–U. V and launch 22 are reserved there until Phase 1 writes them into `config.h` and `launch_readiness.c`. Phase 12 turns that reservation into the live letter once the code exists, and adds the operator guide. Chat's missing `DUMP_CONFIG_SECTION("U")` is still a Chat bug, not a NATS gate. | [`INSTRUCTIONS.md`](/docs/H/INSTRUCTIONS.md) |
| `config.c` | `DUMP_CONFIG_SECTION("U", …)` for Chat is **missing** — dump sequence goes T (MCP) → `#undef`, skipping U. | `src/config/config.c` |

### Note on the NOTIFICATIONS_PLAN.md reservation

[`NOTIFICATIONS_PLAN.md`](/docs/H/plans/NOTIFICATIONS_PLAN.md) proposed
config letter **V** and launch position **22** for a `Subscribers`
subsystem (Web Push). That plan is **Phase 0 — not approved** and no code
exists for it.

**NATS takes priority.** NATS holds **V / 22 / 62 / `H_HK_NATS = 7`**.
Subscribers holds **W / 23 / 63 / `H_HK_SUBSCRIBERS = 8`**. Neither
number is in source yet. If Subscribers is ever implemented first, swap
the handle kind and amend both plans in the same change.

---

## Verified constraints (2026-10-05)

Checked against the tree and `kubectl` in namespace `nats`. These override
any older sentence in this file.

1. **Letters and slots.** `config.h` comments run A–U. V and W are free.
   `launch_readiness.c` has 21 `process_subsystem_readiness` calls, ending
   at MCP. `MAX_SUBSYSTEMS` is 24. NATS is the 22nd registration,
   Subscribers the 23rd. One slot remains. The readiness writer does not
   bounds-check the index, so a 25th registration is a buffer overrun,
   not a clean error.
2. **Readiness is a boolean.** Disabled matches MCP: `ready = true`, and
   `launch_nats_subsystem` returns 1 without connecting. A not-ready
   subsystem is skipped. It does not stop the rest of the server, and it
   also never starts the retry thread. Unreachable NATS must still be
   `ready = true`. The status field `degraded` is a metric, not a third
   readiness value. `ready = false` is only for invalid config (enabled,
   no servers, bad URL, bad delays, or `TlsEnabled: true`). v1 has no
   TLS client. Do not connect in plaintext when the operator asked for
   TLS.
3. **Sockets.** NATS opens its own TCP connection to port 4222. Do not
   route it through `src/network/`. Do not connect to 6222. TLS is out of
   v1. Live servers: `nats://nats.nats.svc.cluster.local:4222`, three
   replicas, image 2.11.4, no auth, no JetStream. `ClusterId` is the
   subject prefix (`philement` unless the operator changes it). It is not
   the NATS server cluster name (`festival`).
4. **Echo.** A connection receives its own messages unless `CONNECT`
   sets `no_echo: true`. A second connection does not suppress that.
   Use one connection and `no_echo: true`. Still compare `instance_id`
   before applying an invalidation, so a bug in the flag cannot evict
   twice or loop. Local evict happens in the publisher before the
   publish returns. Do not wait for the message to come back.
5. **What is cached.** Invalidate the global query **result** cache
   (`query_result_cache_*`), not the QTC template catalog.
   `timeout_seconds` on a template is the query deadline and already
   works. `queries.collection` is not loaded by bootstrap and is not an
   invalidation list. v1 has no `InvalidateQTC` column.
6. **Invalidation v1.** One subject, `cluster.<ClusterId>.cache.invalidate`.
   Body carries `database` (connection name) and `query_ref` (integer).
   The receiver looks up that ref's SQL template in the named database's
   QTC and drops every result-cache entry for that template (all
   parameter variants). That needs a new
   `query_result_cache_invalidate_template`. `query_cache_clear` and
   `query_result_cache_clear` wipe whole caches and must not be the
   per-ref path. The publisher evicts the same way locally, then
   publishes. Emission v1 is an explicit C or Lua call. A lost message leaves peers
   holding the old result until process restart: the result cache has
   no TTL, and template `timeout_seconds` does not expire it. A
   declarative list on the query row is a later Helium migration (next
   file 1386, next QueryRef 156), not something the executor can read
   today.
7. **Wildcards.** NATS tokens are `*` (one token) and `>` (the rest).
   `+` is not a wildcard.
8. **WebSocket relay.** Add a session list (connect / close) and a
   `subscribed_events` field on `WebSocketSessionData`. The NATS thread
   enqueues. The libwebsockets service thread calls `lws_write`. There
   is no direct `H.ws.broadcast`.
9. **Lua.** `ScriptingConfig` has no handler-file map. `H.mcp` and
   `H.mail` are C functions installed on the Lua state, not JSON paths.
   A `"NATS Handler"` script path is new work, not an existing pattern.
   `H.nats.broadcast_sync` waits until the frame is written to the
   socket. It does not wait for a peer ack. Core NATS here is
   at-most-once. `H.wait` must grow a branch in **both**
   `H_lua_wait_one` and the multi-handle loop in `H_lua_wait`.
10. **Presence v1.** `app_state` carries `instance_id`, `state`, and the
    current WebSocket `active_connections` count if that counter is easy
    to read. A 5-minute unique-IP tally across REST and WebSocket does
    not exist. Do not block presence on building one.
11. **Landing.** The landing table is shutdown order, first entry first.
    It is not the reverse of launch. Land NATS **before WebSocket**
    (with Print and Mail Relay), while libwebsockets and the database
    are still up. Landing only closes the NATS socket and joins NATS
    threads. It must not call into a subsystem that has already landed.
    Do not insert NATS after MCP. Comments in `landing_readiness.c` and
    `landing.c` say "reverse launch order". `land_approved_subsystems`
    walks the readiness results in table order and skips Registry.
    `landing_plan.c` `expected_order[]` is only the Go/No-Go log. It
    already omits MCP, Scripting, and Reporting. Add `SR_NATS` before
    `SR_PRINT` in that array. Do not rebuild the stale list, and do not
    look for MCP in it.
12. **Tests.** Hydrogen web ports and the NATS listen port must differ.
    Fixture `nats-server` on **5620**. Hydrogen instances on **5621**
    (disabled), **5622** (connected), **5623** (enabled, servers pointed
    at nothing on **5628**), **5624** (peer). Unity covers each phase.
    Phase 10 is the coverage fence, not the first time tests are written.
13. **Chat dump gap.** `DUMP_CONFIG_SECTION("U")` is missing and
    `initialize_config_defaults_chat` is not called from the master init
    (`memset` still zeroes the struct). That is pre-existing. It is not
    a NATS exit gate.
14. **Config stores suffixes.** Code builds every subject as
    `cluster.` + `ClusterId` + `.` + suffix. Do not store
    `cluster.philement…` in config and also prepend `ClusterId`. There
    is no `PublishSubjects.ClusterPrefix` field. `QueueGroup`, when set,
    is the full name sent in `SUB` and is not prefixed again. v1 suffixes
    are `cache.invalidate`, `instance.app_state`, and optional
    `jobs.<name>`.

---

## Goals

1. **New subsystem `NATS`** — peer of MCP / Mail Relay, not a Conduit query,
   not a Webhook, not Notify. A managed-NATS backchannel for cross-instance
   and instance-to-client communication.
2. **Config section V** listing which NATS subjects (channels) to subscribe to,
   what NATS servers/credentials to use, and WebSocket broadcast forwarding rules.
3. **Launch / landing** — dedicated readiness, launch, and landing. Disabled by
   default = clean skip so [`test_17`](/docs/H/tests/TESTING.md) min/max stay
   stable.
4. **Result-cache invalidation** — an explicit C or Lua call names a
   database and a QueryRef. The publisher deletes matching result-cache
   entries, then publishes. Peers delete the same template's entries.
   The next read misses and runs SQL. A lost message stays stale.
5. **WebSocket forwarding** — optionally forward a subset of broadcast messages
   to connected WebSocket clients so UIs get live updates.
6. **Channel coordination** — distinguish cluster-wide subjects from
   instance-group / instance-specific subjects so broadcasts go only where
   intended.

## Non-goals

- Linking an upstream NATS C library (`cnats` / `libnats`). The client is ours. See [NATS Client Implementation](#nats-client-implementation).
- Auth complexity. The live DOKS server has no authorization block. Support username/password or none. Do not implement NKEYS or operator JWT.
- Building the NATS server itself. The user states they already have NATS running
  in their DOKS cluster.
- Full schema migrations. NATS is a message bus; it is stateless from the
  database perspective. Any coordination state (e.g., "this instance claimed
  this cache shard") would be in-memory only, not persisted.
- Replacing the existing database query cache with a distributed cache. The
  query cache stays local; NATS is the invalidation signal.

---

### NATS Client Implementation

**Option B: Roll our own NATS client** — confirmed.

Rationale:

- No new dependencies
- Core NATS is a small TCP protocol and is enough for invalidation
- jansson is already available for CONNECT JSON and message bodies
- The client owns its sockets. `src/network/` is not a socket or TLS layer

Scope of the custom client (plaintext v1):

- Read the server `INFO` line. Honour `auth_required`, `tls_required`, and `max_payload`.
- Send `CONNECT` with `verbose:false`, `pedantic:true`, `protocol:1`, `lang`, `version`, and `no_echo:true`. Omit `user` / `pass` when the config credentials are empty. The live server has no auth and no TLS; a client that sends an empty user can still be rejected by a future server, so omit the fields.
- `PUB` / `SUB` / `UNSUB` / `MSG`. Queue-group subscribe is `SUB <subject> <queue-group> <sid>`.
- `MSG` payloads are raw bytes of the declared length, not a text line. One TCP read can contain several operations or a split body. Parse with a byte buffer.
- Answer server `PING` with `PONG` on the reader thread. The server drops a client that does not.
- Reconnect with the cascading delays already locked. After every reconnect, re-send every `SUB` (the server does not remember them), including queue groups.
- One connection per instance for publish and subscribe. `no_echo:true` stops that connection from receiving its own publishes. It does nothing for a second connection.

Out of scope for v1:

- NKEYS / operator JWT
- JetStream (the DOKS config does not enable it)
- TLS. `nats.conf` is plaintext on 4222. There is no shared outbound TLS helper to borrow. A later phase can add an OpenSSL client on the same socket if `INFO` reports `tls_required`. Do not connect to the cluster port **6222**
- A second "connection pool". One live connection plus the reconnect loop is the v1 model

Client files:

- `src/nats/nats_client.c` and `nats_client.h` — sockets, protocol, reconnect
- `src/config/config_nats.c` — JSON parsing only. Do **not** also add `src/nats/nats_config.c`

Public client operations (names can move, the split cannot):

- connect, publish, subscribe (subject + optional queue group), unsubscribe, disconnect
- a function-pointer seam so Unity never opens a real socket

Split the reader, writer, and frame parser across files before any one file reaches 1000 lines (Test 99). No `static` functions in `src/`.

Subscription state (sid, subject, queue group, callback) lives in `nats_internal.h`, as file-scope state or heap structures. Not a connection pool.

---

## Subsystem Placement

### Config letter: V

Add **V. NATS** to `config.h`'s comment block (after U. Chat), and to:

- `src/config/config_forward.h` — forward-declare `NATSConfig`
- `src/config/config_nats.h` — struct + load/dump/cleanup/apply_defaults
- `src/config/config_nats.c` — implementation
- `src/config/config_defaults.c` — `initialize_config_defaults_nats()`
- `src/config/config.c` — `LOAD_CONFIG("V", …)`, `DUMP_CONFIG_SECTION("V", …)`,
  `cleanup_nats_config(...)` in `clean_app_config`
- `src/hydrogen.h` — `NATSConfig nats;` in `AppConfig`

### Launch order: 22 (after MCP) — non-blocking, degraded-ok

NATS is a cross-cutting backchannel. It launches **after** the subsystems it
coordinates (Database cache, API, WebSocket, Scripting) are initialized, so that
invalidation hooks are ready when the first mutating request hits.

NATS does not use the Network subsystem's API. It still launches after
Database, API, and WebSocket so those objects exist before the first
publish. Launch position is **22**, immediately after MCP.

- **Disabled.** Same shape as MCP: `ready = true`, launch returns 1, no
  socket. Test 17 min keeps working when the section is absent.
- **Enabled but unreachable.** `ready = true`. Launch starts the retry
  thread and returns 1 even when the first connect fails. Status metric
  `state` is `degraded` until a connect succeeds, then `up`. Delays are
  30s, 60s, 120s, 240s, 480s, then 480s forever. A `ready = false` here
  would skip launch, and the retry thread would never run.
- **Enabled but invalid.** `ready = false`. The subsystem is not launched.
  The rest of the server still starts. Invalid means no servers, a bad
  URL, bad delays, or `TlsEnabled: true`. v1 has no TLS client, so a
  true flag must not fall through to a plaintext connect.
- **Landing.** Insert `{SR_NATS, check_nats_landing_readiness}` at the
  top of the landing table, before Print, so the bridge stops while
  WebSocket and Database are still up. Do not insert it after MCP.

| Must update | File | What |
| --- | --- | --- |
| `SR_NATS` constant | `src/globals.h` | `#define SR_NATS "NATS"` |
| `nats_system_shutdown` | `src/state/state.h` | `extern volatile sig_atomic_t` |
| `nats_threads` | `src/state/state.h` + `src/threads/threads.h` | `ServiceThreads` extern + definition in `state.c` |
| Readiness check | `src/launch/launch.h` | `check_nats_launch_readiness(void)` |
| Launch dispatch | `src/launch/launch_readiness.c` | Add `process_subsystem_readiness(..., SR_NATS, check_nats_launch_readiness())` after MCP |
| Launch function | `src/launch/launch.h` | `launch_nats_subsystem(void)` |
| Launch dispatch | `src/launch/launch.c` | Add `strcmp(subsystem, SR_NATS) == 0` branch in `launch_approved_subsystems()` |
| Landing readiness | `src/landing/landing.h` | `check_nats_landing_readiness(void)` |
| Landing readiness table | `src/landing/landing_readiness.c` | Insert `{SR_NATS, check_nats_landing_readiness}` **before** `{SR_PRINT, ...}`, not after MCP |
| Landing dispatch | `src/landing/landing.c` | Add `SR_NATS → land_nats_subsystem` in `get_landing_function()` |
| Landing function | `src/landing/landing.h` | `land_nats_subsystem(void)` |
| Status metrics | `src/status/status_core.h` | `ServiceMetrics nats;` in `SystemMetrics` + union arm |
| Startup log | `src/launch/launch.c` `startup_hydrogen()` | Log NATS status in the SERVICES section |
| Thread/queue totals | `src/launch/launch.c` `startup_hydrogen()` | Add `nats_threads` / `nats_queue_memory` to thread + queue counts |

> Reference: `launch_mcp.c` / `landing_mcp.c` / `config_mcp.c` / `config_mcp.h`
> are the most recent templates to mirror exactly.
>
> **Letter reservation:** V is held by NATS. NOTIFICATIONS/SUBSCRIBERS shifts to **W** (next free letter) when its plan is approved. NATS is implemented first.

### Source directory structure

```text
src/nats/
  nats.h                    public API: nats_init, nats_shutdown, nats_broadcast, nats_subscribe, status snapshot (state: healthy|degraded)
  nats_internal.h           one connection, subscription table, shutdown flag, peer registry
  nats.c                    init / shutdown / metrics snapshot / retry scheduler (state up|degraded)
  nats_client.c             plaintext client: INFO, CONNECT, PUB, SUB, MSG, PING/PONG, reconnect
  nats_client.h             public client API
  nats_publish.c            publish path (broadcast invalidation events)
  nats_subscribe.c          subscribe path (receive and dispatch invalidation events)
  nats_dispatch.c           local dispatch: invalidate caches, refresh queries, etc.
  nats_ws_bridge.c          forward selected NATS messages to WebSocket clients via ws_broadcast_json()
  nats_queue.c              internal queue for outbound publishes (bounded)
  nats_reconnect.c          cascading retry: 30s, 60s, 120s, 240s, 480s, then steady 480s
  nats_registry.c           in-memory peer registry (presence/heartbeat tracking)
  nats_subject.c            subject name validation + assembly helpers

src/api/nats/
  nats_service.h            //@ swagger:service NATS (if an admin API is needed)
  status/status.c/.h        GET /api/nats/status (JWT, cluster connectivity, subscriptions)
  status/peers.c/.h         GET /api/nats/instances (peer list from presence heartbeats)
```

That block is the end-state sketch from Phase 0. Files added in each
phase are listed in that phase's Accomplished section. Do not add a
file only to match the sketch. There is no `nats_init`, no
`nats_queue.c`, and no separate client header. The outbound ring lives
in `nats_client.c`. Link state strings are `down`, `degraded`, and
`up`. `nats_dispatch.c` parses an incoming envelope. It does not delete
result-cache rows.

Each `.c` begins with `#include <src/hydrogen.h>`. Includes use
`<src/folder/...>`. Every function has a header prototype. **No `static`
functions** in `src/` (build gate fails on new `static` functions). File-scope
state only.

---

## Channel Model

### What to call them: "subjects"

NATS uses the term **subject** for what a message is published to. The user's
question "do we call them channels?" — in NATS terminology the answer is
"subject." Code and docs say subject. The config array is `Subscriptions`.
Each entry's `Subject` is a suffix. The client prefixes
`cluster.<ClusterId>.`. There is no second array named `Subjects` or
`QueueSubscriptions`.

### Subject naming convention

Subjects are built as `cluster.<ClusterId>.<suffix>`. The suffix is one
or more dot-separated tokens. Do not hard-code `philement` in C; read
`ClusterId`. Wildcards, if a subscription needs them, are `*` (one token)
and `>` (the remainder). `+` is not a wildcard.

v1 subjects:

| Subject | Meaning |
| --- | --- |
| `cluster.<ClusterId>.cache.invalidate` | Result-cache invalidation. `query_ref` and `database` are in the JSON body, not in the subject |
| `cluster.<ClusterId>.instance.app_state` | Presence. One subject for every instance |
| `cluster.<ClusterId>.jobs.<name>` | Optional queue-group work. Not required for invalidation |

`order.updated` in examples elsewhere is a stand-in event name for the
WebSocket relay. There is no order subsystem to hook.

### Subscription model

Each Hydrogen instance subscribes to a configurable set of subjects. The config
lists the subjects to listen on. We distinguish:

- **Cluster-wide subscriptions** — every instance listens (e.g.,
  `cluster.<cluster_id>.auth.token-refresh`). All instances receive the
  message; each acts independently.
- **Queue-group subscriptions** — instances join a NATS **queue group** so that
  messages are delivered to only one instance (load-balanced). Useful for
  tasks that should run once per cluster, not once per instance (e.g., a
  background sync, a report generation trigger). Subject:
  `cluster.<cluster_id>.jobs.<name>`, queue group `<cluster_id>-jobs`.
- **Instance-group / instance-specific subscriptions** — for coordinating
  subsets of instances. See [Channel Coordination](#channel-coordination)
  below.

---

## Message Model

### What we send and receive

Messages are **JSON** (consistent with the rest of Hydrogen's API and config,
which uses jansson). Each message has a common envelope. `subject` here
is the on-wire name after the client prefixes `cluster.<ClusterId>.`.
The example assumes `ClusterId` is `philement`. Config does not store
that full string.

```json
{
  "event": "cache.invalidate_by_ref",
  "subject": "cluster.philement.cache.invalidate",
  "timestamp": "2026-10-05T00:00:00Z",
  "source": "hydrogen-01",
  "instance_id": "hydrogen-01",
  "data": {
    "database": "Acuranzo",
    "query_ref": 127,
    "reason": "mutation"
  }
}
```

### Message types (events)

Only `cache.invalidate_by_ref` and `app_state` are v1. The other names
are reserved so later work does not invent a second vocabulary. Do not
subscribe to them in v1. `instance.heartbeat` is not a subject. Presence
is `app_state`.

| Event | v1 | When emitted | When received | Purpose |
| --- | --- | --- | --- | --- |
| `cache.invalidate_by_ref` | yes | Explicit C or Lua call. One message per QueryRef. Subject suffix `cache.invalidate` | Look up `data.query_ref` in the QTC for `data.database`, then drop result-cache entries for that SQL template | Result-cache invalidation. Next read misses and runs SQL |
| `app_state` | yes | Instance publishes `Starting`, `Alive`, or `Stopping` on suffix `instance.app_state` | Peers track liveness from the stale window. `Alive` may include the current WebSocket connection count | Singleton detection and peer count. Unique-IP tallies are not v1 |
| `state.changed` | no | Reserved | `data.resource` + `data.id` | Later state-change notification |
| `order.updated` | no | Reserved stand-in for the WebSocket relay allowlist | — | There is no order subsystem |

### Cache invalidation hooks (the core use case)

v1 has two emitters. Neither reads `queries.collection`.

1. **Explicit C call** after a successful mutation, naming the database
   connection and one or more QueryRefs whose **results** are now stale.
   Each ref becomes one message on
   `cluster.<ClusterId>.cache.invalidate`.
2. **Lua** `H.nats.broadcast("cache.invalidate_by_ref", data)` with the
   same `database` and `query_ref` fields. See [Lua Host API](#lua-host-api).

A declarative list on the query row (working name `invalidate_refs`) can
come later. It needs a Helium column the bootstrap SELECT actually loads,
plus a field on `QueryCacheEntry`. Do not store it in `collection`, and
do not parse SQL comments for it.

On receipt, `nats_dispatch.c`:

- ignores the message when `instance_id` is this process (local evict
  already happened)
- loads the template with `query_cache_lookup` on that database's QTC
- calls a new `query_result_cache_invalidate_template(database, sql)`
- does not call `query_cache_clear` or `query_result_cache_clear`

"Invalidate" means delete matching result-cache entries. The next lookup
misses and runs the SQL. It does not mean write a fresh result, and it
does not mean drop the SQL template. The publisher does the same delete
locally before `PUB`, then publishes for peers. Parameter variants of
that template all go. That is coarser than one parameter set, and it
matches a list of QueryRefs.

The prepared-statement cache is a different structure. v1 does not
touch it. Template text changes when the process reloads the QTC.

**App state.** Each instance publishes `app_state` on
`cluster.<ClusterId>.instance.app_state` with `instance_id` and `state`
(`Starting`, `Alive`, `Stopping`). Peers drop an instance that has not
published `Alive` within `StaleAfterSeconds` (default three heartbeat
intervals). A crashed process simply stops publishing. v1 may attach
`websocket_connections` from the existing counter. It does not compute
unique client IPs.

---

## Channel Coordination

### Cluster identity

Config provides:

- `NATS.ClusterId` — subject prefix only (`cluster.<ClusterId>.…`).
  Default `philement`. This is not the NATS server cluster name
  (`festival` in the live `nats.conf`). Two Hydrogen deployments that
  must not hear each other use different `ClusterId` values.
- `NATS.InstanceId` — defaults to hostname (K8s pod name). Used in message
  `source`/`instance_id` and for instance-targeted subjects. If not set,
  derive from `gethostname()`.

### Instance-group / subset coordination

A Hydrogen deployment may have multiple "groups" of instances (e.g., a
public-facing group and an admin group, or groups serving different tenants).
Coordination strategies:

1. **Subject prefix per group** — config defines a `Group` field. Subjects
   become `cluster.<cluster_id>.group.<group>.auth.token-refresh`. Only
   instances in that group subscribe.

2. **NATS accounts / namespaces** — if the managed NATS supports accounts
   (NATS 2.x), different groups could use different accounts with shared
   users. Hydrogen would connect with an account-scoped credential.

3. **Dynamic subject interest** — instances can programmatically subscribe to
   additional subjects at runtime (e.g., a Lua script calls
   `H.nats.subscribe("cluster....order.updated.1234")`) and unsubscribe when
   the subscription is no longer needed. This lets individual instances
   express interest in specific resource streams.

4. **Suppress self-receipt** — set `no_echo: true` on the one connection
   that both publishes and subscribes. Also drop any message whose
   `instance_id` equals this process. Do not open a second connection
   to try to hide publishes; the subscriber connection would still
   receive them.

### Queue groups for cluster-wide single-processing tasks

For events that should trigger **one** action per cluster (not N), use a NATS
queue group. The config specifies queue-group subscriptions:

```json
{
  "Subject": "jobs.refresh-all",
  "Type": "queue-group",
  "QueueGroup": "philement-refresher"
}
```

That object is one element of `Subscriptions`. `Subject` is the suffix.
`QueueGroup` is the full name sent in `SUB`.

Each instance joins the same queue group; NATS load-balances each message to
exactly one member.

---

## WebSocket Broadcast Forwarding

The user wants client apps to receive notifications. When WebSocket is enabled,
NATS messages can be mirrored down to connected WebSocket clients.

### Design

- Config section `NATS.WebSocketRelay` lists which NATS event types are
  eligible for WebSocket forwarding (e.g., `["order.updated"]`).
  This is the server-side allowlist — events not in this list are never relayed.
- Each WebSocket connection carries a `subscribed_events` list — the event
  types the client has declared interest in at connection time. A client
  with no access to orders would not include `order.updated` in its list,
  so it never receives order information of any kind.
- Hydrogen does not keep a list of `wsi`s. Phase 6 adds one: append in
  the established callback, remove in the close callback, mutex held.
  `subscribed_events` is a new field on `WebSocketSessionData`. The
  client sends it in an application message after auth. There is no
  such message today.
- The NATS thread only enqueues `(event, json)` onto sessions whose
  `authenticated` flag is set and whose list contains `event`. The
  libwebsockets service thread performs `lws_write` from
  `LWS_CALLBACK_SERVER_WRITEABLE`. Calling `lws_write` from the NATS
  thread is a defect, not an implementation shortcut.
- The vhost name is `"hydrogen"` on `websocket.port`. There is no second
  chat vhost to iterate.
- Lua reaches a browser only by publishing an event that
  `WebSocketRelay` allows. There is no `H.ws.broadcast`. There is also
  no scripting config key that maps a subject to a Lua file; see
  [Lua Host API](#lua-host-api).

### Message format to clients

The same JSON envelope is sent to WebSocket clients. A `type` field distinguishes
NATS-relayed messages from native WebSocket protocol messages. `order.updated`
is a stand-in allowlist name. There is no order subsystem. `subject` is
the on-wire name.

```json
{
  "type": "nats_event",
  "event": "order.updated",
  "subject": "cluster.philement.order.updated",
  "data": { "order_id": 1234 }
}
```

---

## Instance Presence & Group Membership

The user requires that Hydrogen instances know whether they are running alone
(singleton) or in a multi-instance deployment, with awareness of peer liveness
for autoscaling and load-balancing decisions.

### Presence Design

- Each instance publishes `app_state` on subject `cluster.<cluster_id>.instance.app_state` with payload:
  - `instance_id`
  - `state`: one of `"Starting"`, `"Alive"`, `"Stopping"`
  - When `state == "Alive"`: optional `websocket_connections`, copied from the existing WebSocket counter. Not a unique-IP window.
- Instances subscribe to `cluster.<cluster_id>.instance.app_state` to track all peers.
- An in-memory registry (`nats_registry.c`) maintains the last-seen timestamp
  and the latest `app_state` payload for each peer. Peers are considered stale after
  `HeartbeatIntervalSeconds * 3` with no "Alive" update (configurable via `Presence.StaleAfterSeconds`).
- `H.nats.instances()` Lua function and a `GET /api/nats/instances` endpoint
  (if admin API is added) return the current peer list and singleton status.
- **Singleton detection**: if no other instance is `Alive` for one full
  stale window after startup, `H.nats.instances()` reports a singleton.
  Cache invalidation does not change based on that. Every peer still
  subscribes to `cache.invalidate`.
- **Client counts**: summing `websocket_connections` across `Alive` peers
  is allowed. Do not invent a 5-minute unique-IP set in v1. Nothing in
  the server records REST client IPs that way today.

### Config additions

```json
"Presence": {
  "Enabled": true,
  "HeartbeatIntervalSeconds": 60,
  "StaleAfterSeconds": 0,
  "ReportConnections": true
}
```

| Field | Type | Default | Notes |
| --- | --- | --- | --- |
| `Enabled` | bool | false | Enable instance presence / app_state broadcasting |
| `HeartbeatIntervalSeconds` | int | 60 | App_state publish interval (how often "Alive" is re-published) |
| `StaleAfterSeconds` | int | `HeartbeatIntervalSeconds * 3` | Peer expiry threshold (0 = auto) |
| `ReportConnections` | bool | true | Include active connection count in "Alive" payload |

---

## Lua Host API

Following the pattern where subsystems expose host API functions to Lua scripts
(see `H.mcp`, `H.mail`, `H.notify` in `src/scripting/`), NATS would expose:

| Function | Purpose |
| --- | --- |
| `H.nats.broadcast(event, data)` | Publish a JSON event to a cluster-wide subject |
| `H.nats.broadcast_sync(event, data)` | Publish and return when the bytes are written to the socket. No peer ack |
| `H.nats.subscribe(subject, handler)` | Subscribe to a NATS subject; handler receives parsed JSON envelope (optional — config-driven is the normal path) |
| `H.nats.unsubscribe(subject)` | Remove a runtime subscription |
| `H.nats.status()` | Return NATS connectivity / message counters |
| `H.nats.instances()` | Return live peer instances from app_state registry |

Config subscriptions are the normal path: the C dispatcher handles
`cache.invalidate` and `app_state`. Lua handlers for arbitrary subjects
are optional and are new code. `ScriptingConfig` has source roots,
workers, and sandbox flags. It has no `Handlers` map. `H.mcp` and
`H.mail` are C functions registered in the Lua state.

`H.nats.subscribe` / `H.nats.unsubscribe` are the optional runtime path.
Handle kind is `H_HK_NATS = 7` (the current last value is
`H_HK_MCP = 6`). Wire it in both wait functions. Subscribers, if it
lands second, uses `8`.

`H.wait` today special-cases HTTP, LLM, mail, notify, and MCP in
`H_lua_wait_one` and again in the multi-handle loop. A new kind that
is added in only one of those places is treated as a database query
and fails with "no pending query".

---

## Updated: Open Question #5 — Heartbeat necessity

Presence is required for peer count and singleton detection. The subject
suffix is `instance.app_state`, not `instance.heartbeat`. K8s liveness
probes cover crash detection and do not tell a pod how many peers are
up. Two Hydrogen pods mean cache invalidation must reach both, and a
queue-group job should be able to see that it has a peer.

## Config Sketch

`Subject` values below are suffixes. The client publishes and subscribes
on `cluster.<ClusterId>.<suffix>`. Nested `Enabled` flags match the
struct defaults. `StaleAfterSeconds: 0` means three heartbeat intervals.

```json
"NATS": {
  "Enabled": false,
  "Servers": ["nats://nats.nats.svc.cluster.local:4222"],
  "ClusterId": "philement",
  "InstanceId": "",
  "Group": "",
  "Username": "${env.NATS_USERNAME}",
  "Password": "${env.NATS_PASSWORD}",
  "TlsEnabled": false,
  "TlsCaCert": "",
  "ConnectionTimeoutSeconds": 10,
  "Reconnect": {
    "MaxRetries": -1,
    "Delays": [30, 60, 120, 240, 480],
    "SteadyDelaySeconds": 480
  },
  "Subscriptions": [
    {
      "Subject": "cache.invalidate",
      "Type": "cluster-wide"
    },
    {
      "Subject": "instance.app_state",
      "Type": "cluster-wide"
    },
    {
      "Subject": "jobs.refresh-all",
      "Type": "queue-group",
      "QueueGroup": "philement-refresh"
    }
  ],
  "WebSocketRelay": {
    "Enabled": false,
    "Events": ["order.updated"]
  },
  "Presence": {
    "Enabled": false,
    "HeartbeatIntervalSeconds": 60,
    "StaleAfterSeconds": 0,
    "ReportConnections": true
  },
  "Test": {
    "FailNextPublishOnLaunch": false,
    "MockConnection": false
  }
}
```

### Field descriptions

| Field | Type | Default | Notes |
| --- | --- | --- | --- |
| `Enabled` | bool | false | Clean skip when absent |
| `Servers` | string[] | — | NATS server URLs |
| `ClusterId` | string | "philement" | Namespace prefix for subjects |
| `InstanceId` | string | "" | Defaults to hostname |
| `Group` | string | "" | Optional instance group |
| `Username` / `Password` | string | null | Auth creds (env-injected) |
| `TlsEnabled` | bool | false | v1 rejects `true` (`ready = false`). The field is reserved so a later TLS phase does not rename config. Do not connect in plaintext when it is true |
| `ConnectionTimeoutSeconds` | int | 10 | Connect timeout |
| `Reconnect.MaxRetries` | int | -1 (unlimited) | -1 = forever |
| `Reconnect.Delays` | int[] | [30, 60, 120, 240, 480] | Cascading retry delays in seconds; after the last delay, stay at `SteadyDelaySeconds` |
| `Reconnect.SteadyDelaySeconds` | int | 480 | Retry interval after all cascading delays are exhausted |
| `Subscriptions` | array | [] | One array. `Subject` is a suffix (`cache.invalidate`, `instance.app_state`, optional `jobs.<name>`). Code prefixes `cluster.<ClusterId>.`. Do not add `PublishSubjects.ClusterPrefix` or a second `QueueSubscriptions` key |
| `WebSocketRelay` | object | `{Enabled: false}` | Forward events to WS clients; `Events` is the server-side allowlist; per-client `subscribed_events` set at WS connection time filters which clients receive each event |
| `Presence` | object | `{Enabled: false}` | Instance presence/heartbeat registry (see Presence section) |
| `Presence.HeartbeatIntervalSeconds` | int | 60 | App_state publish interval (how often "Alive" is re-published) |
| `Presence.StaleAfterSeconds` | int | 0 | `0` means three heartbeat intervals. A positive value is the window in seconds |
| `Test` | object | test seam flags | For blackbox/Unity testing |
| Scripting | — | — | No new `Scripting.Handlers` key in v1. Cache and presence are C dispatch. Optional `H.nats.subscribe` is a later phase |

---

## Completeness fences (what must be touched)

A phase is not complete if the behavior works but Hydrogen's normal structures
were skipped. This section lists every integration point.

### Config / AppConfig

- `config.h` — letter V in comment block
- `config_forward.h` — `NATSConfig` forward declaration
- `config_nats.h` / `config_nats.c` — struct + load/dump/cleanup/apply_defaults (includes Presence sub-struct)
- `config_defaults.c` — `initialize_config_defaults_nats()` + call in master init
- `config.c` — `LOAD_CONFIG("V", …)`, `DUMP_CONFIG_SECTION`, cleanup in `clean_app_config`
- `hydrogen.h` — `NATSConfig nats;` in `AppConfig`
- Example `examples/configs/hydrogen.json` section with `Enabled: false`.
  That file currently has no MCP section. A missing `NATS` section must
  still clean-skip, so `tests/configs/hydrogen_test_17_startup_min.json`
  stays untouched
- Test 12 env-var pattern for `Username`/`Password` (`${env.*}`)
- `tests/artifacts/hydrogen_config_schema.json` — root
  `additionalProperties` is true, and MCP is not in the schema. Do not
  block Phase 1 on a schema object. Phase 12 adds `NATS` when the
  operator guide is written

### Launch / landing / registry / threads

- `SR_NATS` in `globals.h`
- `launch_nats.c` — readiness clean-skip when disabled; No-Go when enabled+invalid
- `launch.h` — `check_nats_launch_readiness()` + `launch_nats_subsystem()` decls
- `launch_readiness.c` — 22nd `process_subsystem_readiness` call
- `launch.c` — `strcmp` dispatch in `launch_approved_subsystems()`
- `landing_nats.c` — drain + join + registry shutdown
- `landing.h` — declarations
- `landing.c` — `get_landing_function()` dispatch
- `landing_readiness.c` — table entry
- `src/state/state.h` — `nats_system_shutdown` shutdown flag extern
- `src/state/state.c` — `nats_system_shutdown` definition (or in `nats.c`)
- `src/threads/threads.h` / `src/state/state.c` — `ServiceThreads nats_threads` extern + definition
- Dependencies: Registry + Network always; Database when Enabled (cache hooks); WebSocket when WS relay enabled
  - `nats_registry.c` — peer registry for app_state tracking (no extra dependency)
  - `nats_subject.c` — subject validation helpers (no extra dependency)

`MAX_SUBSYSTEMS` (24) stays as-is. NATS is registration 22 of 24.
Subscribers is planned as 23. One slot remains. The readiness writer
does not bounds-check the index.

### API / Swagger / prefix (if admin API is needed)

- Handler files under `src/api/nats/` + `nats_service.h` tag
- `api_service.c` routes + `json_endpoints`
- JWT auth (if endpoints need protection)
- `payloads/swagger-generate.sh` reads `//@ swagger:` annotations
- Test 20 (prefix), Test 22 (Swagger), Test 17 (min/max startup)

### Status / observability

- `status_core.h` — a `ServiceMetrics nats` member of `SystemMetrics` and
  a `specific.nats` union arm, matching MCP (member and arm, not one of
  them). Counters include peer count, reconnect count, and messages
  published and received. `state` is `degraded` or `up`
- `GET /api/nats/status` (if API is added) or counters in `/api/system/info`
- `GET /api/nats/instances` (if presence enabled) — peer list + singleton status
- `log_this(SR_NATS, …)` — `num_args` matches `%` count
- Log in startup SERVICES / THREADS / QUEUES sections in `launch.c`

### Scripting

- `H.nats.broadcast` / `broadcast_sync` / `subscribe` / `unsubscribe` / `status` / `instances`
- No `Scripting.Handlers` map in v1. C dispatch owns `cache.invalidate` and `app_state`
- `H.nats.subscribe()` / `H.nats.unsubscribe()` — optional, and new. Not an existing scripting feature
- No `H.ws.broadcast()` — WebSocket push from Lua goes through NATS relay only
- Handle kind constant in `src/scripting/scripting_handle.h`
- `lua_api.md` for `H.nats.*`. Document that `broadcast_sync` means "bytes hit the socket"

### Helium

- If any coordination state needs persistence (e.g., subscription registry,
  instance table), Helium migrations + QueryRefs. But the plan leans toward
  in-memory-only coordination to avoid cross-engine complexity.

### Tests / extras

- Unity under `tests/unity/src/nats/` mirroring all `src/nats/*.c` files
  (including `nats_registry.c` and `nats_subject.c`)
- One test file per function, `<source_test_function>.c` naming
  (see [`TESTING_UNITY.md`](/docs/H/tests/TESTING_UNITY.md) for orphan-detection
  and CMake basename-uniqueness rules)
- Prefer a real `nats-server` on port 5620, started by the test script,
  with no credentials. It is not on `PATH` on this workstation
  (2026-10-05). Phase 11 installs or locates it. Add `extras/natsval`
  only if that binary is missing in CI. Do not invent it before the
  blackbox phase needs it.
- `tests/test_62_nats.sh` + `docs/H/tests/test_62_nats.md` + 3 config files
- Ports inside `5620–5629`: NATS listen **5620**; Hydrogen **5621–5624**;
  closed port **5628** for the unreachable case. Do not bind Hydrogen and
  `nats-server` to the same port
- CHANGELOG + TEST_VERSION at the top of every script
- `jq` only for JSON in tests
- `TEST_COUNTER` owned by the framework — never increment manually
- Test 04 (markdown links), Test 90 (markdownlint), Test 91 (cppcheck),
  Test 92 (shellcheck), Test 93 (jsonlint), Test 98 (luacheck)

### Docs (Phase 12)

- `docs/H/core/subsystems/nats/` — subsystem guide (incl. app_state presence design)
- `docs/H/api/nats/` — API reference (`GET /api/nats/status`, `GET /api/nats/instances`)
- Index updates: `README.md`, `SITEMAP.md`, `STRUCTURE.md`,
  `API_OVERVIEW.md`, `lua_api.md`, `TESTING.md` (Test 62 row), `SECRETS.md`
  (NATS credential env names). `INSTRUCTIONS.md` already lists U. Chat and
  reserves V / launch 22. Phase 12 promotes V from reserved to present
  once `config.h` has the letter. Do not add the operator guide before
  the code exists.
- Chat's missing `DUMP_CONFIG_SECTION("U")` and the uncalled
  `initialize_config_defaults_chat` are pre-existing. They are not a NATS
  exit gate. Fix them only in a change whose subject is Chat.
- Landing order is the order of the table in `landing_readiness.c`, first
  entry first. It is not the reverse of launch. NATS is inserted before
  Print.

---

## Testing Strategy

### Coverage fences (repo-wide rules that apply)

- **Unity unit tests** target `src/nats/` files. One test file per function,
  following `<source_test_function>.c` naming (see [`TESTING_UNITY.md`](/docs/H/tests/TESTING_UNITY.md)).
  Test files mirror source structure: `tests/unity/src/nats/`.
- **Blackbox integration tests** run the real binary with embedded payload
  (Test 62). Test 89 reconciles combined Unity + blackbox coverage.
- **Coverage target**: source files over 100 lines need >75% unit test coverage
  (repo constraint). Combined Unity + blackbox target 85%.
- **`TEST_COUNTER` is framework-owned** — never increment manually.
- **`mkt`** when a `.c` file is added or removed. The source glob is
  resolved at configure time. `mkq` is enough for edits to files that
  are already in the build. Unity files are also picked up only on
  reconfigure.
- **`mkp`** (cppcheck, Test 91) and **`mks`** (shellcheck, Test 92) after each
  phase.

### Unity unit tests — per source file

Each `.c` file in `src/nats/` gets dedicated Unity tests. Mock injection follows
the existing mock framework (`tests/unity/mocks/`). The NATS client connection
layer should sit behind an injectable seam (function pointer table or mock header,
like `mock_system`) so Unit tests never require a live NATS server.

The names in the table are the Phase 0 sketch. Tests that exist use
`<source>_test_<function>.c` and are listed in the phase status
blocks. Phase 4 added `nats_dispatch_test_nats_dispatch_message.c`.
Do not add `nats_test_publish.c` for a symbol that is not in the tree.

| Source file | Unit-testable functions | Unity test files |
| --- | --- | --- |
| `nats.c` | `nats_init()`, `nats_shutdown()`, `nats_get_status()` (includes `state: healthy \| degraded`) | `nats_test_init.c`, `nats_test_shutdown.c`, `nats_test_get_status.c` |
| `config_nats.c` | load, dump (password redacted), cleanup, defaults, subscription subject check | `config_nats_test_load.c`, `config_nats_test_dump.c`, `config_nats_test_defaults.c` |
| `nats_publish.c` | `nats_publish()`, `nats_broadcast()`, `nats_build_envelope()`, `nats_broadcast_to_subject()` | `nats_test_publish.c`, `nats_test_build_envelope.c`, `nats_test_broadcast.c` |
| `nats_subscribe.c` | `nats_subscribe()`, `nats_unsubscribe()`, `nats_subscriptions_init()` | `nats_test_subscribe.c`, `nats_test_unsubscribe.c`, `nats_test_subscriptions_init.c` |
| `nats_dispatch.c` | dispatch, skip-self, parse, invalidate-by-ref (template hash), app_state | `nats_dispatch_test_message.c`, `nats_dispatch_test_skip_self.c`, `nats_dispatch_test_invalidate.c`, `nats_dispatch_test_app_state.c` |
| `nats_ws_bridge.c` | `nats_ws_bridge_start()`, `nats_ws_bridge_stop()`, `nats_should_relay_to_ws()`, `nats_forward_to_clients()` | `nats_test_ws_bridge_start.c`, `nats_test_should_relay_to_ws.c`, `nats_test_forward_to_clients.c` |
| `nats_queue.c` | `nats_queue_init()`, `nats_queue_push()`, `nats_queue_pop()`, `nats_queue_destroy()` | `nats_test_queue_init.c`, `nats_test_queue_push.c`, `nats_test_queue_pop.c`, `nats_test_queue_destroy.c` |
| `nats_reconnect.c` | `nats_reconnect()`, `nats_backoff_delay()` (cascading: 30, 60, 120, 240, 480, then steady 480), `nats_reconnect_should_retry()` | `nats_test_reconnect.c`, `nats_test_backoff_delay.c`, `nats_test_reconnect_should_retry.c` |
| `nats_registry.c` | `nats_registry_init()`, `nats_registry_update_peer()`, `nats_registry_peers()`, `nats_registry_is_singleton()`, `nats_registry_active_count()`, `nats_registry_active_clients()` | `nats_test_registry_init.c`, `nats_test_registry_update_peer.c`, `nats_test_registry_peers.c`, `nats_test_registry_is_singleton.c`, `nats_test_registry_active_count.c`, `nats_test_registry_active_clients.c` |
| `nats_subject.c` | `nats_validate_subject_name()`, `nats_build_subject()` | `nats_test_validate_subject_name.c`, `nats_test_build_subject.c` |

**Mock strategy**: There is no `cnats` library to mock. Put connect,
read, and write behind function pointers on the client. Unity supplies
a byte buffer that speaks `INFO` / `MSG` / `PING`. Dispatch tests call
`nats_dispatch_message` with a parsed envelope and a fake
`query_result_cache_invalidate_template`. Do not define
`USE_MOCK_NATS` against symbols that will never exist.

### Blackbox integration test — Test 62

`tests/test_62_nats.sh` is the end-to-end integration test. Config file:
`tests/configs/hydrogen_test_62_nats.json`. Port range: `562x` (single instance,
as NATS does not require multi-DB variants for its own logic).

**Subtests** (each `print_subtest` → exactly one `print_result`):

| # | TEST | What |
| --- | --- | --- |
| 1 | NATS disabled clean-skip | `NATS.Enabled: false` → server starts, no NATS connections, shutdown clean |
| 2 | NATS enabled, server unreachable | `Enabled: true`, servers pointed at `127.0.0.1:5628`. Readiness is Go (`ready = true`). Launch returns success. Status shows `state: "degraded"`. Use a test delay list of 1 second. Do not sleep through the production 480s ladder |
| 3 | NATS enabled connects | `Enabled: true` + local mock NATS → connects, subscribes to configured subjects, publishes heartbeat within `HeartbeatIntervalSeconds + grace` |
| 4 | Result-cache invalidation | Two Hydrogen processes. A cache-queue query is warm on B. A calls the explicit invalidate for that ref and database. B's next execution of that template misses and hits the database. Assert with the result-cache entry count, not `query_cache_clear` |
| 4a | Local evict does not wait | A drops its own entries before publish returns. Stopping `nats-server` after the local delete still leaves A correct. B stays stale, which is the at-most-once case |
| 4b | Lost message stays stale | Covered by 4a on B. Do not expect a timeout to heal it |
| 5 | WebSocket relay | With `WebSocketRelay.Enabled: true` + authenticated WS client with `subscribed_events` including `order.updated` → NATS broadcast event arrives as `nats_event` message on WS |
| 6 | WS relay filtering | With `WebSocketRelay.Events` list excluding an event type → that event is NOT forwarded to WS clients; also, a WS client whose `subscribed_events` excludes an event type does NOT receive it even if the server allowlist includes it |
| 7 | Presence / singleton detection | Two instances launched → both see each other as live peers within `HeartbeatIntervalSeconds * 3`; singleton reports 0 peers |
| 7a | app_state | A publishes `Alive`, B lists A. A publishes `Stopping` or goes silent past `StaleAfterSeconds` (set the test window to a couple of seconds). B drops A. No unique-IP assertion |
| 8 | Self-suppression | Instance publishes on a subject it also subscribes to → handler skips processing (`source == instance_id`) |
| 9 | Queue group load balancing | Queue-group subscription on `jobs.refresh-all` → message delivered to exactly one of N instances |
| 10 | Reconnect backoff | NATS server stops mid-session → Hydrogen reconnects on next backoff tick; verify via status endpoint shows `reconnect_count > 0` |
| 11 | Clean shutdown | SIGTERM during active NATS session → `nats_system_shutdown` flag set, threads joined, connection drained, server exits cleanly |

**NATS server fixture**: A real `nats-server -D` (in-memory store, dev mode)
started by the test script before launching Hydrogen. Port `5620`. Credentials:
none (dev mode). A fallback `extras/natsval` mock binary (analogous to
`extras/mailval`) can be used if the container is unavailable in CI.

**Helpers**: `tests/lib/nats_helpers.sh` — `run_nats_server()`, `stop_nats_server()`,
`nats_publish_test_event()`, `nats_wait_for_subscription()`, `assert_ws_relay()`.

### Blackbox test documentation

`docs/H/tests/test_62_nats.md` — mirrors the style of
[`test_47_mcp.md`](/docs/H/tests/test_47_mcp.md) and
[`test_43_scripting.md`](/docs/H/tests/test_43_scripting.md).

### Test config files

| Config | Port | NATS enabled? | Purpose |
| --- | --- | --- | --- |
| `hydrogen_test_62_nats_disabled.json` | 5621 | false | Subtest 1. NATS listen 5620 stays unused |
| `hydrogen_test_62_nats_local.json` | 5622 | true | Connected cases. Peer process uses 5624 |
| `hydrogen_test_62_nats_bad.json` | 5623 | true | Subtest 2. `Servers` is `nats://127.0.0.1:5628` |

### Library dependencies

- No external NATS C library dependency — custom client (Option B).
- jansson for JSON (CONNECT handshake + MSG payloads).
- Network subsystem for socket/epoll layer.
- `src/launch/launch_mcp.c` has a pattern for checking library availability at
  startup; NATS readiness should follow the same — if `Enabled: true` and the
  client cannot initialize, return "degraded" (not No-Go). Retries continue
  with cascading backoff; if NATS later becomes available, transition to healthy.

### Linting matrix (Tests 90–98)

| Test | What it checks for NATS |
| --- | --- |
| 90 | `NATS_PLAN.md` / `test_62_nats.md` — absolute links, no trailing spaces, headings |
| 91 | `src/nats/*.c/h` — cppcheck all-directives, no suppressions |
| 92 | `tests/test_62_nats.sh`, `tests/lib/nats_helpers.sh` — shellcheck all-directives |
| 93 | `tests/configs/hydrogen_test_62_nats_*.json` — valid JSON |
| 98 | Any Lua files in `src/scripting/` for `H.nats.*` bindings |

---

## Open Questions

1. **NATS C library**: RESOLVED — Option B, roll our own. No external NATS C library dependency. Custom client in `src/nats/nats_client.c`. See [NATS Client Implementation](#nats-client-implementation).

2. **Cache invalidation granularity**: AMENDED 2026-10-05 — The stale data is the global query **result** cache, not the QTC. v1 evicts by SQL template for one `(database, query_ref)`. All parameter variants of that template drop. The next read misses and runs SQL. Nothing writes a fresh row on receipt. `collection` / `InvalidateQTC` is not loaded and is not the mechanism. See [Verified constraints](#verified-constraints-2026-10-05).

3. **Token table refresh mechanism**: RESOLVED — OIDC sits above the query cache layer and calls queries like any other subsystem. It does not maintain its own mutable token table. Cache invalidation/refresh is handled by the subscribe system at the database layer; OIDC is "none the wiser." The `token_refresh` NATS event is a phantom requirement and has been removed from the message model.

4. **WebSocket relay filter**: AMENDED 2026-10-05 — `subscribed_events` does not exist yet. Phase 6 adds the field and a session list. The NATS thread enqueues; the libwebsockets thread writes. See [WebSocket Broadcast Forwarding](#websocket-broadcast-forwarding).

5. **Heartbeat necessity**: RESOLVED — instance presence is now a required feature. See [Instance Presence & Group Membership](#instance-presence--group-membership). K8s probes cover crash detection; NATS heartbeats provide peer-count and singleton awareness.

6. **Error handling**: AMENDED 2026-10-05 — same delays (30, 60, 120, 240, 480, then 480). Readiness stays `ready = true` when the server is down so the retry thread actually starts. Status `state` is `degraded` until connect, then `up`. `ready = false` is invalid config only. Reconnect re-sends every `SUB`.

7. **Config letter conflict**: RESOLVED — NATS takes **V** (implemented first). NOTIFICATIONS/SUBSCRIBERS plan, if approved later, shifts to **W**. Documented in both plans.

8. **Subscription path**: AMENDED 2026-10-05 — C dispatch for `cache.invalidate` and `app_state` is the v1 path. There is no `Scripting.Handlers` entry. `H.nats.subscribe` is optional later work. Handle kind `H_HK_NATS = 7`.

9. **Self-suppression**: AMENDED 2026-10-05 — one connection, `CONNECT` field `no_echo: true`, plus an `instance_id` check. Two connections do not suppress self-delivery. The publisher evicts locally before `PUB` and does not wait to hear itself.

10. **Auth simplicity**: RESOLVED — internal DOKS cluster, no external exposure. Simple auth only (username/password from env vars, or none). No NKEYS/JWT enterprise auth needed. Credentials in config via `${env.NATS_USERNAME}` / `${env.NATS_PASSWORD}` env var substitution.

11. **App state payload**: AMENDED 2026-10-05 — `{instance_id, state}` with optional `websocket_connections` (the live counter). No unique-IP window. One subject `cluster.<ClusterId>.instance.app_state`. A peer that stops publishing is removed after the stale window.

12. **Delivery guarantees**: AMENDED 2026-10-05 — core NATS on this cluster is at-most-once. A lost invalidation does **not** cause the next lookup to miss. The result cache keeps the old JSON until something deletes it or the process restarts. v1 accepts that. Do not describe `timeout_seconds` as a safety net.

13. **Originating instance**: AMENDED 2026-10-05 — the publisher deletes the local result-cache entries, then publishes. It does not write a replacement result, and it does not wait for an echo.

14. **Subject strategy**: AMENDED 2026-10-05 — one subject, `cluster.<ClusterId>.cache.invalidate`. The ref is in the body. A subject per ref would still be subscribed by every instance, so it adds wildcards (`*`, never `+`) without filtering.

15. **Declarative list**: AMENDED 2026-10-05 — not in v1. A future column must be part of the bootstrap SELECT. `collection` is the wrong column.

16. **NATS server**: CONFIRMED 2026-10-05 — image `nats:2.11.4`, three pods, client port 4222, monitor 8222, route port 6222, cluster name `festival`, no auth, no TLS, no JetStream. Client URL `nats://nats.nats.svc.cluster.local:4222`. Do not dial 6222. `ClusterId` is not `festival`.

17. **TLS**: CONFIRMED absent. v1 is plaintext. A later TLS phase uses OpenSSL on the NATS socket. There is no Network TLS helper to reuse.

18. **WebSocket vhost**: CONFIRMED — one vhost named `hydrogen` on `websocket.port`. There is no session list yet. Phase 6 creates one. The NATS thread does not call `lws_write`.

19. **Write path**: AMENDED 2026-10-05 — WebSocket owns the session list and the writable callback. NATS owns the decision of which event is eligible.

20. **Config file**: AMENDED 2026-10-05 — parsing is only `src/config/config_nats.c`. Do not add `src/nats/nats_config.c`. Sockets and reconnect stay in `nats_client.c`.

21. **Handle kind constant**: AMENDED 2026-10-05 — `H_HK_NATS = 7`. The current last value is `H_HK_MCP = 6`. Subscribers, landing second, is `8`. Wire the kind in both `H.wait` functions.

22. **Presence registry persistence**: RESOLVED — registry is ephemeral. Clients re-register on reconnect. No persistence needed.

23. **Queue group names**: AMENDED 2026-10-05 — the config string is the full queue-group name sent in `SUB`. Code does not prepend `ClusterId` a second time. Operators put the cluster id in the name if two deployments share a NATS server.

24. **`app_state` counts**: AMENDED 2026-10-05 — optional current WebSocket connection count only. See question 11.

---

## Proposed phased breakdown

The phases below are the work plan. Each phase is its own conversation.
Gate template:
Goal, Entry gate, Work items, Done means, Exit gate, Status. Build aliases:
`mkq` / `mkt` / `mku <base>` / `mkp` / `mka` / `mks` (see
[INSTRUCTIONS.md](/docs/H/INSTRUCTIONS.md)).

| Phase | Goal | Key deliverables |
| --- | --- | --- |
| 0 | Design lock | **Approved 2026-10-05.** Letter V, launch 22, message model, channel model, custom client, app_state naming, WebSocket broadcast boundary |
| 1 | Config + launch + landing | `config_nats.c/h`, `launch_nats.c`, `landing_nats.c`, `nats_subject.c`, wiring in all dispatch tables, disabled clean-skip |
| 2 | NATS connection + lifecycle | Plaintext client: INFO, CONNECT (`no_echo`), PUB/SUB/MSG, PING/PONG, reconnect that re-sends SUBs. No TLS. Unity against a fake socket in this phase |
| 3 | Publish path (broadcast) | `nats_broadcast()` function, message envelope, test seam |
| 4 | Subscribe + dispatch | Receive messages, parse JSON, dispatch to cache invalidation hooks |
| 5 | Result-cache invalidation | New `query_result_cache_invalidate_template`. Explicit emit plus local evict. One subject. No `collection` parsing |
| 6 | WebSocket relay | Session list, `subscribed_events`, enqueue from NATS, `lws_write` on the service thread |
| 7 | Instance presence & registry | `nats_registry.c`, `app_state` publish/subscribe, singleton detection, active client tally |
| 8 | Lua host API | `H.nats.broadcast` / `subscribe` / `unsubscribe` / `status` / `instances` |
| 9 | Status + metrics | Counters in `status_core.h`, `GET /api/nats/status` + `GET /api/nats/instances` |
| 10 | Coverage fence | Per-file Unity fence for `src/nats/` and `src/config/config_nats.c`. Tests are written in the phase that adds the code. This phase only closes gaps |
| 11 | Blackbox Test 62 | End-to-end integration with local `nats-server`; all 12 subtests pass |
| 12 | Docs + indexes | Operator guide, API docs, INSTRUCTIONS.md/STRUCTURE.md/SITEMAP.md updates, lua_api.md |

---

## Phase 0 — Design lock

### Goal

Approve or amend the design locks below. **No `src/` edits in this phase.**
Confirms config letter V, launch position 22, the message model, the channel
model, the custom-client decision, the `app_state` subject naming, and the
WebSocket broadcast boundary.

### Entry gate

- This plan exists and has been read against Hydrogen project conventions
  ([INSTRUCTIONS.md](/docs/H/INSTRUCTIONS.md),
  [TESTING.md](/docs/H/tests/TESTING.md),
  [TESTING_UNITY.md](/docs/H/tests/TESTING_UNITY.md),
  [NOTIFICATIONS_PLAN.md](/docs/H/plans/NOTIFICATIONS_PLAN.md)).
- Source code verified: `config.h` letters A–U (V free in the tree),
  `MAX_SUBSYSTEMS` = 24, `launch_readiness.c` has 21
  `process_subsystem_readiness` calls. NATS is registration 22.
  Subscribers is planned as 23. One slot remains. `subscribed_events`
  and `ws_broadcast_json` do **not** yet exist (new additions).

### Work items

- [x] 0.1 Confirm config letter **V** / launch **22** (after MCP at 21).
- [x] 0.2 Confirm a plaintext custom client on its own TCP socket.
      `no_echo: true`, one connection, no `cnats`, no Network subsystem
      API, no TLS in v1, do not dial port 6222.
- [x] 0.3 Confirm message model: JSON envelope with `event`, `subject`,
      `timestamp`, `source`, `instance_id`, `data`.
- [x] 0.4 Confirm on-wire subjects
      `cluster.<ClusterId>.cache.invalidate` and
      `cluster.<ClusterId>.instance.app_state`. Config stores the suffix
      only. Code adds the prefix. `QueueGroup` is the full name. Ref and
      database travel in the JSON body. Wildcards are `*` and `>`, never
      `+`.
- [x] 0.5 Confirm the WebSocket relay needs a new session list and a new
      `subscribed_events` field, and that `lws_write` stays on the
      libwebsockets thread.
- [x] 0.6 Confirm config parsing is only `src/config/config_nats.c`.
      No `src/nats/nats_config.c`.
- [x] 0.7 Confirm invalidation targets `query_result_cache` by SQL
      template. Local delete, then publish. No `InvalidateQTC`, no
      `collection` field, no optimistic fill. A lost message stays stale.
- [x] 0.8 Confirm `no_echo: true` on one connection, plus an
      `instance_id` check. Not two connections.
- [x] 0.9 Confirm unreachable NATS is `ready = true` plus status
      `degraded`, with the retry thread running. `ready = false` is
      invalid config only, including `TlsEnabled: true` in v1.
- [x] 0.10 Confirm Chat's missing config dump is **not** a NATS exit
      gate. Landing inserts NATS before Print, not after MCP.
      `landing_plan.c` `expected_order[]` is only the Go/No-Go log; add
      `SR_NATS` before `SR_PRINT` and do not rebuild that list.
- [x] 0.11 Record amendments in this document if any lock changes.

### Done means

All locks in this section are approved or explicitly amended; Phase 0 Status
marked complete. No C compiled.

### Exit gate

User explicit approval of Phase 0. No `src/` edits.

### Implementation Status

**Approved** 2026-10-05. Phase 1 has not started. No `src/` edits in the approval turn.

### Lessons learned

- 2026-10-04: `subscribed_events` and `ws_broadcast_json` are new additions;
  the original "Current observed state" table incorrectly claimed they existed.
- 2026-10-04: Config parsing was first described as `src/nats/nats_config.c`.
  2026-10-05 moved that role to `src/config/config_nats.c` only, matching
  every other subsystem. Sockets stay in `nats_client.c`.
- 2026-10-05: Review against the tree. The result cache is
  `query_result_cache`, not the QTC. `timeout_seconds` is a query
  deadline. `collection` is not loaded. NATS echo needs `no_echo`.
  `+` is not a wildcard. Network is not a socket layer. Live NATS is
  2.11.4, plaintext, port 4222, cluster name `festival`, no auth.
  A lost invalidation stays cached. Config stores subject suffixes and
  the client prefixes `cluster.<ClusterId>.`. `landing_plan.c`
  `expected_order[]` is a Go/No-Go log, not shutdown order. Details are
  in [Verified constraints](#verified-constraints-2026-10-05).
- 2026-10-04: `app_state` subject must follow the `cluster.<id>.<domain>.<action>.<resource>`
  convention — `cluster.<id>.app_state` is a flat name that does not comply.
  Proposed fix: `cluster.<id>.instance.app_state`.
- 2026-10-04: config.h comment block and config.c "Configuration Sections" comment go
  A–U (Chat is present). `DUMP_CONFIG_SECTION("U", …)` for Chat is **missing** in
  config.c — the dump sequence jumps from T (MCP) to `#undef`, skipping U. This is a
  pre-existing. It is not a NATS exit gate. Fix it only in a Chat change.
- 2026-10-04: landing table (`landing_readiness.c`) is not the reverse of
  launch order. 2026-10-05 corrected the insertion point: NATS lands
  **before Print**, while WebSocket and Database are still up. Inserting
  after MCP shuts NATS down only after those subsystems are already gone.
- 2026-10-04: WebSocket has a single named vhost ("hydrogen") and its own dedicated port
  (`websocket.port`), not "no vhost". The NATS bridge iterates sessions on that single
  vhost, so no vhost routing complexity exists.
- 2026-10-04: `SR_CHAT`, `SR_MIRAGE`, and `SR_WEBSOCKET_CHAT` exist in globals.h but are
  **not** registered in the landing readiness table and Chat has no launch/landing
  functions (config-only). This is expected — Chat is config-only like Webhooks.
- 2026-10-04: `mcp_threads` is declared in `registry_integration.h` and
  again in `threads.h`. `mcp_system_shutdown` is in `state.h`.
- 2026-10-05: Phase 0 approved. Indexes and `INSTRUCTIONS.md` reserve
  letter V and launch 22. They do not claim the code exists. `nats-server`
  is not on `PATH`. Phase 1 is the next conversation.

---

## Phase 1 — Config, launch, landing

### Goal

`NATS` config loads, dumps, cleans up, and the subsystem registers,
launches, and lands. Disabled and missing sections are a clean skip.
No TCP client, no cache hook, no WebSocket session list.

### Dependencies

Phase 0 approved.

### Entry gate

Phase 0 Status is approved. This section is the work list.

### Work items

- [x] 1.1 `config_nats.h` / `config_nats.c` in `src/config/`. No
      `src/nats/nats_config.c`. Letter **V** in the `config.h` comment
      block, after U. Chat. `config_forward.h`, `LOAD_CONFIG("V")`,
      `DUMP_CONFIG_SECTION("V")`, cleanup in `clean_app_config`,
      `NATSConfig nats` on `AppConfig`. Call
      `initialize_config_defaults_nats` from the master init. Chat's
      initializer is not called today; do not copy that bug, and do not
      add Chat's missing dump in this phase.
- [x] 1.2 Parse the sketch in [Config Sketch](#config-sketch). `Subject`
      is a suffix. Reject `TlsEnabled: true`, an enabled section with
      no servers, a bad URL, and a bad delay list (`ready = false`).
      Defaults match the field table, including `StaleAfterSeconds` 0.
      Passwords stay out of the dump.
- [x] 1.3 `nats_subject.c`: build `cluster.<ClusterId>.<suffix>`.
      Reject an empty id, an empty suffix, and a suffix that already
      starts with `cluster.`. No sockets. `nats_registry.c` waits for
      Phase 7.
- [x] 1.4 `SR_NATS` in `globals.h`. `launch_nats.c` and
      `landing_nats.c`. Wire `launch.h`, `launch.c`,
      `launch_readiness.c` (22nd call, after MCP), `landing.h`,
      `landing.c`, and `landing_readiness.c` (before Print).
      `nats_system_shutdown` and `ServiceThreads nats_threads` follow
      the MCP externs. In `landing_plan.c`, add `SR_NATS` before
      `SR_PRINT` only. Do not rebuild `expected_order[]`. Do not bump
      `MAX_SUBSYSTEMS`.
- [x] 1.5 Readiness. Missing or disabled: `ready = true`, launch returns
      1, no thread. Enabled and valid: `ready = true`, launch returns 1,
      still no thread. The retry thread arrives in Phase 2, including
      the unreachable-but-valid case. Enabled and invalid:
      `ready = false`. Register a Network dependency. Register Database
      when Enabled. Leave the WebSocket dependency for Phase 6.
- [x] 1.6 Disabled `NATS` object in `examples/configs/hydrogen.json`.
      Leave the Test 17 min config without a `NATS` section.
- [x] 1.7 Unity in this phase, one file per function, MCP names as the
      pattern: `config_nats_test_load_nats_config`,
      `launch_nats_test_check_nats_launch_readiness`,
      `launch_nats_test_launch_nats_subsystem`,
      `landing_nats_test_check_nats_landing_readiness`. Cover disabled,
      missing, valid, and the four invalid cases in 1.2.
- [x] 1.8 `mkt` (new `src/` and Unity files are invisible to `mkq`),
      then `mkp`. `test_17` min still reaches ready. The user runs
      those commands.

### Done means

Trial build is green. The named Unity tests pass. Dump redacts the
password. A missing section still launches. No socket is opened.

### Exit gate

`zsh -ic 'mkt'`, the named `mku` bases, `zsh -ic 'mkp'`. `test_17` min
if it is run.

### Status

**Complete** 2026-10-05. `mkp` passed (2,189 files). `mkt` passed
(4m 38s, shutdown test passed, no NATS symbol in the dead-code list).
The five Unity bases passed: `config_nats_test_load_nats_config` (14),
`launch_nats_test_check_nats_launch_readiness` (8),
`launch_nats_test_launch_nats_subsystem` (4),
`landing_nats_test_check_nats_landing_readiness` (1),
`nats_subject_test_nats_subject_build` (2). `test_17` was not run.

### Implementation note

2026-10-05: config, launch, landing, subject builder, shutdown stub,
example `NATS` object (`Enabled: false`), and the Unity files named in
1.7 plus `nats_subject_test_nats_subject_build`. No socket is opened.
`TlsEnabled: true`, no servers, a bad URL, and a bad delay list load
successfully and make `ready = false`. A missing or disabled section
stays a clean skip. cppcheck `variableScope` on the server and event
lookups was fixed by declaring them in the block that uses them. The
password Unity check reads the mock log history (`*****` present, the
secret absent) because Unity replaces `log_this` and
`log_get_messages` does not see those lines.

---

## Phase 2 — NATS connection + lifecycle

### Goal

One plaintext NATS connection. The client speaks INFO, CONNECT
(`no_echo`), PUB, SUB, UNSUB, MSG, and PING/PONG. A retry thread
reconnects and re-sends every SUB. No TLS. No cache dispatch, no
WebSocket relay, no registry, no Lua, no status HTTP, no Test 62.

### Dependencies

Phase 1 complete.

### Entry gate

Phase 1 Status is complete. `mkp`, `mkt`, and the five Phase 1 Unity
bases passed on 2026-10-05.

### Work items

- [x] 2.1 Plaintext client in `src/nats/`. Read INFO. Refuse
      `tls_required`. Refuse `auth_required` when no username is set.
      Send CONNECT with `verbose: false`, `pedantic: true`,
      `protocol: 1`, `lang: "c"`, `no_echo: true`, and optional `name`,
      `user`, and `pass`. Do not log the CONNECT line. SUB from config
      suffixes via `nats_subject_build`. Queue-group subscriptions send
      `QueueGroup` as the queue token. Answer PING with PONG. `-ERR` and
      unknown ops (`HMSG`) are protocol failures. MSG payload is the
      declared byte count plus CRLF. One read may hold several ops or a
      split body. Do not dial port 6222.
- [x] 2.2 Reconnect thread. Delays are 30, 60, 120, 240, 480, then
      `SteadyDelaySeconds`. `MaxRetries < 0` retries forever.
      `MaxRetries >= 0` stops after that many handshake failures
      (`0` is one attempt and no retry). The wait wakes on shutdown.
      After CONNECT the link is `up`. Until then it is `degraded`.
      A drop after `up` still reconnects and re-sends SUBs. The outbound
      queue survives the handshake. `nats_on_msg` logs the subject and
      length at TRACE and does not dispatch.
- [x] 2.3 Launch. Enabled and valid calls `nats_start` and returns 1
      when the thread starts, including when the first connect fails.
      Disabled still returns 1 with no thread. Invalid still returns 0.
      `MockConnection` installs the in-source IO that fails immediately.
      Tests install their own IO table and do not dial.
- [x] 2.4 Unity, fake socket only:
      `config_nats_test_nats_server_endpoint`,
      `nats_client_test_nats_session_handshake`,
      `nats_client_test_nats_parser_feed`,
      `nats_client_test_nats_client_publish`,
      `nats_reconnect_test_nats_reconnect_delay_seconds`,
      `nats_reconnect_test_nats_reconnect_should_retry`,
      and the updated `launch_nats_test_launch_nats_subsystem`.
      The enabled-valid launch case expects one thread, link
      `degraded`, then a joined shutdown with link `down`.
- [x] 2.5 Exit gate below. New `src/` and Unity files are invisible to
      `mkq`. Do not check these boxes until the commands pass.

### Done means

`mkp` is clean. `mkt` is green and the dead-code list has no new NATS
symbol. The Unity bases in 2.4 pass. No live `nats-server` is required.

### Exit gate

`zsh -ic 'mkp'`, then `zsh -ic 'mkt'`, then `zsh -ic 'mku <base>'` for
each base in 2.4. `test_17` was not part of this phase.

### Status

**Complete** 2026-10-05. `mkp` passed (2,200 files) after a cppcheck
`variableScope` / `unreadVariable` fix on the connect `fd` and the
reset loop index. `mkt` passed (4m 51s, shutdown test passed, 346
dead functions, no `nats_` symbol). The seven Unity bases passed:
`config_nats_test_nats_server_endpoint` (5),
`nats_client_test_nats_session_handshake` (4),
`nats_client_test_nats_parser_feed` (9),
`nats_client_test_nats_client_publish` (4),
`nats_reconnect_test_nats_reconnect_delay_seconds` (2),
`nats_reconnect_test_nats_reconnect_should_retry` (3),
`launch_nats_test_launch_nats_subsystem` (4). `test_17` was not run.

### Implementation note

2026-10-05: `src/nats/nats_client.c`, `nats_reconnect.c`, `nats_frame.c`,
and `nats_io.c`. `launch_nats_subsystem` calls `nats_start`.
`nats_shutdown` wakes the wait, joins the thread, and closes the
socket. `nats_server_endpoint` splits a checked `nats://` URL and
strips IPv6 brackets. The default port is 4222. Unity never opens a
socket: `MockConnection` fails the launch connect, and the protocol
tests install an IO table. cppcheck wanted `fd` declared inside the
address loop and the outbound-reset index declared inside the lock.
Do not publish on the live DOKS broker as a fixture.

### Accomplished in Phase 2

2026-10-05. One plaintext connection and a retry thread. No TLS, no
cache dispatch, no WebSocket relay, no registry, no Lua, no status
HTTP, and no Test 62.

Files in the tree:

- `src/nats/nats.h` — public surface (`nats_start`, `nats_shutdown`,
  `nats_client_publish`, the IO table, the message handler)
- `src/nats/nats_internal.h` — buffer caps and internal prototypes
- `src/nats/nats.c` — lifecycle, link state, `nats_on_msg` (114 lines)
- `src/nats/nats_client.c` — parser, handshake, SUB table, outbound
  ring (745 lines)
- `src/nats/nats_frame.c` — `SUB`, `UNSUB`, `PUB` header, `CONNECT`
- `src/nats/nats_io.c` — TCP connect, read, and write, plus the
  failing mock IO used when `Test.MockConnection` is set
- `src/nats/nats_reconnect.c` — delay ladder and the retry thread
- `src/nats/nats_subject.c` / `nats_subject.h` — from Phase 1,
  `nats_subject_build`
- `src/config/config_nats.c` — `nats_server_endpoint` added beside
  `nats_server_url_ok`
- `src/launch/launch_nats.c` — enabled and valid calls `nats_start`

What that code does:

- Launch returns 1 when the thread starts, including when the first
  connect fails. The link stays `degraded` until `CONNECT`, then
  `up`. Disabled still returns 1 with no thread. Invalid still
  returns 0.
- `nats_shutdown` sets the shutdown flag, wakes the wait, joins,
  closes the socket, and resets the client.
- `CONNECT` sends `verbose:false`, `pedantic:true`, `protocol:1`,
  `lang:"c"`, `version` from `VERSION`, and `no_echo:true`. `name`,
  `user`, and `pass` are omitted when those strings are empty. The
  JSON is not logged.
- `tls_required`, or `auth_required` with no username, skips
  `CONNECT`. INFO flags that are absent mean false.
- Every handshake rebuilds `SUB` lines from config.
  `nats_subject_build` turns each suffix into
  `cluster.<ClusterId>.<suffix>`. A queue-group entry sends
  `QueueGroup` as the queue token. The sid is the index plus 1.
- The reader answers `PING` with `PONG`. `-ERR` and an unknown op
  (`HMSG`) fail the session. A `MSG` body is the declared byte count
  plus a trailing CRLF. One read may hold several ops or a split body.
- The outbound ring has 8 slots (`NATS_OUTBOUND_SLOTS`). It survives
  a dropped connection. `nats_client_reset` clears it, and that runs
  on start and after shutdown joins, not on a failed handshake.
  Publish while the link is not `up` returns 0 and stays queued.
  Publish while `up` flushes. A write failure pushes the same copies
  back to the head.
- `nats_on_msg` logs the subject and the length at TRACE. It does not
  read the payload and does not dispatch.
- Port 6222 is rejected by `nats_server_url_ok`, again by
  `nats_server_endpoint`, and again before `connect`. A host with no
  port defaults to 4222. IPv6 brackets are stripped.
- Unity never dials. Protocol tests install an `NatsIo` table. The
  launch test sets `Test.MockConnection`.

Gate, after the cppcheck fix below: `mkp` clean on 2,200 files.
`mkt` passed in 4m 51s (shutdown test passed, 346 dead functions, no
`nats_` symbol). Unity: `config_nats_test_nats_server_endpoint` (5),
`nats_client_test_nats_session_handshake` (4),
`nats_client_test_nats_parser_feed` (9),
`nats_client_test_nats_client_publish` (4),
`nats_reconnect_test_nats_reconnect_delay_seconds` (2),
`nats_reconnect_test_nats_reconnect_should_retry` (3),
`launch_nats_test_launch_nats_subsystem` (4). `test_17` was not run.

### Lessons learned (Phase 2)

- cppcheck `variableScope` is not suppressed. The comment in
  `.lintignore-c` says it is. Declare a local in the block that uses
  it. An initializer that the next statement always overwrites is
  `unreadVariable`. Phase 1 hit this on `servers` and `events`. Phase
  2 hit it on the connect `fd` and on the reset loop index. The first
  `mkp` failed on those four findings. The second was clean.
- `-Werror=format-truncation` fires when `snprintf` copies a 256-byte
  token into the 32-byte sid buffer (`NATS_SID_CAP`). Check the
  length, then `memcpy`. A sid of 32 or more is a protocol error.
- The dead-code gate is `-O0` with `--gc-sections`, rooted at
  `main()`. A function that only Unity calls is still dead there.
  Production code has to call it or store its address.
  `nats_start` stores `nats_client_publish` in `nats_publish_fn` and
  logs `nats_link_state_name()`. `nats_io_use_config` takes the mock
  IO addresses on every call, including when `MockConnection` is
  false. `nats_session_handshake_try` calls `nats_server_endpoint`.
  `nats_on_msg` is the handler installed at start.
- New `src/` and Unity `.c` files show up only after `mkt`
  reconfigures. `mkq` keeps the previous file list. The gate order
  for this phase was `mkp`, then `mkt`, then `mku <base>`.
- Unity builds `src/nats` with `-Dlog_this=mock_log_this`. Assert with
  `mock_logging.h`. `log_get_messages` does not see those lines.
  Phase 1's first publish of the password test failed for this reason.
- `TEST_ASSERT` longjmps out of the test. A stack `AppConfig` is gone
  in `tearDown` while the retry thread is still running. The launch
  test keeps `launch_cfg` at file scope and calls `nats_shutdown` in
  `tearDown` before it frees the config. The test body does not free
  that config.
- The reconnect wait is `pthread_cond_timedwait` on `CLOCK_REALTIME`.
  Shutdown broadcasts it. Without that wake the launch test sits on
  the 30 second first delay. `nats_start` clears
  `nats_system_shutdown` because setUp and tearDown set the flag.
  `add_service_thread` runs on the parent while `nats_life_mu` is
  held, so `thread_count` is already 1 when launch returns.
- Do not hold `nats_client_mu` across `nats_io_write`. `nats_on_msg`
  must not take that lock. Phase 4's handler has the same constraint.
  A lock failure after flush has popped a slot can leave
  `nats_flush_busy` set and free the copies. That path is rare. Do
  not "fix" it by clearing the outbound ring on handshake failure.
- A clean drop (`nats_session_once` returns 1) waits
  `nats_reconnect_delay_seconds(1)` and does not increment the
  failure count. A handshake failure (`-1`) does. `MaxRetries < 0`
  retries forever. `MaxRetries >= 0` stops once
  `failure_number > MaxRetries`, so `0` is one attempt and no retry.
- `no_echo` is true, so this process does not receive its own `PUB`
  as a `MSG`. Local work cannot wait for that echo. Phase 5 still
  evicts before publish. Phase 4's skip-self check is for peers, and
  for a server that echoes anyway.
- The older sketch called the good link `healthy`. The launch section
  and the code use `up`. `nats_link_state_name` returns `down`,
  `degraded`, or `up`.
- Do not log the `CONNECT` line. The password is in that JSON when it
  is set. Do not log a `MSG` payload. Trace logs the subject and the
  length.
- A bad `MSG` trailer is a protocol error only once all `size + 2`
  bytes are present. A short buffer returns 0, which means "need
  more". The parser test uses `MSG demo 1 1\r\nX\nZ` for that case.
- Handshake does not flush the outbound ring, and it does not drop
  it. A test that publishes while the link is down, then completes a
  handshake, still has to call `nats_client_flush_outbound` (or enter
  the read loop) before the `PUB` bytes appear.
- `nats-server` is not on `PATH` (checked 2026-10-05). The DOKS pod
  IPs change. Do not put them in a test, and do not publish on that
  shared broker as a fixture. Phase 11 is a local `nats-server` on
  port 5620.

### Handoff for Phase 3

Phase 3 adds one JSON envelope and `nats_broadcast()`. The byte path
is already there. This phase does not parse an incoming envelope,
evict `query_result_cache`, keep a WebSocket session list, track
peers, or register Lua. Those are Phases 4, 5, 6, 7, and 8.

Publish through the function that exists:

`nats_client_publish(const char *subject, const void *data, size_t len)`

`subject` is the on-wire name. Build it with
`nats_subject_build(ClusterId, suffix)`, which returns a malloc'd
`cluster.<ClusterId>.<suffix>` or NULL. The frame is
`PUB <subject> <size>\r\n`, then the raw body, then `\r\n`
(`nats_frame_pub_header` in `nats_frame.c`). Return values:

- `0` — queued, because the link is not `up`
- the flush result — the link is `up` (0 when the bytes were written)
- `-1` — bad subject token, subject length of 256 or more, `len`
  above `max_payload`, the 8-slot ring is full, or
  `Test.FailNextPublishOnLaunch` was set (that flag fails once and
  does not enqueue)

Put `nats_broadcast` in a new `src/nats/nats_publish.c`. Leave it out
of `nats_client.c`. That file is 745 lines, and Test 99 rejects a
`.c` file past 1000 lines. Do not add `nats_queue.c`. The ring is the
`nats_out` array in `nats_client.c`. `nats_start` has to call
`nats_broadcast` or store its address, or the dead-code gate will
drop it. `nats_publish_fn` is typed as `nats_client_publish`
(`const char *`, `const void *`, `size_t`). Leave that assignment.
Take the address of `nats_broadcast` in a second pointer whose type
matches `nats_broadcast`, and have `nats_broadcast` call
`nats_client_publish`. Both then stay reachable from `main()`.

One call is one envelope and one `PUB`. The loop over several
QueryRefs, and the local evict before publish, belong to Phase 5.
Map the v1 event `cache.invalidate_by_ref` to the suffix
`cache.invalidate`. Reject a suffix that already starts with
`cluster.`. Read `ClusterId` and `InstanceId` from
`app_config->nats`. An explicit `""` in JSON stays empty. The
hostname default is applied only when the field was never set.

The envelope is the object in [Message Model](#message-model):
`event`, `subject` (on-wire name), `timestamp` (ISO-8601 UTC),
`source`, `instance_id`, and `data` (`database`, `query_ref`,
`reason` for invalidation). Build it with jansson and `JSON_COMPACT`,
the same way `nats_connect_send` builds `CONNECT`. Do not log that
JSON. `max_payload` stays 1 MiB until INFO sets it, with an 8 MiB
ceiling. A full ring returns -1. Do not raise `NATS_OUTBOUND_SLOTS`
in this phase.

`no_echo` is already on. `nats_broadcast` is finished when the bytes
are queued or written. It does not wait for a `MSG`. While the link
is `degraded`, a 0 return means queued, not acknowledged by a peer.
Leave `H.nats.broadcast_sync` for Phase 8.

Leave `nats_on_msg` as the TRACE log. Phase 4 is the handler that
parses the envelope. That handler must not take `nats_client_mu`.

Unity file: `nats_publish_test_nats_broadcast.c`, one fake `NatsIo`,
no dial. Cover the envelope fields, the `PUB` line, a down link that
only queues, and `FailNextPublishOnLaunch`. Pass `NULL` as the
jansson error argument so cppcheck does not flag an unread
`json_error_t`. If a test starts the retry thread, shut it down in
`tearDown` before freeing config, and keep that `AppConfig` at file
scope.

Exit gate shape: `zsh -ic 'mkp'`, then `zsh -ic 'mkt'`, then
`zsh -ic 'mku nats_publish_test_nats_broadcast'`. A new `.c` file
needs `mkt` before `mku`. Leave the Phase 3 boxes open until those
commands pass.

Names from the Phase 0 sketch that are not in the tree:
`nats_init`, `nats_get_status`, `nats_build_subject`,
`nats_validate_subject_name`, `nats_queue_push`. The live names are
`nats_start`, `nats_shutdown`, `nats_link_state_name`,
`nats_subject_build`, and `nats_client_publish`.

---

## Phase 3 — Publish path (broadcast)

### Goal

One JSON envelope and `nats_broadcast()`. The call maps
`cache.invalidate_by_ref` to the suffix `cache.invalidate`, builds the
[Message Model](#message-model) object, and hands the bytes to
`nats_client_publish`. No MSG parse, no result-cache eviction, no
WebSocket relay, no peer table, no Lua, and no new socket.

### Dependencies

Phase 2 complete.

### Entry gate

Phase 2 Status is complete. `mkp`, `mkt`, and the seven Phase 2 Unity
bases passed on 2026-10-05.

### Work items

- [x] 3.1 `src/nats/nats_publish.c`. `nats_broadcast(event, data)`
      accepts only `cache.invalidate_by_ref`. The on-wire suffix is
      `cache.invalidate`. An event or suffix that starts with
      `cluster.` returns -1. `data` must be an object with string
      `database` (non-empty), integer `query_ref`, and string
      `reason`. The envelope fields are `event`, `subject` (on-wire
      name), `timestamp` (ISO-8601 UTC), `source`, `instance_id`, and
      `data`. `source` and `instance_id` are `app_config->nats.InstanceId`.
      An empty string stays empty. Build with jansson and
      `JSON_COMPACT`. Do not log that JSON. Return the
      `nats_client_publish` code. Do not raise `NATS_OUTBOUND_SLOTS`.
- [x] 3.2 `nats_start` stores `&nats_broadcast` in a pointer typed as
      `nats_broadcast`. Leave `nats_publish_fn` assigned to
      `nats_client_publish`. `nats_broadcast` calls
      `nats_client_publish`.
- [x] 3.3 Unity `nats_publish_test_nats_broadcast.c`. One fake
      `NatsIo`. No dial and no retry thread. Cover the envelope
      fields, the `PUB` line, a down link that only queues,
      `FailNextPublishOnLaunch`, a `cluster.` event, an empty
      `InstanceId`, and bad `data`. `json_loads` takes a NULL error
      argument.
- [x] 3.4 Exit gate below. A new `.c` file is invisible to `mkq`.
      Leave these boxes open until the commands pass.

### Done means

`mkp` is clean. `mkt` is green and the dead-code list has no new NATS
symbol. `nats_publish_test_nats_broadcast` passes. No live
`nats-server` is required.

### Exit gate

`zsh -ic 'mkp'`, then `zsh -ic 'mkt'`, then
`zsh -ic 'mku nats_publish_test_nats_broadcast'`. `test_17` is not
part of this phase.

### Status

**Complete** 2026-10-05. `mkp` passed (2,202 files). `mkt` passed
(2m 41s, shutdown test passed, 346 dead functions, no `nats_`
symbol). `mku nats_publish_test_nats_broadcast` passed (6). `test_17`
was not run.

### Implementation note

2026-10-05: `src/nats/nats_publish.c` (127 lines).
`nats_broadcast(const char *event, const json_t *data)` builds one
compact JSON object and calls `nats_client_publish`. `nats_start`
stores that function in `nats_broadcast_fn` and still stores
`nats_client_publish` in `nats_publish_fn`. `nats_client.c` stayed
at 745 lines. The Unity test uses a fake `NatsIo` and does not start
the retry thread.

### Accomplished in Phase 3

2026-10-05. One JSON envelope and one `PUB`. No MSG parse, no
result-cache eviction, no WebSocket relay, no peer table, no Lua,
and no new socket.

Files:

- `src/nats/nats_publish.c` — `nats_broadcast`,
  `nats_broadcast_suffix`, `nats_broadcast_data_ok`,
  `nats_broadcast_timestamp` (127 lines)
- `src/nats/nats.h` — public prototype
  `nats_broadcast(const char *event, const json_t *data)`
- `src/nats/nats_internal.h` — prototypes for the three helpers
- `src/nats/nats.c` — `nats_start` stores `nats_broadcast` (116 lines)
- `tests/unity/src/nats/nats_publish_test_nats_broadcast.c` — six
  cases, fake socket

What that code does:

- The only accepted event is `cache.invalidate_by_ref`. The suffix
  is `cache.invalidate`. `nats_subject_build` turns that into
  `cluster.<ClusterId>.cache.invalidate`.
- An empty event, any other event, or a name that starts with
  `cluster.` returns -1 and does not queue. `app_state` is not
  mapped here.
- `data` must be an object with a non-empty string `database`, an
  integer `query_ref`, and a string `reason`. The envelope copies
  those three fields and drops anything else.
- Envelope fields, in order: `event`, `subject` (the on-wire name),
  `timestamp`, `source`, `instance_id`, `data`. `timestamp` is
  `YYYY-MM-DDTHH:MM:SSZ` from `gmtime_r`. `source` and `instance_id`
  are `app_config->nats.InstanceId`. NULL becomes `""`. An explicit
  empty string stays empty. The publisher does not call
  `gethostname`.
- The JSON is `json_dumps` with `JSON_COMPACT`. It is not logged.
- Return values are `nats_client_publish`'s: 0 when the link is not
  `up` (queued), the flush result when the link is `up`, and -1 for
  a bad subject, a payload over `max_payload`, a full 8-slot ring,
  or `Test.FailNextPublishOnLaunch`. `NATS_OUTBOUND_SLOTS` was not
  raised.
- The call is finished when the bytes are queued or written. It
  does not wait for a `MSG`. `no_echo` is already on.

Gate: `mkp` clean on 2,202 files. `mkt` passed in 2m 41s (shutdown
test passed, 346 dead functions, no `nats_` symbol). Unity:
`nats_publish_test_nats_broadcast` (6). `test_17` was not run.

### Lessons learned (Phase 3)

- The first `mkp` was clean. Locals are declared in the function
  that uses them. In the Unity test, `char *end` is not initialized
  before `strtoul` writes it. An initializer that the next call
  always overwrites is `unreadVariable`.
- The helpers are not `static`. Their prototypes are in
  `nats_internal.h`. A new `static` function fails `mkt`. cppcheck
  did not ask to make them static.
- The dead-code gate stayed at 346 functions and listed no `nats_`
  symbol. `nats_start` stores `nats_broadcast` in a pointer whose
  type matches `nats_broadcast`, and it still stores
  `nats_client_publish` in `nats_publish_fn`. `nats_broadcast`
  calls `nats_client_publish`, so both stay reachable from `main()`.
- `mkt` was 2m 41s and built the Unity binary. `mku` then reported
  `ninja: no work to do` and ran the six tests. A new `.c` file
  still needs that `mkt` before `mku`. `mkq` would not have seen it.
- `format_iso_time` uses `gmtime`. The envelope uses `gmtime_r`.
- Do not log the envelope. Unity builds `src/nats` with
  `-Dlog_this=mock_log_this`. The test includes `mock_logging.h`
  and checks that the log does not contain the database name or
  the event.
- The test does not call `nats_start`, so it does not need
  `nats_shutdown`. `AppConfig` is file scope. `json_t` values are
  released in `tearDown` because `TEST_ASSERT` longjmps.
- `json_loads` takes NULL as the error argument. An unread
  `json_error_t` fails cppcheck.
- `nats_client.c` stayed at 745 lines. Test 99 rejects a `.c` file
  past 1000 lines. The envelope lives in `nats_publish.c`.

### Handoff for Phase 4

Phase 4 parses one incoming envelope and dispatches. It does not
create `query_result_cache_invalidate_template`, delete result-cache
rows, relay WebSocket, track peers, or register Lua. Those are
Phases 5, 6, 7, and 8.

The handler today is `nats_on_msg` in `src/nats/nats.c`. `nats_start`
installs it. It logs the subject and the length at TRACE and does
not read the payload. Phase 4 replaces that behavior. The handler
must not take `nats_client_mu`. Do not log the payload.

`no_echo` is on, so this process does not receive its own `PUB`.
Skip-self compares `instance_id` with `app_config->nats.InstanceId`
for a peer, and for a server that echoes anyway. Phase 5 is the
local delete, in the publisher, before `nats_broadcast` returns.
Phase 4 does not delete cache rows.

The bytes Phase 3 writes are `PUB <subject> <size>\r\n`, then
compact JSON, then `\r\n`. The object is:

- `event`: `cache.invalidate_by_ref`
- `subject`: `cluster.<ClusterId>.cache.invalidate`
- `timestamp`: `YYYY-MM-DDTHH:MM:SSZ`
- `source` and `instance_id`: the configured `InstanceId`, which
  may be `""`
- `data.database`: non-empty string
- `data.query_ref`: a JSON integer
- `data.reason`: a string

`nats_parser_feed` already hands a `MSG` body to `nats_on_msg`.
Tests install a fake `NatsIo`. They do not dial. `json_loads` takes
NULL for the error argument.

Leave `nats_broadcast`'s allow-list as `cache.invalidate_by_ref`
only. Do not map `app_state` in Phase 4. That publish path is
Phase 7. `nats_broadcast_suffix` rejects a name that starts with
`cluster.`.

Do not grow `nats_client.c` (745 lines). `nats.c` is 116 lines. Put
a parser in its own file if it would push either toward the 1000-line
Test 99 cap. Unity files use `<source>_test_<function>.c`.

Exit gate shape: `zsh -ic 'mkp'`, then `zsh -ic 'mkt'`, then
`zsh -ic 'mku <base>'` for each new base. A new `.c` file needs
`mkt` before `mku`. Leave the Phase 4 boxes open until those
commands pass. `test_17` is not part of Phase 4. `nats-server` is
not on `PATH`. Do not publish on the live DOKS broker.

---

## Phase 4 — Subscribe + dispatch

### Goal

Parse one incoming envelope and dispatch `cache.invalidate_by_ref`.
The handler reads the `MSG` body. A peer whose `instance_id` differs
from `InstanceId` reaches `nats_dispatch_invalidate`. That function
traces the event name. It does not delete result-cache rows, relay
WebSocket, track peers, register Lua, or open a socket.

### Dependencies

Phase 3 complete.

### Entry gate

Phase 3 Status is complete. `mkp`, `mkt`, and
`nats_publish_test_nats_broadcast` passed on 2026-10-05.

### Work items

- [x] 4.1 `src/nats/nats_dispatch.c`. `nats_dispatch_message` parses
      the `MSG` body with `json_loadb` and a NULL error argument.
      The body is not a C string. The only accepted event is
      `cache.invalidate_by_ref`. The wire subject and the envelope
      `subject` must both be
      `cluster.<ClusterId>.cache.invalidate`. `data` must pass
      `nats_broadcast_data_ok`. `timestamp` is a non-empty string.
      `source` and `instance_id` are strings. An empty `instance_id`
      matches an empty `InstanceId`. NULL `InstanceId` is `""`.
      Skip-self compares those two strings. Do not log the payload,
      the database name, `query_ref`, or `reason`. Do not take
      `nats_client_mu`. Do not map `app_state`.
- [x] 4.2 `nats_on_msg` still traces the subject and the length, then
      calls `nats_dispatch_message`. `nats_start` stores
      `nats_dispatch_message` and installs `nats_dispatch_invalidate`.
      The invalidate function traces the event name and discards
      its arguments. Pointers passed into it are valid only for
      that call.
- [x] 4.3 Unity `nats_dispatch_test_nats_dispatch_message.c`. One
      fake `NatsIo`. No dial and no retry thread. Cover a peer
      dispatch, skip-self, an empty `InstanceId`, bad JSON (the
      parser stays 0), `app_state`, bad `data`, a subject mismatch,
      and a log that omits the database name. `json_loadb` takes a
      NULL error argument.
- [x] 4.4 Exit gate below. A new `.c` file is invisible to `mkq`.
      Leave these boxes open until the commands pass.

### Done means

`mkp` is clean. `mkt` is green and the dead-code list has no new NATS
symbol. `nats_dispatch_test_nats_dispatch_message` passes. No live
`nats-server` is required. No result-cache row is deleted.

### Exit gate

`zsh -ic 'mkp'`, then `zsh -ic 'mkt'`, then
`zsh -ic 'mku nats_dispatch_test_nats_dispatch_message'`. `test_17`
is not part of this phase.

### Status

**Complete** 2026-10-05. `mkp` passed (2,204 files). `mkt` passed
(2m 43s, shutdown test passed, 346 dead functions, no `nats_`
symbol). `mku nats_dispatch_test_nats_dispatch_message` passed (8).
`test_17` was not run.

### Implementation note

2026-10-05: `src/nats/nats_dispatch.c` (133 lines).
`nats_on_msg` traces the subject and the length, then calls
`nats_dispatch_message`. `nats_dispatch_invalidate` traces the event
name. `nats_start` stores `nats_dispatch_message` and installs that
invalidate function. `nats_client.c` stayed at 745 lines. `nats.c`
is 120 lines. The Unity test uses a fake `NatsIo` and does not start
the retry thread.

### Accomplished in Phase 4

2026-10-05. One incoming envelope is parsed and dispatched. No
result-cache eviction, no WebSocket relay, no peer table, no Lua,
and no new socket.

Files:

- `src/nats/nats_dispatch.c` — `nats_dispatch_message`,
  `nats_dispatch_fields_ok`, `nats_dispatch_subject_ok`,
  `nats_dispatch_is_self`, `nats_dispatch_invalidate`,
  `nats_dispatch_set_invalidate` (133 lines)
- `src/nats/nats_internal.h` — prototypes for those functions
- `src/nats/nats.c` — `nats_on_msg` calls the parser;
  `nats_start` stores it (120 lines)
- `tests/unity/src/nats/nats_dispatch_test_nats_dispatch_message.c`
  — eight cases, fake socket

What that code does:

- `nats_parser_feed` still delivers the `MSG` body to `nats_on_msg`.
  The body is `len` bytes and is not NUL-terminated. The parser
  uses `json_loadb` with a NULL error argument.
- The trace line is still the subject and the length. The payload
  is not logged. A dropped envelope logs `NATS envelope dropped`.
  Skip-self logs `NATS skip self`. A peer logs
  `NATS dispatch cache.invalidate_by_ref`.
- Accepted event: `cache.invalidate_by_ref`. The `MSG` subject, the
  envelope `subject`, and
  `cluster.<ClusterId>.cache.invalidate` must be the same string.
  `app_state` is dropped.
- `data` reuses `nats_broadcast_data_ok`: non-empty string
  `database`, integer `query_ref`, string `reason`. `timestamp` is
  a non-empty string. `source` may be empty.
- Skip-self compares `instance_id` with
  `app_config->nats.InstanceId`. NULL becomes `""`. Two empty
  strings match, so the hook is not called.
- `nats_dispatch_invalidate` receives `database`, `query_ref`, and
  `reason`. It traces the event name and discards the arguments.
  The pointers are valid only for that call, before `json_decref`.
- A bad JSON body, a bad `data` object, or a subject mismatch does
  not change the parser result. `nats_parser_feed` stays 0. The
  handler does not take `nats_client_mu`.
- `nats_client.c` stayed at 745 lines. `nats_publish.c` stayed at
  127 lines.

Gate: `mkp` clean on 2,204 files. `mkt` passed in 2m 43s (shutdown
test passed, 346 dead functions, no `nats_` symbol). Unity:
`nats_dispatch_test_nats_dispatch_message` (8). `test_17` was not
run.

### Lessons learned (Phase 4)

- The `MSG` body is not a C string. `json_loads` would read past
  `len`. `json_loadb` with a NULL error argument parses those
  bytes. An unread `json_error_t` fails cppcheck.
- The subject trace includes the wire subject. A test that forbids
  the token `app_state` fails, because the subject
  `cluster.philement.instance.app_state` contains it. Assert the
  payload words (`Acuranzo`, `mutation`) and the dispatch line.
- Bad JSON is an application drop. `nats_parser_feed` returns 0.
  Returning -1 would tear the session down.
- The helpers are not `static`. Their prototypes are in
  `nats_internal.h`. Locals that belong to one branch are declared
  in that block. The first `mkp` was clean (2,204 files).
- The dead-code gate stayed at 346 functions and listed no `nats_`
  symbol. `nats_start` stores `nats_dispatch_message` and passes
  `nats_dispatch_invalidate` to `nats_dispatch_set_invalidate`.
  `nats_on_msg` calls `nats_dispatch_message`.
- `mkt` was 2m 43s and built the Unity binary. `mku` then reported
  `ninja: no work to do` and ran the eight tests. A new `.c` file
  still needs that `mkt` before `mku`.
- The test does not call `nats_start`. `AppConfig` is file scope.
  The fake `NatsIo` connect count stays 0. `json_t` is released
  inside the parser. The test frees the frame in `tearDown`
  because `TEST_ASSERT` longjmps.
- `nats_client.c` stayed at 745 lines. The parser lives in
  `nats_dispatch.c` (133 lines). `nats.c` is 120 lines.

### Handoff for Phase 5

Phase 5 deletes result-cache rows for one SQL template. It does not
relay WebSocket, track peers, register Lua, or map `app_state`.

Add `query_result_cache_invalidate_template`. It drops every
parameter variant of one template in one database. The global cache
is `query_result_cache_get_global`. Keys are the database name, the
SHA-256 of `QueryCacheEntry.sql_template`, and the parameter hash.
Do not call `query_result_cache_clear` or `query_cache_clear`.

The template catalog is `DatabaseQueue.query_cache`.
`global_queue_manager` and
`database_queue_manager_get_database(manager, name)` find the queue
for `data.database`. `query_cache_lookup(cache, query_ref, SR_NATS)`
returns the entry. `query_ref` arrives as `json_int_t`. The lookup
takes `int`. Drop the message when the value does not fit in `int`.

Call that delete from both sides:

- `nats_dispatch_invalidate` does it for a peer. The `database` and
  `reason` pointers are valid only for that call. Copy them if the
  work outlives the call. Do not take `nats_client_mu`. Do not log
  the payload, the database name, `query_ref`, or `reason`.
- `nats_broadcast` does the same delete before
  `nats_client_publish` returns. Local evict does not wait for a
  `MSG`. `no_echo` is already on.

Leave the publish allow-list as `cache.invalidate_by_ref`. Do not
grow `nats_client.c` (745 lines). `nats_dispatch.c` is 133 lines.
`nats.c` is 120 lines. `nats_publish.c` is 127 lines.

Exit gate shape: `zsh -ic 'mkp'`, then `zsh -ic 'mkt'`, then
`zsh -ic 'mku <base>'` for each new base. A new `.c` file needs
`mkt` before `mku`. Leave the Phase 5 boxes open until those
commands pass. `test_17` is not part of Phase 5. `nats-server` is
not on `PATH`. Do not publish on the live DOKS broker.

---

## Phase 5 — Result-cache invalidation

### Goal

Drop every parameter variant of one SQL template in one database.
`nats_broadcast` does that delete before `nats_client_publish`.
`nats_dispatch_invalidate` does it for a peer. No WebSocket relay, no
peer table, no Lua, and no `app_state` handling.

### Dependencies

Phase 4 complete.

### Entry gate

Phase 4 Status is complete. `mkp`, `mkt`, and
`nats_dispatch_test_nats_dispatch_message` passed on 2026-10-05.

### Work items

- [ ] 5.1 `query_result_cache_invalidate_template(cache, database, sql)`
      walks that cache under its own lock and drops every parameter
      variant of one template in one database. The cache pointer matches
      `query_result_cache_get` and `query_result_cache_put`. Callers pass
      `query_result_cache_get_global()`. A NULL cache or a NULL template
      removes nothing. NULL `database` matches rows stored with a NULL
      database name. The return value is the number of rows removed.
      Keys are `database:template_hash:param_hash`. The two hashes do
      not contain `:`. The database name may. Do not call
      `query_result_cache_clear` or `query_cache_clear`.
- [ ] 5.2 `nats_invalidate_query_ref` finds the database on
      `global_queue_manager` and takes the first
      `query_cache_lookup` row for that ref. It copies
      `sql_template` for the call. `query_ref` outside `int` makes
      `nats_broadcast_data_ok` return false, so `nats_broadcast`
      returns -1 and does not publish or delete. A missing database
      or a missing ref deletes nothing. `nats_broadcast` still
      publishes in that case. Do not log the payload, the database
      name, `query_ref`, or `reason`. Do not take `nats_client_mu`.
- [ ] 5.3 `nats_dispatch_invalidate` calls `nats_invalidate_query_ref`,
      then keeps the trace line `NATS dispatch cache.invalidate_by_ref`.
      `nats_broadcast` calls the helper after the envelope bytes exist
      and before `nats_client_publish`. A bad event, bad `data`, or a
      failed envelope returns -1 with no delete. Do not grow
      `nats_client.c` (745 lines). `nats.c` stays 120 lines.
- [ ] 5.4 Unity. New file
      `query_result_cache_test_query_result_cache_invalidate_template`.
      Extend `nats_publish_test_nats_broadcast` and
      `nats_dispatch_test_nats_dispatch_message`. Cover two parameter
      variants, a different template, another database, a database
      name that contains `:`, a missing ref that still publishes, a
      `query_ref` outside `int` that does not publish, the first QTC
      row when two rows share a ref, a peer delete, and skip-self
      leaving rows in place. Logs still omit the database name and
      `reason`. No dial and no retry thread.
- [ ] 5.5 Exit gate below. A new `.c` file is invisible to `mkq`.
      Leave these boxes open until the commands pass.

### Done means

`mkp` is clean. `mkt` is green and the dead-code list has no new NATS
symbol. The three Unity bases pass. No live `nats-server` is required.
`query_result_cache_clear` and `query_cache_clear` are not the
per-template path.

### Exit gate

`zsh -ic 'mkp'`, then `zsh -ic 'mkt'`, then
`zsh -ic 'mku query_result_cache_test_query_result_cache_invalidate_template'`,
`zsh -ic 'mku nats_publish_test_nats_broadcast'`, and
`zsh -ic 'mku nats_dispatch_test_nats_dispatch_message'`. `test_17`
is not part of this phase.

### Status

**In progress** 2026-10-05. The code is in the tree. The exit gate
has not been run.

### Implementation note

2026-10-05: `query_result_cache_invalidate_template` walks every bucket
and unlinks matching keys. `nats_invalidate_query_ref` lives in
`src/nats/nats_dispatch.c` (180 lines) and is called from
`nats_dispatch_invalidate` and from `nats_broadcast`
(`src/nats/nats_publish.c`, 140 lines) before
`nats_client_publish`. `nats_client.c` stayed at 745 lines. `nats.c`
stayed at 120 lines. `query_result_cache.c` is 536 lines. A `query_ref`
that does not fit in `int` fails `nats_broadcast_data_ok`.

---

## Reference: how a recently-added subsystem was integrated

For exact patterns to mirror, study these files (they represent the most recent
subsystem additions):

- `src/config/config_mcp.h` + `config_mcp.c` — config struct + load/dump/cleanup
- `src/launch/launch_mcp.c` — readiness clean-skip pattern + launch dispatch
- `src/landing/landing_mcp.c` — landing readiness + land function
- `src/mcp/mcp.h` — minimal public API surface
- `src/registry/registry_integration.h` — `extern ServiceThreads mcp_threads;`
- `src/threads/threads.h` — `extern ServiceThreads mcp_threads;`
- `src/state/state.h` — `extern volatile sig_atomic_t mcp_system_shutdown;`
- `src/launch/launch_readiness.c` — last `process_subsystem_readiness` call is MCP. NATS is the next call
- `src/launch/launch.c` — `launch_mcp_subsystem` branch in `launch_approved_subsystems`
- `src/landing/landing_readiness.c` — shutdown table, first entry first. Insert NATS before Print. MCP is near the end, after Threads, with Scripting and Reporting after it
- `src/landing/landing.c` — `get_landing_function` maps `SR_MCP` to `land_mcp_subsystem`. The comment says reverse launch order; the walk is the readiness-table order
- `src/landing/landing_plan.c` — `expected_order[]` is the Go/No-Go log only. Add `SR_NATS` before `SR_PRINT`. Do not rebuild the list
- `src/status/status_core.h` — `ServiceMetrics mcp` member of `SystemMetrics`, plus a `specific.mcp` union arm
- `src/config/config_chat.h` — config-only subsystem (no launch) pattern

---

## Working Log

Resume from the latest entry. Phase status, lessons, and the next-phase
notes stay in the phase sections above.

### 2026-10-05 — Phase 0 approved

Locks signed off. No `src/` edits in that turn. Letter V, launch 22,
landing before Print, plaintext client, `no_echo`, subject suffixes.

### 2026-10-05 — Phase 1 complete

Config, launch, landing, and `nats_subject_build`. Gate numbers are in
the Phase 1 status block. cppcheck `variableScope` is not suppressed.

### 2026-10-05 — Phase 2 complete

Plaintext client, retry thread, and fake-socket Unity tests. Gate
numbers are in the Phase 2 status block. What landed, what bit the
build, and how to start Phase 3:

- [Accomplished in Phase 2](#accomplished-in-phase-2)
- [Lessons learned (Phase 2)](#lessons-learned-phase-2)
- [Handoff for Phase 3](#handoff-for-phase-3)

Phase 3 has not started. Do not dispatch, evict, or open a socket in
that phase.

### 2026-10-05 — Phase 3 complete

JSON envelope and `nats_broadcast()`. Gate numbers are in the Phase 3
status block. What landed, what the build did, and how to start
Phase 4:

- [Accomplished in Phase 3](#accomplished-in-phase-3)
- [Lessons learned (Phase 3)](#lessons-learned-phase-3)
- [Handoff for Phase 4](#handoff-for-phase-4)

Phase 4 has not started. Do not evict, relay, or open a socket in
that phase.

### 2026-10-05 — Phase 4 complete

Incoming envelope parse and dispatch. Gate numbers are in the Phase 4
status block. What landed, what the build did, and how to start
Phase 5:

- [Accomplished in Phase 4](#accomplished-in-phase-4)
- [Lessons learned (Phase 4)](#lessons-learned-phase-4)
- [Handoff for Phase 5](#handoff-for-phase-5)

Phase 5 has not started. Do not relay WebSocket, track peers, or
register Lua in that phase.

### 2026-10-05 — Phase 5 code in the tree

Result-cache delete for one SQL template, from `nats_broadcast` and
from `nats_dispatch_invalidate`. The exit gate has not been run.
Boxes in [Phase 5](#phase-5--result-cache-invalidation) stay open
until `mkp`, `mkt`, and the three `mku` bases pass. Do not start
Phase 6 in that turn.
