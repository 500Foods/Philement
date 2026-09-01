# mDNS Client

## What is the mDNS Client?

The mDNS Client browses configured service types, resolves SRV/TXT/A/AAAA records, caches discovered services, honors TTL-0 goodbye, runs optional TCP health checks, and exposes a registry other subsystems can query.

## Why is it Important?

Discovery is what lets Hydrogen find printers, web endpoints, and other services on the LAN without manual configuration. The client builds the picture the server side announces.

## Key Features

- Browse/resolve worker with immediate first query
- Thread-safe cache with endpoint ordering and dedup
- TTL-0 goodbye, max-services cap with drop logging
- Filters: own services, printer types, custom types
- TCP health checks to advertised host:port
- Registry snapshot API, `/api/system/info`, Lua `H.mdns.list`

## Documentation

- [mdnsclient.md](/docs/H/core/subsystems/mdnsclient/mdnsclient.md) — full client documentation
- [mdns_client_architecture.md](/docs/H/core/reference/mdns_client_architecture.md) — architecture overview
- [mdns_configuration.md](/docs/H/core/reference/mdns_configuration.md) — configuration guide
