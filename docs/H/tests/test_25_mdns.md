# Test 25: mDNS Server and Client

## Overview

`test_25_mdns.sh` validates both the mDNS server and the mDNS client. It is the blackbox gate for the [mDNS Upgrade](/docs/H/plans/complete/MDNS_UPGRADE_COMPLETE.md) plan.

## Purpose

The script proves that the server probes, claims, announces, responds selectively, and sends goodbye; that the client browses, resolves, caches, and logs the discovery sequence; and that two processes with the same instance name cause one to rename.

## Script Details

- **Script Name**: `test_25_mdns.sh`
- **Test Name**: mDNS
- **Version**: 4.1.0
- **Dependencies**: `tests/lib/mdns_test_helpers.sh`, `lib/`, tshark (optional)

## What It Tests

### Log Contract (always gating)

The test greps the Hydrogen server log for these stable tokens:

- `MDNS_SERVER CLAIMED` — instance and hostname survived probing
- `MDNS_CLIENT QUERY` — browse/resolve query sent
- `MDNS_CLIENT FOUND` — new instance cached
- `MDNS_CLIENT SRV` — port + target
- `MDNS_CLIENT TXT` — at least one key
- `MDNS_CLIENT ADDR` — A or AAAA
- `MDNS_SERVER GOODBYE` / `MDNS_CLIENT GOODBYE` — TTL-0 on shutdown

### Duplicate Names (always gating — two processes)

A second Hydrogen is started with the **same** instance name as the first but different ports. The second must log `MDNS_SERVER CONFLICT` then `MDNS_SERVER CLAIMED` with `(2)`.

### Wire Capture (gating if tshark exists; skip-not-fail if not)

After CLAIMED, the pcap must contain:

- PTR per advertised type
- SRV with the configured port
- TXT with a known key
- At least one A or AAAA
- QR=1 AA=1 responses
- Probe packets (ANY question, QR=0) before claim
- TTL=0 goodbye records after shutdown

When tshark cannot capture loopback multicast, server log evidence of announcements is accepted as proof.

## Configuration Files

- `hydrogen_test_25_mdns.json` — enables mDNS server + client
- `hydrogen_test_25_mdns_dup.json` — same instance name, different ports (for the duplicate-name test)

## Usage

```bash
# Run individually
./test_25_mdns.sh

# Run as part of the full suite
./test_00_all.sh all
```

## Results

- **Results directory**: `tests/results/`
- **Server logs**: `tests/logs/test_25_mdns_[timestamp]_server.log`
- **Packet capture**: `tests/logs/test_25_mdns_[timestamp]_traced.log`

## Related Documentation

- [mDNS Server](/docs/H/core/subsystems/mdnsserver/mdnsserver.md)
- [mDNS Client](/docs/H/core/subsystems/mdnsclient/mdnsclient.md)
- [mDNS Configuration Guide](/docs/H/core/reference/mdns_configuration.md)
- [test_00_all.md](/docs/H/tests/test_00_all.md)
