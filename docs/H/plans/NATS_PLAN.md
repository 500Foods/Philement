# NATS Subsystem Plan

## Phases

Each phase is self-contained with its own exit gate (V/Val/C) and is documented in detail in the [Phased Breakdown](#proposed-phased-breakdown) below. This table summarizes the individual phases:

| Phase | Focus | Status | Effort |
| --- | --- | --- | --- |
| 0 | Design approval — confirm config letter V, launch 22, message model, open questions | **Not approved** — awaiting sign-off | Easy |
| 1 | Config + launch + landing wiring | Not started | Medium |
| 2 | NATS connection + lifecycle (custom client) | Not started | Hard |
| 3 | Publish path (broadcast) | Not started | Medium |
| 4 | Subscribe + dispatch | Not started | Hard |
| 5 | Cache invalidation hooks | Not started | Hard |
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
| 0 | **Not approved** — design review pending | 2026-10-04 | Awaiting sign-off on config letter V, launch position 22, and message model. |

## Purpose

Draft an implementation plan for a **NATS subsystem** in Hydrogen that serves as
a lightweight, cross-instance communication backchannel, consumed by Hydrogen
instances running in a Kubernetes cluster (DOKS) alongside a managed NATS server.

The original motivation is **cache invalidation without database triggers**.
Database triggers are notoriously difficult to build cross-engine (PostgreSQL,
MySQL/MariaDB, SQLite, DB2, Firebird, YugabyteDB, MSSQL) and painful to manage
during schema evolution. Instead, when a query executes that mutates cached state,
Hydrogen broadcasts a NATS message identifying what cached data is now stale.
Peer instances receive the message and refresh their local caches. The same
messages can also be forwarded to connected WebSocket clients so front-end
applications receive live notifications (e.g., "your JWT token table was
refreshed", "order 1234 was updated").

This document is a **planning draft**. It describes the subsystem concept, the
integration points, the message model, and the channel-coordination strategy.
It is **not** a phased implementation plan with work items yet — those follow
once the approach is approved.

## How To Use This Document

1. Review the design locks and open questions.
2. Confirm the config letter, launch position, and message model.
3. Once approved, convert into a phased plan (Phases 0–N) following the
    template of [`NOTIFICATIONS_PLAN.md`](/docs/H/plans/NOTIFICATIONS_PLAN.md)
    and [`INSTRUCTIONS.md`](/docs/H/INSTRUCTIONS.md).

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
| `MAX_SUBSYSTEMS` | **24** / `INITIAL_REGISTRY_CAPACITY` **24**. Adding NATS as 22nd = **no bump required.** | `src/globals.h` |
| Source glob | `file(GLOB_RECURSE HYDROGEN_SOURCES "../src/*.c")` — new `.c` files in `src/` are picked up automatically. | `cmake/CMakeLists-init.cmake:147` |
| Query cache | Per-DQM `QueryTableCache` in `src/database/database_cache.h`. Currently in-memory only; no cross-instance invalidation. `timeout_seconds` field exists in `QueryCacheEntry` but auto-refresh was never implemented — NATS invalidation replaces this missing behavior. | `src/database/database_cache.h` |
| WebSocket broadcast | libwebsockets context in `src/websocket/websocket_server_internal.h`. Connected sessions tracked per-vhost, each with a `subscribed_events` list (events the client declared interest in). No existing "push to all clients" helper — each subsystem sends to specific sessions. **New `ws_broadcast_json(event_type, payload)` required** (iterates all sessions, filters by `subscribed_events` membership, writes JSON text frame). | `src/websocket/` |
| API service | `api_service.c` mounts REST handlers; `json_endpoints` for body parsing; `extract_and_validate_jwt` for auth. Mail Relay checks JWT inside handlers (not via `protected_endpoints` middleware). | `src/api/` |
| Status metrics | `ServiceMetrics` union in `src/status/status_core.h`. MCP pattern added as a union arm. QueueMetrics per-subsystem. | `src/status/status_core.h` |
| Landing dispatch | `landing_readiness.c` uses a table in reverse launch order; `landing.c` `get_landing_function()` maps name → `land_*_subsystem()`. | `src/landing/` |
| Env var substitution | `${env.NAME}` resolved in `src/config/config.c:load_config`. | `config.c` |
| Blackbox slots | **62 is free** (NOTIFICATIONS plan reserved it; since that plan is unapproved, reclaim it here or pick another). NATS takes 62; NOTIFICATIONS shifts to **63**. | `tests/test_*.sh` |
| Helium | Last `acuranzo_1377.lua`, QueryRef **#154**. | `elements/002-helium/acuranzo/migrations/` |
| INSTRUCTIONS.md | Stale: letters end at T, launch order ends at 21 MCP, **U. Chat missing**. | [`INSTRUCTIONS.md`](/docs/H/INSTRUCTIONS.md) |
| `landing_plan.c` | `expected_order[]` is stale (missing Scripting/Reporting/MCP). Do **not** rewrite the whole list. | `src/landing/landing_plan.c` |

### Note on the NOTIFICATIONS_PLAN.md reservation

[`NOTIFICATIONS_PLAN.md`](/docs/H/plans/NOTIFICATIONS_PLAN.md) proposed
config letter **V** and launch position **22** for a `Subscribers`
subsystem (Web Push). That plan is **Phase 0 — not approved** and no code
exists for it.

**NATS takes priority.** NATS holds **V / 22 / 62**. NOTIFICATIONS
shifts to **W / 23 / 63** (ports 563x). Since Subscribers was never
committed to source, there is no conflict — NATS lands first, then
NOTIFICATIONS moves to W/23/63 when its own plan is approved.

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
4. **Cache invalidation broadcast** — when a mutating query completes (or a
   scripted action runs), publish a NATS message; when messages arrive, refresh
   or clear the relevant in-process cache (query cache, token table, etc.).
5. **WebSocket forwarding** — optionally forward a subset of broadcast messages
   to connected WebSocket clients so UIs get live updates.
6. **Channel coordination** — distinguish cluster-wide subjects from
   instance-group / instance-specific subjects so broadcasts go only where
   intended.

## Non-goals

- Implementing the NATS client library in C. We are rolling our own (Option B) — see [NATS Client Implementation](#nats-client-implementation).
- Auth complexity. This is an internal DOKS cluster — no external exposure. Simple auth only (username/password or none), not full NKEYS/JWT enterprise auth.
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

- No new dependencies (user has implemented mDNS from scratch; prefers no external libs)
- NATS protocol is text frames over TCP — manageable scope
- jansson already available for JSON encoding/decoding
- Network subsystem provides socket/epoll layer — borrow, don't duplicate
- Full control over reconnect, backoff, and auth behavior

Scope of custom client:

- CONNECT handshake (client info + auth)
- PUB/SUB/UNSUB frame encoding/decoding
- MSG delivery with subject matching
- PING/PONG keepalive
- Reconnect with exponential backoff
- TLS support (optional, via existing Network TLS)

Out of scope for the custom client:

- NKEYS/JWT enterprise auth (internal cluster, simple auth sufficient)
- JetStream (not needed for invalidation signals)
- TLS certificate pinning (use system CA store)

The client lives in `src/nats/nats_client.c` with `nats_client.h` public API:

- `nats_client_connect(config)` → connection handle
- `nats_client_publish(conn, subject, payload)` → publish
- `nats_client_subscribe(conn, subject, callback, user_data)` → subscribe
- `nats_client_disconnect(conn)` → graceful shutdown
- `nats_client_ping(conn)` → keepalive

The connection pool and subscription table are in `nats_internal.h`.

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

**Explicit Network dependency.** NATS opens TCP (optionally TLS) connections to
NATS servers, which the **`Network`** subsystem provides. Concrete ordering from
  `src/launch/launch_readiness.c` (launch order) and
  `src/landing/landing_readiness.c` (landing order, adjusted per subsystem lifecycle, not strictly reversed):

- Launch 4 = `Network`; launch 21 = MCP; NATS = launch 22 (last). So NATS
  starts **after Network** — the prerequisite is guaranteed.
- Landing table (`landing_readiness.c`) generally follows reverse-launch order but
  has been adjusted as needed per subsystem lifecycle. NATS lands **before Network**
  (which lands 12th from the end) — see the table insertion point below for exact placement.
- **Non-blocking startup.** If NATS is enabled but the NATS server is unreachable,
  the server starts anyway. The NATS subsystem enters **degraded** mode and
  retries with a cascading backoff: 30s, 60s, 120s, 240s, 480s, then stays at
  480s indefinitely. The readiness check returns "degraded" (not No-Go). If NATS
  becomes available after a retry period, the subsystem transitions to healthy
  and updates its status in the subsystem registry. Nothing in Hydrogen depends
  on NATS — if it never becomes available, the server continues running without
  it.
- The readiness clean-skip when `Enabled: false` must run *after* Network so
  the connect attempt (had it been enabled) would have a working socket layer.

| Must update | File | What |
| --- | --- | --- |
| `SR_NATS` constant | `src/globals.h` | `#define SR_NATS "NATS"` |
| `nats_system_shutdown` | `src/state.h` | `extern volatile sig_atomic_t` |
| `nats_threads` | `src/state.h` + `src/threads/threads.h` | `ServiceThreads` extern + definition in `state.c` |
| Readiness check | `src/launch/launch.h` | `check_nats_launch_readiness(void)` |
| Launch dispatch | `src/launch/launch_readiness.c` | Add `process_subsystem_readiness(..., SR_NATS, check_nats_launch_readiness())` after MCP |
| Launch function | `src/launch/launch.h` | `launch_nats_subsystem(void)` |
| Launch dispatch | `src/launch/launch.c` | Add `strcmp(subsystem, SR_NATS) == 0` branch in `launch_approved_subsystems()` |
| Landing readiness | `src/landing/landing.h` | `check_nats_landing_readiness(void)` |
| Landing readiness table | `src/landing/landing_readiness.c` | Add `{SR_NATS, check_nats_landing_readiness}` to the table (reverse position: after MCP, before Threads) |
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
  nats_internal.h           connection pool, subscription table, shutdown flag, peer registry
  nats.c                    init / shutdown / metrics snapshot / main loop (retry scheduler, state transitions healthy↔degraded)
  nats_client.c             custom NATS client: connect, publish, subscribe, reconnect, ping/pong, TLS
  nats_client.h             public client API
  nats_config.c             NATS connection lifecycle (connect/disconnect/reconnect)
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

Each `.c` begins with `#include <src/hydrogen.h>`. Includes use
`<src/folder/...>`. Every function has a header prototype. **No `static`
functions** in `src/` (build gate fails on new `static` functions). File-scope
state only.

---

## Channel Model

### What to call them: "subjects"

NATS uses the term **subject** for what a message is published to. The user's
question "do we call them channels?" — in NATS terminology the answer is
"subject." We will use "subject" in code/config and document the mapping
("NATS subjects are the logical channels"). In the config, we'll call the
array **Subjects** to match NATS conventions, but document clearly.

### Subject naming convention

We propose a hierarchical subject scheme using `.` as a delimiter (NATS token):

```text
cluster.<cluster_id>.<domain>.action.<resource>
```

Examples:

| Subject | Meaning |
| --- | --- |
| `cluster.philement.query.invalidate.ref.<ref>` | Specific QueryRef (e.g. `127`) cache entry stale; evict + re-fetch |
| `cluster.philement.order.updated` | An order was modified |
| `cluster.philement.order.updated.<order_id>` | Specific order changed (instance-group targeted) |
| `cluster.<cluster_id>.instance.<instance_id>.status` | Per-instance status beacon (for health/coordination) |

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
which uses jansson). Each message has a common envelope:

```json
{
  "event": "cache.invalidate_by_ref",
  "subject": "cluster.philement.query.invalidate.ref.127",
  "timestamp": "2026-10-04T00:00:00Z",
  "source": "hydrogen-01.philement.svc.cluster.local",
  "instance_id": "hydrogen-01",
  "data": {
    "query_ref": 127,
    "reason": "mutation"
  }
}
```

### Message types (events)

| Event | When emitted | When received | Purpose |
| --- | --- | --- | --- |
| `cache.invalidate_by_ref` | A specific QueryRef's result cache is stale; emitted per-QueryRef from the `InvalidateQTC` list in the query's collection JSON | `data.query_ref` → evict that cache entry, then re-fetch from DB | Targeted query cache invalidation (QTC refresh) |
| `state.changed` | Any scripted or API state mutation | `data.resource` + `data.id` → mark stale | General state-change notification |
| `order.updated` | An order record changes | `data.order_id` → refresh order cache | Example domain event |
| `instance.heartbeat` | Periodic (configurable interval) | Health/status awareness | Peer liveness, optional |
| `app_state` | Instance publishes state: "Starting", "Alive", "Stopping" | Peers track active instances via heartbeat timeout; "Alive" payload carries active connection count + unique active IPs | Singleton detection, peer count, cluster-wide active client tally |

### Cache invalidation hooks (the core use case)

The plan is to hook into two emission points:

1. **Query execution completion** — when a Conduit query (or any DB operation)
     that is registered as "mutating" succeeds, broadcast a
     `cache.invalidate_by_ref` message. Which queries are
     "mutating" can be determined by:
     - Query metadata flag in the SQL template (e.g., a comment `-- mutating`
       or a QueryRef attribute)
     - A config table in Helium listing "watch these QueryRefs"
     - The scripting layer explicitly calling `H.nats.broadcast(...)`
     - **QTC invalidation list**: the query's `collection` JSON may contain
       `"InvalidateQTC":[14,15]`. When the query executes, Hydrogen broadcasts
       one `cache.invalidate_by_ref` message per QueryRef in the list, each
       on its own subject (`cluster.<cluster_id>.query.invalidate.ref.<ref>`).
       This is the primary mechanism for cross-instance cache consistency.

2. **Scripting layer** — Lua scripts can call `H.nats.broadcast(event, data)`
      to publish arbitrary events (not cache-invalidation-specific), or
      invalidation events via `H.nats.broadcast("cache.invalidate_by_ref", ...)`.
      See [Lua Host API](#lua-host-api) below.

Receiving side: when a `cache.invalidate_by_ref` message
arrives, `nats_dispatch.c` calls into the existing cache management:

- `query_cache_lookup` / `query_cache_clear` in `src/database/database_cache.h`
  to evict the stale `QueryCacheEntry`.

**QTC refresh semantics**: "Invalidate" means "refresh" — the cache entry is
evicted and the next lookup re-fetches from the database. The existing
`timeout_seconds` field in `QueryCacheEntry` was never implemented for
auto-refresh; this NATS mechanism replaces that missing timeout behavior.
If a timeout is later implemented, it would serve as a safety net (catch
missed invalidations) rather than the primary refresh path.

**Originating instance behavior**: When an instance executes a mutating
query with `InvalidateQTC:[14,15]`, it broadcasts invalidation messages
for refs 14, 15 AND updates its own cache immediately (optimistic update).
It does NOT wait for the round-trip message to come back, because
self-suppression means it won't receive its own messages anyway, and
there's no guarantee the round-trip will complete. The broadcast is for
peer instances only.

**App state broadcasting**: Each instance publishes `app_state` with its current state ("Starting", "Alive", "Stopping"). The "Alive" payload includes `active_connections` (count of unique active IPs in the last 5 minutes, covering REST API clients and active WebSocket connections). Instances subscribe to `app_state` and maintain a peer registry: an instance is considered active if it has published an "Alive" state within `HeartbeatIntervalSeconds * 3`. The cluster-wide active instance count and active client count are derived from the aggregated "Alive" payloads. This replaces separate `app.startup`/`app.shutdown` subjects — one-shot events are folded into the state machine, and crash handling is resolved via heartbeat-stale detection (an instance that stops publishing "Alive" is considered dead after the stale window).

---

## Channel Coordination

### Cluster identity

Config provides:

- `NATS.ClusterId` — identifies the DOKS cluster / deployment group.
  Subjects are namespaced by cluster (`cluster.<cluster_id>....`). Instances
  in different clusters never cross-talk.
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

4. **Suppress self-receipt** — NATS does not deliver a message back to the
    connection that published it on the same subject within the same
    subscription (standard NATS behavior with separate publish/subscribe
    connections). The custom client uses separate connections for publish
    and subscribe, so self-suppression is handled by NATS itself. The
    handler still checks `message.source == own instance_id` as a
    safety net (defense in depth) for edge cases where the library might
    echo back.

### Queue groups for cluster-wide single-processing tasks

For events that should trigger **one** action per cluster (not N), use a NATS
queue group. The config specifies queue-group subscriptions:

```json
"QueueSubscriptions": [
  {
    "Subject": "cluster.philement.jobs.refresh-all",
    "QueueGroup": "philement-refresher"
  }
]
```

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
- A dedicated bridge thread (`nats_ws_bridge.c`) subscribes to the
  configured subjects and, for each message, calls the new
  `ws_broadcast_json(event_type, json_payload)` helper. The helper
  iterates all connected libwebsockets sessions on the chat vhost and
  writes the JSON text frame **only to sessions whose `subscribed_events`
  list contains `event_type`**.
- Only **authenticated** WebSocket sessions receive relayed messages (the
  existing `WebSocketSessionData.authenticated` flag is checked).
- **New function required**: `ws_broadcast_json(event_type, payload)` in
  `websocket_server_message.c/h` iterates all connected libwebsockets
  sessions, filters by `subscribed_events` membership, and writes the
  JSON payload as a text frame. This does not exist yet — the current
  API (`ws_write_json_response`, `ws_write_raw_data`) targets a single
  `wsi`. The bridge thread calls `ws_broadcast_json()` rather than
  sending to individual sessions.
- **No direct Lua→WS path.** Lua scripts push to WebSocket clients only
  by publishing a NATS event that is relayed via `WebSocketRelay`. A
  direct `H.ws.broadcast()` API is not in scope — the Scripting config
  linkage (`"NATS Handler":"NATS/intake_handler.lua"`) is the Lua→NATS
  path, and NATS→WS relay handles the rest.

### Message format to clients

The same JSON envelope is sent to WebSocket clients. A `type` field distinguishes
NATS-relayed messages from native WebSocket protocol messages:

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

- Each instance publishes `app_state` on subject `cluster.<cluster_id>.app_state` with payload:
  - `instance_id`
  - `state`: one of `"Starting"`, `"Alive"`, `"Stopping"`
  - When `state == "Alive"`: `active_connections` (count of unique active IPs in last 5 minutes, covering REST API clients and active WebSocket connections)
- Instances subscribe to `cluster.<cluster_id>.app_state` to track all peers.
- An in-memory registry (`nats_registry.c`) maintains the last-seen timestamp
  and the latest `app_state` payload for each peer. Peers are considered stale after
  `HeartbeatIntervalSeconds * 3` with no "Alive" update (configurable via `Presence.StaleAfterSeconds`).
- `H.nats.instances()` Lua function and a `GET /api/nats/instances` endpoint
  (if admin API is added) return the current peer list and singleton status.
- **Singleton detection**: if no other instances are seen as "Alive" for one full
  stale-window after startup, the instance can optionally signal "sole operator"
  — useful for background tasks that should run only once in single-instance
  deployments. This does not gate cache invalidation (that is handled by the
  wildcard subject logic regardless of instance count).
- **Active client tally**: the "Alive" payload's `active_connections` field
  carries the count of unique active IPs in the last 5 minutes. Since all
  instances know the heartbeat timeout, they can compute both the active
  instance count and the cluster-wide active client count from the aggregated
  "Alive" payloads. No separate tally mechanism is needed.

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
| `H.nats.broadcast_sync(event, data)` | Publish and wait for confirmation (optional) |
| `H.nats.subscribe(subject, handler)` | Subscribe to a NATS subject; handler receives parsed JSON envelope (optional — config-driven is the normal path) |
| `H.nats.unsubscribe(subject)` | Remove a runtime subscription |
| `H.nats.status()` | Return NATS connectivity / message counters |
| `H.nats.instances()` | Return live peer instances from app_state registry |

**Subscription is config-driven by default.** The Scripting subsystem maps NATS subjects to Lua handler files via the scripting config:

```json
"Scripting": {
  "Handlers": {
    "NATS": "NATS/intake_handler.lua"
  }
}
```

The Scripting subsystem loads the Lua handler and invokes it when a matching NATS message arrives. The handler receives the parsed JSON envelope. This matches the existing scripting config pattern (e.g., `H.mcp`, `H.mail`).

`H.nats.subscribe()` / `H.nats.unsubscribe()` are available as an optional Lua API for runtime subscriptions that the config cannot anticipate (e.g., dynamic subject interest per instance). The scripting handle kind would need a new constant in `src/scripting/scripting_handle.h` (next free slot, analogous to `H_HK_MCP = 6`).

---

## Updated: Open Question #5 — Heartbeat necessity

Now a **desired feature**: instance presence/group membership is required for
autoscaling awareness and singleton detection. The `instance.heartbeat` subject
is needed. K8s liveness probes cover *crash* detection but not *peer count* —
two Hydrogen pods running means cache invalidation must fan out to both, and a
background job should know it has a peer to avoid duplicate work.

## Config Sketch

```json
"NATS": {
  "Enabled": false,
  "Servers": ["nats://hydrogen-nats:4222"],
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
  "PublishSubjects": {
    "ClusterPrefix": "cluster.philement"
  },
  "Subscriptions": [
    {
      "Subject": "cluster.philement.query.invalidate.ref.+",
      "Type": "cluster-wide"
    },
    {
      "Subject": "cluster.philement.app_state",
      "Type": "cluster-wide"
    },
    {
      "Subject": "cluster.philement.jobs.refresh-all",
      "Type": "queue-group",
      "QueueGroup": "philement-refresh"
    }
  ],
  "WebSocketRelay": {
    "Enabled": true,
    "Events": ["order.updated"]
  },
  "Presence": {
    "Enabled": true,
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
| `TlsEnabled` | bool | false | Use TLS for NATS connection |
| `ConnectionTimeoutSeconds` | int | 10 | Connect timeout |
| `Reconnect.MaxRetries` | int | -1 (unlimited) | -1 = forever |
| `Reconnect.Delays` | int[] | [30, 60, 120, 240, 480] | Cascading retry delays in seconds; after the last delay, stay at `SteadyDelaySeconds` |
| `Reconnect.SteadyDelaySeconds` | int | 480 | Retry interval after all cascading delays are exhausted |
| `Subscriptions` | array | [] | Subjects to listen on (see Channel Model). Includes `query.invalidate.ref.+` wildcard for per-QueryRef invalidation. |
| `WebSocketRelay` | object | `{Enabled: false}` | Forward events to WS clients; `Events` is the server-side allowlist; per-client `subscribed_events` set at WS connection time filters which clients receive each event |
| `Presence` | object | `{Enabled: false}` | Instance presence/heartbeat registry (see Presence section) |
| `Presence.HeartbeatIntervalSeconds` | int | 60 | App_state publish interval (how often "Alive" is re-published) |
| `Test` | object | test seam flags | For blackbox/Unity testing |
| **Scripting** | object | — | Lua handler linkage: `"NATS Handler":"NATS/intake_handler.lua"` maps NATS subjects to Lua handler files (see [Lua Host API](#lua-host-api)) |

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
- Example `hydrogen.json` section with `Enabled: false`
- Test 12 env-var pattern for `Username`/`Password` (`${env.*}`)
- `tests/artifacts/hydrogen_config_schema.json` — if schema is maintained

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
- `state.h` — `nats_system_shutdown` shutdown flag extern
- `state.c` — `nats_system_shutdown` definition (or in `nats.c`)
- `threads.h` / `state.c` — `ServiceThreads nats_threads` extern + definition
- Dependencies: Registry + Network always; Database when Enabled (cache hooks); WebSocket when WS relay enabled
  - `nats_registry.c` — peer registry for app_state tracking (no extra dependency)
  - `nats_subject.c` — subject validation helpers (no extra dependency)

`MAX_SUBSYSTEMS` (24) stays as-is (22nd subsystem fits).

### API / Swagger / prefix (if admin API is needed)

- Handler files under `src/api/nats/` + `nats_service.h` tag
- `api_service.c` routes + `json_endpoints`
- JWT auth (if endpoints need protection)
- `payloads/swagger-generate.sh` reads `//@ swagger:` annotations
- Test 20 (prefix), Test 22 (Swagger), Test 17 (min/max startup)

### Status / observability

- `NATSConfig` counters in `status_core.h` `ServiceMetrics` union + `SystemMetrics`
  (include peer count, reconnect count, messages published/subscribed)
- `GET /api/nats/status` (if API is added) or counters in `/api/system/info`
- `GET /api/nats/instances` (if presence enabled) — peer list + singleton status
- `log_this(SR_NATS, …)` — `num_args` matches `%` count
- Log in startup SERVICES / THREADS / QUEUES sections in `launch.c`

### Scripting

- `H.nats.broadcast` / `broadcast_sync` / `subscribe` / `unsubscribe` / `status` / `instances`
- Scripting config linkage: `"NATS Handler":"NATS/intake_handler.lua"` — Scripting subsystem loads the Lua handler and invokes it on matching NATS messages (primary subscription path)
- `H.nats.subscribe()` / `H.nats.unsubscribe()` — optional runtime subscription API for dynamic subject interest
- No `H.ws.broadcast()` — WebSocket push from Lua goes through NATS relay only
- Handle kind constant in `src/scripting/scripting_handle.h`
- `lua_api.md` documentation for all `H.nats.*` functions and the Scripting config linkage pattern

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
- `extras/natsval` — a local NATS test server mock or wrapper (analogous to
  `extras/pushval`, `extras/mailval`) if a real `nats-server` container is
  unavailable in CI. Alternatively, a lightweight `nats-server -D` dev-mode
  process started by the test script.
- `tests/test_62_nats.sh` + `docs/H/tests/test_62_nats.md` + 3 config files
- Ports: `562x` scheme (Test 62 → 5620–5629); NATS server fixture on 5620
- CHANGELOG + TEST_VERSION at the top of every script
- `jq` only for JSON in tests
- `TEST_COUNTER` owned by the framework — never increment manually
- Test 04 (markdown links), Test 90 (markdownlint), Test 91 (cppcheck),
  Test 92 (shellcheck), Test 93 (jsonlint), Test 98 (luacheck)

### Docs (Phase 12)

- `docs/H/core/subsystems/nats/` — subsystem guide (incl. app_state presence design)
- `docs/H/api/nats/` — API reference (`GET /api/nats/status`, `GET /api/nats/instances`)
- Index updates: `README.md`, `SITEMAP.md`, `STRUCTURE.md`, `INSTRUCTIONS.md`
  (V. NATS + launch 22; also add U. Chat to letter list to fix staleness),
  `API_OVERVIEW.md`, `lua_api.md`, `TESTING.md` (Test 62 row), `SECRETS.md`
  (NATS credential env names)
- **Documentation accuracy check**: The plan previously claimed the landing table
  is the "strict reverse of launch order." This was corrected — the landing table
  has its own order (verified in `landing_readiness.c`), adjusted per subsystem
  lifecycle rather than strictly reversed. Ensure no other docs make this claim.

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
- **`mkq` / `mkt`** after adding any `.c`/`.h` file (the glob
  `cmake/CMakeLists-init.cmake:147` requires a reconfigure on new files).
- **`mkp`** (cppcheck, Test 91) and **`mks`** (shellcheck, Test 92) after each
  phase.

### Unity unit tests — per source file

Each `.c` file in `src/nats/` gets dedicated Unity tests. Mock injection follows
the existing mock framework (`tests/unity/mocks/`). The NATS client connection
layer should sit behind an injectable seam (function pointer table or mock header,
like `mock_system`) so Unit tests never require a live NATS server.

| Source file | Unit-testable functions | Unity test files |
| --- | --- | --- |
| `nats.c` | `nats_init()`, `nats_shutdown()`, `nats_get_status()` (includes `state: healthy \| degraded`) | `nats_test_init.c`, `nats_test_shutdown.c`, `nats_test_get_status.c` |
| `nats_config.c` | `nats_config_apply_defaults()`, `validate_nats_config()`, `nats_subscription_validate()`, `nats_config_parse_reconnect()` | `nats_test_config_defaults.c`, `nats_test_config_validate.c`, `nats_test_subscription_validate.c` |
| `nats_publish.c` | `nats_publish()`, `nats_broadcast()`, `nats_build_envelope()`, `nats_broadcast_to_subject()` | `nats_test_publish.c`, `nats_test_build_envelope.c`, `nats_test_broadcast.c` |
| `nats_subscribe.c` | `nats_subscribe()`, `nats_unsubscribe()`, `nats_subscriptions_init()` | `nats_test_subscribe.c`, `nats_test_unsubscribe.c`, `nats_test_subscriptions_init.c` |
| `nats_dispatch.c` | `nats_dispatch_message()`, `nats_should_skip_self()`, `nats_parse_message()`, `nats_handle_cache_invalidate()`, `nats_handle_qtc_invalidate()`, `nats_handle_app_state()` | `nats_test_dispatch_message.c`, `nats_test_should_skip_self.c`, `nats_test_parse_message.c`, `nats_test_handle_cache_invalidate.c`, `nats_test_handle_qtc_invalidate.c`, `nats_test_handle_app_state.c` |
| `nats_ws_bridge.c` | `nats_ws_bridge_start()`, `nats_ws_bridge_stop()`, `nats_should_relay_to_ws()`, `nats_forward_to_clients()` | `nats_test_ws_bridge_start.c`, `nats_test_should_relay_to_ws.c`, `nats_test_forward_to_clients.c` |
| `nats_queue.c` | `nats_queue_init()`, `nats_queue_push()`, `nats_queue_pop()`, `nats_queue_destroy()` | `nats_test_queue_init.c`, `nats_test_queue_push.c`, `nats_test_queue_pop.c`, `nats_test_queue_destroy.c` |
| `nats_reconnect.c` | `nats_reconnect()`, `nats_backoff_delay()` (cascading: 30, 60, 120, 240, 480, then steady 480), `nats_reconnect_should_retry()` | `nats_test_reconnect.c`, `nats_test_backoff_delay.c`, `nats_test_reconnect_should_retry.c` |
| `nats_registry.c` | `nats_registry_init()`, `nats_registry_update_peer()`, `nats_registry_peers()`, `nats_registry_is_singleton()`, `nats_registry_active_count()`, `nats_registry_active_clients()` | `nats_test_registry_init.c`, `nats_test_registry_update_peer.c`, `nats_test_registry_peers.c`, `nats_test_registry_is_singleton.c`, `nats_test_registry_active_count.c`, `nats_test_registry_active_clients.c` |
| `nats_subject.c` | `nats_validate_subject_name()`, `nats_build_subject()` | `nats_test_validate_subject_name.c`, `nats_test_build_subject.c` |

**Mock strategy**: Create `mock_nats` (analogous to `mock_system`, `mock_libpq`)
for the underlying NATS C library calls (`natsConnection_*`, `natsSubscription_*`,
`natsMsg_*`). Define `USE_MOCK_NATS` before includes. The NATS connection layer
is the only module that links the real library in Unit tests; all dispatch,
parsing, subject-building, and cache-hook logic tests use the mock.

### Blackbox integration test — Test 62

`tests/test_62_nats.sh` is the end-to-end integration test. Config file:
`tests/configs/hydrogen_test_62_nats.json`. Port range: `562x` (single instance,
as NATS does not require multi-DB variants for its own logic).

**Subtests** (each `print_subtest` → exactly one `print_result`):

| # | TEST | What |
| --- | --- | --- |
| 1 | NATS disabled clean-skip | `NATS.Enabled: false` → server starts, no NATS connections, shutdown clean |
| 2 | NATS enabled, server unreachable | `Enabled: true` + no NATS server → readiness returns "degraded", server starts, retries at 30s→60s→120s→240s→480s→480s; status endpoint shows `state: "degraded"` |
| 3 | NATS enabled connects | `Enabled: true` + local mock NATS → connects, subscribes to configured subjects, publishes heartbeat within `HeartbeatIntervalSeconds + grace` |
| 4 | Cache invalidation broadcast → peer receive | Instance A publishes `cache.invalidate_by_ref` for QueryRef 127; Instance B receives, `query_cache_clear` evicts the entry; verify via query re-execution |
| 4a | QTC `InvalidateQTC` list broadcast | Query with `collection: {"InvalidateQTC":[14,15]}` executes → Instance A broadcasts two `cache.invalidate_by_ref` messages (refs 14, 15); Instance B receives both, evicts entries 14 and 15 from its QTC |
| 4b | QTC refresh semantics | After `cache.invalidate_by_ref` for QueryRef 14, Instance B's next lookup for ref 14 re-fetches from DB (not a stale cache hit) |
| 5 | WebSocket relay | With `WebSocketRelay.Enabled: true` + authenticated WS client with `subscribed_events` including `order.updated` → NATS broadcast event arrives as `nats_event` message on WS |
| 6 | WS relay filtering | With `WebSocketRelay.Events` list excluding an event type → that event is NOT forwarded to WS clients; also, a WS client whose `subscribed_events` excludes an event type does NOT receive it even if the server allowlist includes it |
| 7 | Presence / singleton detection | Two instances launched → both see each other as live peers within `HeartbeatIntervalSeconds * 3`; singleton reports 0 peers |
| 7a | Startup/shutdown tally | Instance A publishes `app_state` with state="Alive"; Instance B receives and tracks it as active. Instance A publishes state="Stopping"; Instance B removes it from active set after stale window. Singleton: only one instance in "Alive" state. Active client tally: sum of `active_connections` across all "Alive" instances. |
| 8 | Self-suppression | Instance publishes on a subject it also subscribes to → handler skips processing (`source == instance_id`) |
| 9 | Queue group load balancing | Queue-group subscription on `jobs.refresh-all` → message delivered to exactly one of N instances |
| 10 | Reconnect backoff | NATS server stops mid-session → Hydrogen reconnects on next backoff tick; verify via status endpoint shows `reconnect_count > 0` |
| 11 | Clean shutdown | SIGTERM during active NATS session → `nats_system_shutdown` flag set, threads joined, connection drained, server exits cleanly |

**NATS server fixture**: A real `nats-server -D` (in-memory store, dev mode)
started by the test script before launching Hydrogen. Port `5620`. Credentials:
none (dev mode). A fallback `extras/natsval` mock binary (analogous to
`extras/pushval`) can be used if the container is unavailable in CI.

**Helpers**: `tests/lib/nats_helpers.sh` — `run_nats_server()`, `stop_nats_server()`,
`nats_publish_test_event()`, `nats_wait_for_subscription()`, `assert_ws_relay()`.

### Blackbox test documentation

`docs/H/tests/test_62_nats.md` — mirrors the style of
[`test_47_mcp.md`](/docs/H/tests/test_47_mcp.md) and
[`test_43_scripting.md`](/docs/H/tests/test_43_scripting.md).

### Test config files

| Config | Port | NATS enabled? | Purpose |
| --- | --- | --- | --- |
| `hydrogen_test_62_nats_disabled.json` | 5620 | false | Subtest 1 |
| `hydrogen_test_62_nats_local.json` | 5621 | true | Subtests 3–12 |
| `hydrogen_test_62_nats_bad.json` | 5622 | true | Subtest 2 |

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

2. **Cache invalidation granularity**: RESOLVED — Query-declared invalidation. Each query's `collection` JSON may contain an `"InvalidateQTC":[14,15]` list of QueryRef IDs. When the query executes, Hydrogen broadcasts one NATS message per QueryRef in the list (e.g., `cache.invalidate_by_ref` with `data.query_ref = 14`). This is a hybrid approach: the query itself declares what it invalidates, so the invalidation is precise by construction. Subject naming: `cluster.<cluster_id>.query.invalidate.ref.<ref>` for per-QueryRef. QTC events are grouped under the `cache.invalidate*` family. "Invalidate" is treated as "refresh" — the cache entry is replaced with fresh data on receipt, not merely deleted. The existing `timeout_seconds` field in `QueryCacheEntry` was never implemented for auto-refresh; this NATS mechanism replaces that missing timeout behavior.

3. **Token table refresh mechanism**: RESOLVED — OIDC sits above the query cache layer and calls queries like any other subsystem. It does not maintain its own mutable token table. Cache invalidation/refresh is handled by the subscribe system at the database layer; OIDC is "none the wiser." The `token_refresh` NATS event is a phantom requirement and has been removed from the message model.

4. **WebSocket relay filter**: RESOLVED — each WebSocket connection carries a `subscribed_events` list. The bridge thread calls `ws_broadcast_json(event_type, payload)` which iterates sessions and forwards only to clients whose `subscribed_events` includes `event_type`. A client with no access to orders simply doesn't include `order.updated` in its list. See [WebSocket Broadcast Forwarding](#websocket-broadcast-forwarding).

5. **Heartbeat necessity**: RESOLVED — instance presence is now a required feature. See [Instance Presence & Group Membership](#instance-presence--group-membership). K8s probes cover crash detection; NATS heartbeats provide peer-count and singleton awareness.

6. **Error handling**: RESOLVED — cascading retry with exponential backoff: 30s, 60s, 120s, 240s, 480s, then stay at 480s indefinitely. NATS is a backchannel — nothing depends on it. If NATS is enabled but unavailable at launch, the server starts anyway (degraded mode). The readiness check returns "degraded" (not No-Go). If NATS becomes available after a retry period, it updates its status in the subsystem registry and operates normally. If it stays unavailable, Hydrogen continues running without NATS; retries continue at 480s intervals. When NATS later becomes reachable, the connection is established and the subsystem transitions to healthy.

7. **Config letter conflict**: RESOLVED — NATS takes **V** (implemented first). NOTIFICATIONS/SUBSCRIBERS plan, if approved later, shifts to **W**. Documented in both plans.

8. **Subscription path**: RESOLVED — both paths exist. Config-driven (`"NATS Handler":"NATS/intake_handler.lua"`) is the primary path; `H.nats.subscribe()` / `H.nats.unsubscribe()` are available as an optional Lua API for runtime subscriptions the config cannot anticipate.

9. **Self-suppression**: RESOLVED — every message carries `instance_id` in the envelope. The dispatch handler checks `message.source == own instance_id` universally (all message types, not selective). NATS itself prevents self-delivery via separate publish/subscribe connections; the application-level check is a safety net (defense in depth). Originating instance updates its own cache optimistically (no round-trip wait).

10. **Auth simplicity**: RESOLVED — internal DOKS cluster, no external exposure. Simple auth only (username/password from env vars, or none). No NKEYS/JWT enterprise auth needed. Credentials in config via `${env.NATS_USERNAME}` / `${env.NATS_PASSWORD}` env var substitution.

11. **App startup/shutdown tally**: RESOLVED — single subject `cluster.<id>.app_state` with payload `{instance_id, state: "Starting"|"Alive"|"Stopping", active_connections}`. "Alive" includes unique active IPs in last 5 minutes (REST API + WebSocket). Instances derive active count and client tally from aggregated "Alive" payloads using the shared heartbeat timeout. Crash handling via heartbeat-stale correction. No separate startup/shutdown subjects.

12. **Delivery guarantees**: RESOLVED — fire-and-forget, at-most-once. No ack required. NATS is a backchannel; nothing depends on guaranteed delivery. If a message is lost, the next cache lookup re-fetches from the database (cache miss → fresh data). The `timeout_seconds` field in `QueryCacheEntry` serves as a safety net for missed invalidations.

13. **Self-suppression**: RESOLVED — standard NATS behavior with separate publish/subscribe connections means an instance does NOT receive its own published messages on the same subject. The application-level `source == instance_id` check in `nats_dispatch.c` is a safety net (defense in depth) for edge cases where the library might echo back. No special handling needed in the custom client beyond the standard check.

14. **QTC invalidation — originating instance**: RESOLVED — when an instance executes a mutating query with `InvalidateQTC:[14,15]`, it broadcasts invalidation messages for refs 14, 15 AND updates its own cache immediately (optimistic update). It does NOT wait for the round-trip message to come back, because self-suppression means it won't receive its own messages anyway, and there's no guarantee the round-trip will complete. The broadcast is for peer instances only.

15. **QTC subject strategy**: RESOLVED — per-QueryRef only (`cluster.<id>.query.invalidate.ref.<ref>`). No broad `cache.invalidate` subject. Broad invalidation can be implemented in Lua if needed later.

16. **QTC format**: RESOLVED — `InvalidateQTC` is a list of query numbers in the query's `collection` JSON. When the query executes, Hydrogen broadcasts one `cache.invalidate_by_ref` message per QueryRef in the list. Simple and minimal: just the QueryRef IDs, no extra envelope fields.

17. **NATS server version**: RESOLVED — NATS 2.11.4 in DOKS (confirmed via kubectl). Recent version; no compatibility concerns for the custom client.

18. **TLS cert rotation**: RESOLVED — NATS has no TLS configured in DOKS (plain text on 4222). Not a concern for now. If TLS is added later, cert rotation follows the existing Network TLS pattern.

19. **WebSocket vhost**: RESOLVED — WebSocket runs on its own dedicated port, not per-vhost. Single `/ws` endpoint. No vhost routing complexity for the NATS bridge.

20. **`ws_broadcast_json` ownership**: RESOLVED — WebSocket subsystem owns `ws_broadcast_json()`. The NATS bridge thread (`nats_ws_bridge.c`) calls it.

21. **`nats_config.c` vs `nats_client.c` boundary**: RESOLVED — `nats_config.c` is just initial config parsing (like other subsystems). Connection lifecycle (connect/disconnect/reconnect) lives in `nats_client.c`.

22. **Handle kind constant**: RESOLVED — new `H_HK_NATS` enum value in `scripting_handle.h`, following the existing pattern (next after `H_HK_MCP = 6`).

23. **Presence registry persistence**: RESOLVED — registry is ephemeral. Clients re-register on reconnect. No persistence needed.

24. **Queue group name collision**: RESOLVED — cluster id prefixes queue group names. Two Hydrogen instances with different cluster ids won't collide.

25. **`app_state` active_connections scope**: RESOLVED — unique list across both WebSocket and REST API connections. `active_connections` is the count of unique active IPs in the last 5 minutes, covering all client types.

---

## Proposed phased breakdown

If approved, this becomes a multi-phase plan (modeled on NOTIFICATIONS_PLAN):

| Phase | Goal | Key deliverables |
| --- | --- | --- |
| 0 | Design lock | Approve letter V, launch 22, message model, channel model, custom client decision |
| 1 | Config + launch + landing | `config_nats.c/h`, `launch_nats.c`, `landing_nats.c`, `nats_subject.c`, wiring in all dispatch tables, disabled clean-skip |
| 2 | NATS connection + lifecycle | Custom NATS client (`nats_client.c`): connect, publish, subscribe, reconnect, ping/pong, TLS; shutdown flag, threads |
| 3 | Publish path (broadcast) | `nats_broadcast()` function, message envelope, test seam |
| 4 | Subscribe + dispatch | Receive messages, parse JSON, dispatch to cache invalidation hooks |
| 5 | Cache invalidation hooks | Integrate with `query_cache_clear` / token table refresh in Database + OIDC layers; QTC `InvalidateQTC` list parsing and per-QueryRef broadcast; `app_state` publish on launch/landing (state="Starting" → "Alive" → "Stopping") |
| 6 | WebSocket relay | Bridge thread forwarding selected events to authenticated WS clients; `ws_broadcast_json(event_type, payload)` filters by client `subscribed_events` |
| 7 | Instance presence & registry | `nats_registry.c`, `app_state` publish/subscribe (state="Alive" with active_connection), singleton detection, active client tally via aggregated "Alive" payloads |
| 8 | Lua host API | `H.nats.broadcast` / `subscribe` / `unsubscribe` / `status` / `instances` |
| 9 | Status + metrics | Counters in `status_core.h`, `GET /api/nats/status` + `GET /api/nats/instances` |
| 10 | Unity unit tests | All unit-testable functions covered (see Testing Strategy table), 75% fence for files >100 lines |
| 11 | Blackbox Test 62 | End-to-end integration with local `nats-server`; all 12 subtests pass |
| 12 | Docs + indexes | Operator guide, API docs, INSTRUCTIONS.md/STRUCTURE.md/SITEMAP.md updates, lua_api.md |

---

## Reference: how a recently-added subsystem was integrated

For exact patterns to mirror, study these files (they represent the most recent
subsystem additions):

- `src/config/config_mcp.h` + `config_mcp.c` — config struct + load/dump/cleanup
- `src/launch/launch_mcp.c` — readiness clean-skip pattern + launch dispatch
- `src/landing/landing_mcp.c` — landing readiness + land function
- `src/mcp/mcp.h` — minimal public API surface
- `src/registry/registry_integration.h:109-110` — `mcp_threads` + `mcp_system_shutdown` externs
- `src/threads/threads.h:48` — `extern ServiceThreads mcp_threads;`
- `src/state/state.h:54` — `extern volatile sig_atomic_t mcp_system_shutdown;`
- `src/launch/launch_readiness.c:424` — after-MCP readiness check position
- `src/launch/launch.c:197` — after-MCP launch dispatch
- `src/landing/landing_readiness.c:142` — after-MCP landing readiness table entry
- `src/landing/landing.c:104` — MCP landing function dispatch
- `src/status/status_core.h` — `ServiceMetrics mcp;` in `SystemMetrics` + union arm
- `src/config/config_chat.h` — config-only subsystem (no launch) pattern
