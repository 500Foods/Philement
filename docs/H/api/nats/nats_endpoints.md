# NATS Endpoints

NATS exposes two JWT routes on the WebServer API port. The subsystem guide is [nats.md](/docs/H/core/subsystems/nats/nats.md).

| Method | Path | Auth | When NATS is disabled |
| --- | --- | --- | --- |
| `GET` | `/api/nats/status` | JWT | **200** with `enabled` false and `state` `down` |
| `GET` | `/api/nats/instances` | JWT | **200** with `self` and an empty `peers` array |

Any other method is **405**:

```json
{"success":false,"error":"Method not allowed","message":"Only GET requests are supported"}
```

A missing or invalid JWT is **401** from the same helper as the other API routes. No special role is required. These paths are not on the protected-endpoint list. A builder failure is **500**.

The status log line is `Status request: enabled=%s link=%s`. The instances log line is `Instances request`. Neither line includes a peer id.

## `GET /api/nats/status`

```bash
curl -sS "http://127.0.0.1:5000/api/nats/status" \
  -H "Authorization: Bearer ${TOKEN}"
```

```json
{
  "success": true,
  "enabled": true,
  "link": "up",
  "state": "up",
  "presence": false,
  "singleton": false,
  "peers": 0,
  "alive": 0,
  "self": "none",
  "published": 1,
  "received": 0,
  "reconnects": 0
}
```

| Field | Meaning |
| --- | --- |
| `enabled` | `NATS.Enabled` |
| `link` | `down`, `degraded`, or `up` |
| `state` | `down` only when NATS is disabled. Enabled and not up is `degraded` |
| `presence` | `Presence.Enabled` |
| `singleton` | True only when presence is on, the link is `up`, this instance has been `Alive` for one stale window, and no other peer is `Alive` |
| `peers` | Other instances in the registry |
| `alive` | Other instances in `Alive` |
| `self` | `Starting`, `Alive`, `Stopping`, or `none` |
| `published` | Finished PUB writes since `nats_start` |
| `received` | Inbound MSG bodies since `nats_start`, including messages later dropped |
| `reconnects` | Sessions that reached `up` and then dropped while the process was not shutting down |

`H.nats.status` returns the same link and registry fields and omits `published`, `received`, and `reconnects`.

## `GET /api/nats/instances`

```bash
curl -sS "http://127.0.0.1:5000/api/nats/instances" \
  -H "Authorization: Bearer ${TOKEN}"
```

```json
{
  "success": true,
  "singleton": false,
  "self": "Alive",
  "peers": [
    {"id": "hydrogen-b", "state": "Alive", "websocket_connections": 2}
  ]
}
```

Presence off still returns `self` and an empty `peers` array. Each peer is another instance. `state` on a peer is `Starting` or `Alive`. A `Stopping` message removes that peer. The table holds 64 peers. `websocket_connections` is the count from that peer's last Alive beat when `ReportConnections` is on.

`H.nats.instances` returns the same `singleton`, `self`, and peers. The Lua `peers` array is 1-based.

## Other surfaces

### Prometheus

`GET /api/system/prometheus` does not require a JWT. Save the body, then read the file. The NATS series have no labels.

| Name | Kind |
| --- | --- |
| `hydrogen_nats_enabled` | gauge, 1 or 0 |
| `hydrogen_nats_up` | gauge, 1 when `link` is `up` |
| `hydrogen_nats_published_total` | counter |
| `hydrogen_nats_received_total` | counter |
| `hydrogen_nats_reconnects_total` | counter |
| `hydrogen_nats_peers` | gauge |
| `hydrogen_nats_alive` | gauge |

### `services.nats`

A public `GET /api/system/info` (no JWT) does not include `services`. A valid JWT on that route, and Lua `H.system.info`, include:

```json
{
  "enabled": true,
  "link": "up",
  "state": "up",
  "peers": 1,
  "alive": 1,
  "published": 1,
  "received": 0,
  "reconnects": 0
}
```

`state` follows the same rule as `/api/nats/status`.
