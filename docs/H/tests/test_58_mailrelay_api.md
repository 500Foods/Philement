# Mail Relay API Blackbox Test

Test: `test_58_mailrelay_api.sh`

Validates the Hydrogen Mail Relay REST API (`/api/mailrelay/status`, `/api/mailrelay/preview`, `/api/mailrelay/send`) across all supported database engines, using both plaintext and STARTTLS SMTP delivery against the local C mail validator (`extras/mailval`).

## What It Does

1. Starts the `mailval` SMTP sink (prebuilt binary) listening on `127.0.0.1`.
2. Starts Hydrogen with a config that enables Mail Relay and points outbound delivery at the sink.
3. Waits for HTTP readiness and the canonical `READY FOR REQUESTS` signal.
4. Authenticates as the `mailadmin` account (which has the `mail_send` role) and obtains a JWT.
5. Calls `GET /api/mailrelay/status` and verifies a successful response.
6. Calls `POST /api/mailrelay/preview` with the seeded `mail.test` template and verifies rendered subject, body, and `macros_used`.
7. Calls `POST /api/mailrelay/send` with the same template and a unique idempotency key, and verifies the message is queued.
8. Logs in as a demo user without the `mail_send` role and verifies the send endpoint returns `401`.
9. Stops Hydrogen cleanly, polls the sink capture, and verifies the delivered message subject matches the rendered marker.

## Transport Variants

Each database engine is exercised with two transport variants:

| Variant | Transport | Notes |
| --- | --- | --- |
| Plaintext | `smtp://` | No TLS; fastest path. |
| STARTTLS | `smtp://` + STARTTLS | Self-signed cert from `extras/mailval`; CA pinned via `Servers[0].CAPath`. |

## Database Engines

The test runs in parallel against all configured engines:

- PostgreSQL
- MySQL
- SQLite
- DB2
- MariaDB
- CockroachDB
- YugabyteDB

## Config Files

- `tests/configs/hydrogen_test_58_postgres.json`
- `tests/configs/hydrogen_test_58_mysql.json`
- `tests/configs/hydrogen_test_58_sqlite.json`
- `tests/configs/hydrogen_test_58_db2.json`
- `tests/configs/hydrogen_test_58_mariadb.json`
- `tests/configs/hydrogen_test_58_cockroachdb.json`
- `tests/configs/hydrogen_test_58_yugabytedb.json`

The script overrides the web port, mailval port, and TLS settings in each config at runtime, so a single config file per engine supports both plaintext and STARTTLS variants. Secrets and database connection parameters are injected via `${env.*}` variables resolved by the config loader.

Test 58 uses the dedicated `15800-15831` listener range. These ports are below Linux's default ephemeral client-port range (`32768-60999`), preventing unrelated outbound connections in the full test suite from temporarily occupying a Test 58 listener port.

## Prerequisites

- A built Hydrogen binary (`hydrogen`, `hydrogen_debug`, `hydrogen_release`, or `hydrogen_coverage`) under the project root.
- A prebuilt `mailval` binary at `extras/mailval/build/mailval`. Build it with:

```bash
cd extras/mailval && cmake -S . -B build && cmake --build build
```

- The self-signed cert/key for the TLS variants is generated on demand by `extras/mailval/gen_cert.sh` if missing. The private key must never be committed or logged.
- Environment variables must be set:
  - `HYDROGEN_MAILADMIN_NAME`
  - `HYDROGEN_MAILADMIN_PASS`
  - `HYDROGEN_MAILADMIN_EMAIL`
  - `HYDROGEN_DEMO_USER_NAME`
  - `HYDROGEN_DEMO_USER_PASS`
  - `HYDROGEN_DEMO_API_KEY`
  - `HYDROGEN_DEMO_JWT_KEY`
  - `PAYLOAD_KEY`
  - Server-engine database variables (`ACURANZO_DB_*`) for non-SQLite engines.

## Running

```bash
# From the project root (HYDROGEN_ROOT defined)
zsh -ic 'mkt'   # ensure a current Hydrogen binary exists
bash tests/test_58_mailrelay_api.sh
```

## Assertions

- Hydrogen reaches `READY FOR REQUESTS` for every engine variant.
- Unauthenticated `GET /api/mailrelay/status` returns `401`.
- Authenticated `mailadmin` status, preview, and send requests return `200` with `success:true`.
- Preview returns a rendered subject containing the supplied `%NAME%` value and a `macros_used` array containing `NAME`.
- Send returns a `message_id` and `status: "queued"`.
- A user without the `mail_send` role receives `401` on send.
- The sink writes a `mailval_smtp_*.json` transcript whose `stored_uid` metadata is `yes` and whose `subject` matches the rendered marker.

## Related

- Plan: [Mail Relay Subsystem Implementation Plan](/docs/H/plans/MAILRELAY_PLAN.md)
- Outbound test: [Mail Relay Outbound Blackbox Test](/docs/H/tests/test_57_mailrelay_outbound.md)
- Sink source: `/elements/001-hydrogen/hydrogen/extras/mailval/`
- Phase 7 exit gate: authenticated clients can preview and queue templated emails.
