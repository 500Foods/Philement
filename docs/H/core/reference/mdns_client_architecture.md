# mDNS Client Architecture

The mDNS Client subsystem provides service discovery and monitoring, enabling Hydrogen to discover and track services on the local network.

## System Overview

```diagram
┌───────────────────────────────────────────────────────────────┐
│                     mDNS Client System                        │
│                                                               │
│   ┌───────────────┐         ┌───────────────┐                 │
│   │   Service     │         │   Service     │                 │
│   │   Scanner     │────────►│   Monitor     │                 │
│   └───────┬───────┘         └───────┬───────┘                 │
│           │                         │                         │
│           ▼                         ▼                         │
│   ┌───────────────┐         ┌───────────────┐                 │
│   │   Service     │         │   Health      │                 │
│   │   Registry    │◄────────┤   Checker     │                 │
│   └───────┬───────┘         └───────────────┘                 │
│           │                                                   │
│           ▼                                                   │
│   ┌───────────────┐                                           │
│   │   Event       │                                           │
│   │   Dispatcher  │                                           │
│   └───────────────┘                                           │
│                                                               │
└───────────────────────────────────────────────────────────────┘
```

## What Shipped vs What Is Deferred

This plan shipped the boxes in the diagram below. The load-balancer box is **out of scope** for this plan and is deferred to a later effort (Print / Conduit once a second consumer exists).

| Architecture box | Status | Where |
|---|---|---|
| Scanner | Shown | `src/mdns/mdns_client.c` browse/resolve loop |
| Monitor | Shown | `src/mdns/mdns_client_cache.c` cache + TTL refresh + goodbye |
| Registry | Shown | `src/mdns/mdns_client_registry.c` snapshot API, system/info, `H.mdns.list` |
| Health checker | Shown | `src/mdns/mdns_client_health.c` TCP connect to advertised host:port |
| Event dispatcher | Shown | log tokens + optional C callback `mdns_client_on_change` |
| Load balancer | **Deferred** | `MonitoredServices.LoadBalancers` parsed and ignored until a later plan defines a service type |

## Key Components

### Service Scanner

- Periodic network scanning at `ScanIntervalMs` (milliseconds).
- Service type filtering from configured `ServiceTypes`.
- Sends the first browse query immediately at thread start (does not wait a full interval).
- Accepts unsolicited announcements (QR=1) so start order vs server does not matter.

### Service Monitor

- Real-time service tracking in a thread-safe cache capped at `max_services`.
- TTL-0 goodbye drops the entry.
- Endpoint ordering: IPv4, routable IPv6, link-local IPv6; tie-broken by address bytes.
- Endpoint dedup by (name, family, address bytes).

### Service Registry

- Thread-safe snapshot API: `mdns_client_snapshot()` / `mdns_client_count()` / `mdns_client_lookup_by_type()`.
- Exposed via `/api/system/info` (claimed instance/hostname, cache count, up to N instances).
- Exposed via Lua `H.mdns.list([type])`.

### Health Checker

- TCP connect to each cached endpoint's advertised host:port.
- Configurable `IntervalMs`, `TimeoutMs`, `RetryCount`.
- Logs `MDNS_CLIENT HEALTH ok`/`fail`; marks entry healthy/unhealthy.
- No HTTP GET; no HTTP health URLs.

### Event Dispatcher

- Stable log tokens (see Log Contract) greppable in the Hydrogen server log.
- Optional in-process callback `mdns_client_on_change` for found/lost/health events.

## Configuration

Configured through the `mDNSClient` section of `hydrogen.json`. See the [mDNS Configuration Guide](/docs/H/core/reference/mdns_configuration.md).

## Log Contract

| Token | When |
|---|---|
| `MDNS_CLIENT QUERY` | browse/resolve query sent |
| `MDNS_CLIENT FOUND` | new instance cached |
| `MDNS_CLIENT SRV` | port + target |
| `MDNS_CLIENT TXT` | at least one key |
| `MDNS_CLIENT ADDR` | A or AAAA |
| `MDNS_CLIENT GOODBYE` | TTL-0 for a cached instance |
| `MDNS_CLIENT HEALTH` | TCP check result |
| `MDNS_CLIENT DROP` | evicted |

## Non-Goals

- Round-robin / health-weighted job routing inside mDNS.
- HTTP `auto_connect`, HTTP health URLs.
- Lithium UI for the registry.
- Persistent registry across restart.

## References

- [mDNS Client](/docs/H/core/subsystems/mdnsclient/mdnsclient.md)
- [mDNS Server](/docs/H/core/subsystems/mdnsserver/mdnsserver.md)
- [mDNS Configuration Guide](/docs/H/core/reference/mdns_configuration.md)
