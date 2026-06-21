# Hydrogen Deployment Guide

This guide covers the supported strategies for deploying the Hydrogen server in production. It focuses on three deployment shapes that are used in practice:

- A standalone server on a single VPS (with or without `systemd`)
- A Kubernetes deployment (DigitalOcean Kubernetes / DOKS and equivalents)
- Variations with and without a configured database

It also documents the HTTP-based health checking model that Kubernetes deployments rely on, including the recent switch from raw TCP socket probes to HTTP `GET` probes against the system endpoints.

## Table of Contents

- [Deployment Overview](#deployment-overview)
- [Health and Readiness Endpoints](#health-and-readiness-endpoints)
- [Standalone Server on a VPS](#standalone-server-on-a-vps)
- [Kubernetes / DOKS Deployment](#kubernetes--doks-deployment)
- [Probe Configuration: TCP vs HTTP](#probe-configuration-tcp-vs-http)
- [Deployments With a Database](#deployments-with-a-database)
- [Deployments Without a Database](#deployments-without-a-database)
- [Reference Tenant Deployments](#reference-tenant-deployments)
- [Related Documentation](#related-documentation)

## Deployment Overview

Hydrogen is a single statically-configured C binary. A deployment is essentially three things:

1. The `hydrogen` binary.
2. A JSON configuration file (see the [Configuration Guide](/docs/H/core/configuration.md)).
3. A handful of environment variables for secrets and per-environment values (see [SECRETS.md](/docs/H/SECRETS.md)).

The server exposes a single HTTP listener (default port `7000` in production deployments) that serves both static content and the REST API. An optional WebSocket listener (default `7001`) is exposed when the WebSocket subsystem is enabled.

Regardless of where Hydrogen runs, startup follows the launch/landing model: each subsystem moves through Readiness, Plan, Launch, and Review phases. The server only reports itself as ready for requests once the required subsystems have completed launch. This is the behavior the readiness probe observes.

## Health and Readiness Endpoints

Two system endpoints are central to operating Hydrogen in any orchestrated environment. Both are always available once the API subsystem is up and do not require authentication.

### `/api/system/health`

A pure liveness signal. It always returns HTTP `200 OK` with a small JSON body:

```json
{ "status": "Yes, I'm alive, thanks!" }
```

It performs no subsystem, database, or readiness checks. It only confirms that the process is running and able to serve HTTP. Use it for liveness probes and load-balancer health monitoring.

### `/api/system/readiness`

A readiness signal that reflects whether the server has finished bringing up its subsystems (and, when configured, its databases).

- Returns HTTP `200 OK` once the server is ready for requests.
- Returns HTTP `503 Service Unavailable` while the server is still starting.

The JSON body reports the detail behind the decision:

```json
{
  "ready": false,
  "databases_expected": 2,
  "databases_ready": 1,
  "starting": ["Acuranzo"],
  "started": ["Helium"],
  "status": "starting Acuranzo; started Helium"
}
```

The authoritative signal is the internal `server_ready` flag, which flips to ready once the server emits "READY FOR REQUESTS". How that happens depends on whether a database is configured:

- With one or more databases enabled, the flag is set only after every database's lead queue manager has completed its full sequence (connect, bootstrap, migration, and additional queues). Until then, readiness returns `503` with per-database progress in the `starting` and `started` arrays.
- With no databases enabled, there is nothing to wait for, so the server flips to ready almost immediately and readiness returns `200` with `databases_expected: 0`.

### `/api/system/info`

A richer, read-only status endpoint that returns full system information (hardware, OS, runtime statistics, version, and WebSocket metrics) as JSON. It is useful for dashboards and diagnostics but is not intended as a probe target.

## Standalone Server on a VPS

The simplest deployment is a single binary running on a Linux VPS. This suits low-traffic sites, internal tools, and any case where Kubernetes would be overkill.

### Layout

A typical layout keeps the binary, configuration, and served content together:

```text
/opt/hydrogen/
├── hydrogen              # the compiled binary
├── hydrogen.json         # configuration file
└── web/                  # static content served by the WebServer subsystem
```

### Running directly

```bash
cd /opt/hydrogen
./hydrogen ./hydrogen.json
```

The server reads its configuration, launches the enabled subsystems, and begins serving on the configured port. Environment variables referenced by the configuration (for example `PAYLOAD_KEY`, and any `*_DB_*` values when a database is enabled) must be present in the shell environment. See [SECRETS.md](/docs/H/SECRETS.md) for the full list and generation instructions.

### Running under systemd

For an always-on service, run Hydrogen under `systemd` so it restarts on failure and starts at boot. Create `/etc/systemd/system/hydrogen.service`:

```ini
[Unit]
Description=Hydrogen Server
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
User=hydrogen
WorkingDirectory=/opt/hydrogen
EnvironmentFile=/etc/hydrogen/hydrogen.env
ExecStart=/opt/hydrogen/hydrogen /opt/hydrogen/hydrogen.json
Restart=on-failure
RestartSec=5
# Graceful shutdown: SIGTERM triggers the landing sequence
KillSignal=SIGTERM
TimeoutStopSec=30

[Install]
WantedBy=multi-user.target
```

Place per-environment secrets in `/etc/hydrogen/hydrogen.env` (one `KEY=value` per line, mode `0600`, owned by the service user). Then enable and start it:

```bash
sudo systemctl daemon-reload
sudo systemctl enable --now hydrogen
sudo systemctl status hydrogen
```

Hydrogen handles `SIGTERM`, `SIGINT`, and `SIGHUP` cleanly through the landing (shutdown) sequence, so `systemctl stop` and `systemctl restart` shut the server down gracefully. See [Shutdown Architecture](/docs/H/core/shutdown_architecture.md) for details.

### Health checking on a VPS

Even without an orchestrator, the same endpoints are useful behind a reverse proxy or external monitor:

```bash
# Liveness
curl -fsS http://127.0.0.1:7000/api/system/health

# Readiness (exit non-zero while starting; useful in a wait loop)
curl -fsS http://127.0.0.1:7000/api/system/readiness
```

If TLS is terminated by a fronting proxy (nginx, Caddy, Traefik), point the proxy upstream at the Hydrogen port and use these endpoints for its upstream health checks.

## Kubernetes / DOKS Deployment

In Kubernetes, Hydrogen is deployed as a `Deployment` plus a `Service` and an `Ingress`. The reference deployments run on DigitalOcean Kubernetes (DOKS) using Traefik for ingress and cert-manager for TLS.

A deployment consists of these resources:

| Resource | Purpose |
|---|---|
| `Namespace` | Isolates the tenant's workloads |
| `PersistentVolumeClaim` | Mounts shared storage (CephFS in the reference setup) holding the binary, config, and web content |
| `Secret` | Holds `PAYLOAD_KEY` and any database credentials, created from environment variables |
| `Deployment` | Runs the `hydrogen` container with probes configured against the HTTP endpoints |
| `Service` | Exposes the pod on the cluster network (port `7000`, plus `7001` if WebSocket is enabled) |
| `Ingress` | Routes external HTTPS traffic to the Service and provisions TLS certificates |

### Container and command

The reference image bundles the binary and is invoked with the path to a configuration file. The binary and config live on the mounted volume:

```yaml
containers:
- name: hydrogen
  image: registry.festival:51001/festival-base:v3.0.0
  imagePullPolicy: Always
  workingDir: /tnt
  command: ["hydrogen/hydrogen", "hydrogen/hydrogen.json"]
  ports:
  - containerPort: 7000
    name: http
    protocol: TCP
  securityContext:
    runAsNonRoot: true
    runAsUser: 1000
```

### Probes

Kubernetes deployments configure a readiness probe and a liveness probe against the HTTP system endpoints:

```yaml
readinessProbe:
  httpGet:
    path: /api/system/readiness
    port: 7000
  initialDelaySeconds: 5
  periodSeconds: 10
livenessProbe:
  httpGet:
    path: /api/system/health
    port: 7000
  initialDelaySeconds: 10
  periodSeconds: 30
```

- The readiness probe targets `/api/system/readiness`. Kubernetes keeps the pod out of the Service endpoints (so the Ingress sends no traffic) until it returns `200`. This prevents requests from reaching a pod that is still launching subsystems or waiting on a database.
- The liveness probe targets `/api/system/health`. Because health always returns `200` while the process is up, the pod is only restarted if the process is genuinely wedged (the HTTP listener stops responding), not merely because it is still starting.

### Service and Ingress

```yaml
apiVersion: v1
kind: Service
metadata:
  name: www-example
  namespace: t-example
spec:
  selector:
    app: www-example
    domain: Hydrogen
  ports:
  - port: 7000
    targetPort: 7000
    protocol: TCP
    name: http
  type: ClusterIP
```

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: www-example
  namespace: t-example
  annotations:
    cert-manager.io/cluster-issuer: "letsencrypt-prod"
spec:
  ingressClassName: traefik
  tls:
  - hosts:
    - www.example.com
    secretName: fvl-tls-t-example
  rules:
  - host: www.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: www-example
            port:
              number: 7000
```

### Secrets

Secrets are created from environment variables present in the operator's shell, rather than committed to manifests. The minimal case only needs the payload decryption key:

```bash
kubectl create namespace t-example 2>/dev/null || true
kubectl delete secret t-example-secrets -n t-example 2>/dev/null || true
kubectl create secret generic t-example-secrets -n t-example \
  --from-literal=PAYLOAD_KEY="$PAYLOAD_KEY"
```

The deployment then references the secret with `secretKeyRef`:

```yaml
env:
- name: PAYLOAD_KEY
  valueFrom:
    secretKeyRef:
      name: t-example-secrets
      key: PAYLOAD_KEY
```

## Probe Configuration: TCP vs HTTP

Earlier deployments used raw TCP socket probes:

```yaml
# Older approach — DO NOT USE for new deployments
readinessProbe:
  tcpSocket:
    port: 7000
livenessProbe:
  tcpSocket:
    port: 7000
```

A TCP socket probe only confirms that something is accepting connections on the port. It cannot tell the difference between a server that is fully ready and one that has opened its listener but has not yet finished launching subsystems or connecting to its database. This caused two problems:

- Pods were added to the Service (and started receiving traffic through the Ingress) before they were actually ready to serve requests, producing transient errors during rollouts and restarts.
- A process that was alive on the socket but otherwise stuck still passed the liveness check.

The current approach replaces both TCP probes with HTTP `GET` probes against the system endpoints:

```yaml
# Current approach
readinessProbe:
  httpGet:
    path: /api/system/readiness
    port: 7000
  initialDelaySeconds: 5
  periodSeconds: 10
livenessProbe:
  httpGet:
    path: /api/system/health
    port: 7000
  initialDelaySeconds: 10
  periodSeconds: 30
```

Why this is better:

- Readiness now reflects real application state. `/api/system/readiness` returns `503` until the server reports "READY FOR REQUESTS", so Kubernetes holds traffic back during startup, including while databases connect and migrations run.
- Liveness checks that the HTTP stack itself is responsive, not merely that the TCP port is open.
- The same endpoints are reusable by VPS reverse proxies and external monitors, giving consistent semantics across all deployment shapes.

When migrating an existing deployment, replace each `tcpSocket` block with the corresponding `httpGet` block above and re-apply the manifest. No application configuration change is required, because the endpoints are always present.

## Deployments With a Database

When a database is configured, the server's readiness depends on the database becoming usable. The readiness probe handles this automatically: `/api/system/readiness` returns `503` until every configured database's lead queue manager has connected, bootstrapped, run migrations, and brought up its queues. This means a rolling update will not send traffic to a new pod until its database work is complete.

Database-backed deployments need additional environment variables sourced from the Secret. The reference Lithium deployment, for example, passes database connection details:

```yaml
env:
- name: PAYLOAD_KEY
  valueFrom:
    secretKeyRef:
      name: t-example-secrets
      key: PAYLOAD_KEY
- name: LITHIUM_DB_HOST
  valueFrom:
    secretKeyRef:
      name: t-example-secrets
      key: LITHIUM_DB_HOST
- name: LITHIUM_DB_PORT
  valueFrom:
    secretKeyRef:
      name: t-example-secrets
      key: LITHIUM_DB_PORT
- name: LITHIUM_DB_NAME
  valueFrom:
    secretKeyRef:
      name: t-example-secrets
      key: LITHIUM_DB_NAME
- name: LITHIUM_DB_USER
  valueFrom:
    secretKeyRef:
      name: t-example-secrets
      key: LITHIUM_DB_USER
- name: LITHIUM_DB_PASS
  valueFrom:
    secretKeyRef:
      name: t-example-secrets
      key: LITHIUM_DB_PASS
```

Because the initial connection, migrations, and queue startup can take longer than a plain static site, database-backed deployments add a `startupProbe`. The startup probe gives the pod a generous window to become ready before the liveness probe begins, so a slow first start does not cause Kubernetes to kill the pod mid-migration:

```yaml
startupProbe:
  httpGet:
    path: /api/system/readiness
    port: 7000
  failureThreshold: 60
  periodSeconds: 5
```

With this configuration the pod has up to `failureThreshold * periodSeconds` (here, 300 seconds) to report ready before liveness checks take over. Resource requests and limits are typically higher for database-backed pods than for static ones, and a WebSocket port is exposed when the WebSocket subsystem is enabled:

```yaml
ports:
- containerPort: 7000
  name: http
  protocol: TCP
- containerPort: 7001
  name: websocket
  protocol: TCP
```

The database itself (creating the database and role, and waiting for the engine to accept connections) is provisioned by a separate one-time `Job` rather than by the Hydrogen deployment. Keep database bootstrap concerns out of the application Deployment so that pod restarts do not attempt schema creation. See the [Database Abstraction](/docs/H/core/DATABASE_ABSTRACTION.md) and [Databases](/docs/H/DATABASES.md) documentation for engine specifics.

## Deployments Without a Database

Many Hydrogen deployments serve only static content and the REST API with no database at all — for example a marketing site, a documentation site, or a single-page-application host. These are the simplest deployments and behave as follows:

- No `*_DB_*` environment variables are required. The Secret only needs `PAYLOAD_KEY`.
- `/api/system/readiness` reports `databases_expected: 0` and flips to ready almost immediately, because there is nothing to wait for.
- No `startupProbe` is needed; the readiness and liveness probes alone are sufficient.
- Resource requests can be modest (the reference static deployments request `10m` CPU and `64Mi` memory, with limits of `100m` CPU and `128Mi` memory).

A no-database deployment uses exactly the same probe configuration as any other Hydrogen deployment:

```yaml
readinessProbe:
  httpGet:
    path: /api/system/readiness
    port: 7000
  initialDelaySeconds: 5
  periodSeconds: 10
livenessProbe:
  httpGet:
    path: /api/system/health
    port: 7000
  initialDelaySeconds: 10
  periodSeconds: 30
```

This uniformity is one of the benefits of the HTTP probe model: the same manifest pattern works whether or not a database is involved, and readiness behaves correctly in both cases without per-deployment tuning.

## Reference Tenant Deployments

The Festival cluster runs several Hydrogen tenants that serve as working references for the patterns above. They split into static (no database) and database-backed deployments:

| Type | Notes |
|---|---|
| Static site | `PAYLOAD_KEY` only, HTTP readiness/liveness probes, modest resources |
| Static SPA with data | Static plus an additional served data path; same probe model |
| Database-backed | Adds `*_DB_*` secrets, a `startupProbe`, a WebSocket port, and higher resource limits |

All of them share the same probe pattern (`httpGet` against `/api/system/readiness` and `/api/system/health`), the same CephFS-backed volume layout, the same `domain: Hydrogen` labeling, and the same Traefik plus cert-manager ingress. New tenants should be modeled on whichever existing deployment most closely matches their database requirement.

## Related Documentation

- [Docker Setup Guide](/docs/H/core/deployment/docker.md) - Building a Hydrogen container image
- [Configuration Guide](/docs/H/core/configuration.md) - JSON configuration reference
- [SECRETS.md](/docs/H/SECRETS.md) - Environment variables and secrets management
- [SETUP.md](/docs/H/SETUP.md) - Build and runtime dependencies
- [Databases](/docs/H/DATABASES.md) - Database engine considerations
- [Database Abstraction](/docs/H/core/DATABASE_ABSTRACTION.md) - Multi-engine database layer
- [Shutdown Architecture](/docs/H/core/shutdown_architecture.md) - Graceful shutdown and signal handling
- [API Overview](/docs/H/core/API_OVERVIEW.md) - All REST API endpoints in one place
