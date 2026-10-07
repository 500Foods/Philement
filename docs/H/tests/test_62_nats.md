# Test 62: NATS

## Overview

The [`test_62_nats.sh`](/elements/001-hydrogen/hydrogen/tests/test_62_nats.sh)
script black-box tests the NATS plaintext client against a local
`nats-server`. Broker, Prometheus, and publish helpers live in
[`nats_helpers.sh`](/elements/001-hydrogen/hydrogen/tests/lib/nats_helpers.sh).

## Purpose

Validates one coverage binary against a local broker:

- Disabled NATS reports `hydrogen_nats_enabled` 0 and `hydrogen_nats_up` 0,
  logs the disabled launch line, and leaves port 5620 closed
- An unreachable server logs launch accepted and stays enabled 1, up 0,
  reconnects 0
- The monitor `/varz` answers HTTP 200
- Instance A reaches up and subscribes the four `cluster.philement` subjects,
  with `jobs.refresh-all` in queue `philement-refresh`
- `no_echo` keeps received at 0 and peers at 0 for 3 seconds after up,
  while published is at least 1. The client sends CONNECT `echo` false,
  which is the field `nats-server` 2.11 honors
- An outside `app_state` that reuses A's instance id is skip-self
- Peer B becomes visible on both sides
- An authenticated WebSocket client receives `order.updated`; a client with
  an empty event list does not
- One `jobs.refresh-all` publish is traced on exactly one of A or B,
  and received rises on that instance. A presence heartbeat is a
  different trace line. The subscribed client does not get a `nats_event`
- Stopping the broker increments A's reconnects and clears up; bringing the
  broker back restores up and the four subjects
- Stopping B returns A's peers to 0, then both Hydrogen processes exit clean

Cache invalidation rows 4, 4a, and 4b stay on the Phase 5 Unity tests.
This script does not log in and does not call `GET /api/nats/status`.

## Test Configuration

- **Test Name**: NATS
- **Test Abbreviation**: NAT
- **Test Number**: 62
- **Version**: 1.0.2

## Port Assignment

Every listener is `127.0.0.1`. Hydrogen and `nats-server` do not share a port.

| Port | Process |
| --- | --- |
| 5620 | `nats-server` client port |
| 5621 | Hydrogen, NATS disabled |
| 5622 | Hydrogen A |
| 5623 | Hydrogen pointed at 5628 |
| 5624 | Hydrogen B, a `jq` copy of the local config |
| 5625 | WebSocket for A |
| 5626 | WebSocket for B |
| 5628 | Nothing listens |
| 5629 | `nats-server` monitor |

## Configuration Files

- `hydrogen_test_62_nats_disabled.json` (port 5621, NATS disabled, no database)
- `hydrogen_test_62_nats_bad.json` (port 5623, server `nats://127.0.0.1:5628`,
  presence off, relay off, one SQLite connection)
- `hydrogen_test_62_nats_local.json` (port 5622, WebSocket 5625, the four
  subscriptions, relay, and presence)

`NATS.Test.MockConnection` is false. Username and Password are empty strings.
`PayloadKey` is `${env.PAYLOAD_KEY}`. The WebSocket key is `${env.WEBSOCKET_KEY}`,
and the script sets that variable. Enabled configs copy
`tests/artifacts/database/sqlite/hydrotst.sqlite` into the diagnostics
directory and rewrite `Database` so A and B do not share a file.
`AutoMigration` and `TestMigration` are false. The connection name is Helium.
The bootstrap SELECT matches Test 34.

The broker is a local `nats-server` 2.x binary from `PATH` or
`/usr/local/bin/nats-server`, bound with `-a 127.0.0.1 -p 5620 -m 5629`,
with no auth and no TLS. A missing binary fails the locate subtest.

## Prerequisites

- `hydrogen_coverage` at the Hydrogen root
- `PAYLOAD_KEY`
- `websocat` (the same client Test 23 uses)
- `hydrotst.sqlite` already on disk
- A local plaintext `nats-server`

## Test Flow

1. Locate `hydrogen_coverage` and `nats-server`
2. Disabled instance, then shutdown
3. Unreachable instance, then shutdown
4. Start the broker and read `/varz`
5. Start A and read `/subsz?subs=1`
6. Read published, received, and peers within 3 seconds of up
7. Publish one outside `app_state` with instance id `nats-62-a`
8. Start B and read peer gauges
9. WebSocket `nats_subscribe` for `order.updated`, then one outside PUB.
   `websocat` stdout is a terminal, so each frame is on disk before the
   wait ends
10. One outside PUB of `jobs.refresh-all`. The trace names that subject,
    so a presence heartbeat is not counted as a second delivery
11. Stop the broker, then start it again
12. Stop B, then stop A and the broker

Assertions read `GET /api/system/prometheus` and save the body before
reading a gauge. Monitor documents are read with `jq`.

## Related

- Plan: [NATS_PLAN_COMPLETE.md](/docs/H/plans/complete/NATS_PLAN_COMPLETE.md) Phase 11
- Guide: [nats.md](/docs/H/core/subsystems/nats/nats.md)
- Helper: [`nats_helpers.sh`](/elements/001-hydrogen/hydrogen/tests/lib/nats_helpers.sh)
