# mDNS Server

## What is the mDNS Server?

The mDNS Server is the network announcer that makes the Hydrogen server visible to other devices on the local network. It probes for a unique name, claims it, then advertises services and answers queries per RFC 6762/6763.

## Why is it Important?

Connecting to a device like a 3D printer requires knowing its exact IP address. The mDNS Server solves this by broadcasting availability via multicast DNS, so clients find and connect to Hydrogen automatically.

## Key Features

- **Probe / claim** with conflict rename before announcing
- **Selective answers** — only the records the question asks for
- **Known-answer suppression**, QU unicast, legacy unicast
- **NSEC** when a family has no address
- **Cache-flush** on unique records, TTL split (shared 4500 s / host 120 s)
- **Defend**, shared-record delay, unique-record rate limit, probe tiebreak
- **Goodbye burst** (TTL=0 ×3) on shutdown

## Documentation

- [mdnsserver.md](/docs/H/core/subsystems/mdnsserver/mdnsserver.md) — full server documentation
- [mdns_configuration.md](/docs/H/core/reference/mdns_configuration.md) — configuration guide
