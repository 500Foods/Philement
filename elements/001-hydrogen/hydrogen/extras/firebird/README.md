# Firebird Extras

This directory provides convenience scripts and documentation for running
a local Firebird 4.0 server (SuperServer) and creating test databases.
Firebird is used as the CockroachDB replacement engine in the Acuranzo
migration matrix (Test 37).

## Packages (Fedora 43 / dnf)

```bash
sudo dnf install firebird libfbclient2 firebird-utils libfbclient2-devel firebird-devel
```

| Package | Why |
| --- | --- |
| `firebird` | SuperServer daemon on port 3050 |
| `libfbclient2` | Runtime client library (`libfbclient.so`) |
| `libfbclient2-devel` / `firebird-devel` | Headers (`ibase.h`) for linking Hydrogen |
| `firebird-utils` | `isql-fb`, `gfix`, `nbackup` CLI tools |
| `libbrotli` | Brotli library for the UDR extra (extras/brotli_udf_firebird) |

Firebird 4.0 on Fedora 43 is **4.0.7.3271**. Firebird 5 packages are only
available on Fedora 44+ and are **not** required.

## SYSDBA Password

The `SYSDBA` user is the Firebird superuser. On Fedora, the default password
is set in `/etc/firebird/firebird.conf` under `RemoteAccess` and the password
database `/var/lib/firebird/aliases.conf`. The default password is typically
`masterkey` for a standalone install.

**Never commit the SYSDBA password.** Set it via an environment variable:

```bash
export FIREBIRD_SYSDBA_PASSWORD="masterkey"
```

See [SECRETS.md](/docs/H/SECRETS.md) for `FIREBIRD_SYSDBA_PASSWORD` and
`FIREBIRD_DB_PATH`.

## SuperServer vs Embedded

| Mode | When | Connection |
| --- | --- | --- |
| **SuperServer (Test 37 / 40 default)** | Networked SQL, like the Cockroach slot | `localhost/3050:/var/lib/firebird/data/testfb.fdb` |
| **Embedded** | Appliance / no daemon | Database path, empty Host — `libfbclient` loads Engine plugin |

Tests lock SuperServer unless Phase 0 amends.

## Database Files

| File | Purpose |
| --- | --- |
| `testfb.fdb` | Acuranzo Test 37 / Test 40 test database |
| `demofb.fdb` | Demo / development database |

The default data directory is `/var/lib/firebird/data/`.

## Scripts

- `/elements/001-hydrogen/hydrogen/extras/firebird/start.sh` — Starts the Firebird service if not running; waits for port 3050.
- `/elements/001-hydrogen/hydrogen/extras/firebird/stop.sh` — Stops the Firebird service if this script started it.
- `/elements/001-hydrogen/hydrogen/extras/firebird/create_test_db.sh` — Creates `testfb.fdb` with the SYSDBA user.

## Database Extensions Table

| Engine | Base64 | Brotli | SHA-256 | JSON ingest |
| --- | --- | --- | --- | --- |
| PostgreSQL | native | C extra | native | plpgsql |
| MySQL | native | plugin extra | native | stored fn |
| SQLite | sqlean | loadable extra | sqlean | passthrough |
| DB2 | UDF extras | UDF extra | UDF extra | SQL UDF |
| **Firebird** | native `BASE64_ENCODE` / `BASE64_DECODE` | UDR extra | native `CRYPT_HASH(... USING SHA256)` | PSQL function or UDR |

## Usage

### Quick Start (Test 37 / Test 40)

```bash
export FIREBIRD_SYSDBA_PASSWORD="masterkey"
./start.sh
./create_test_db.sh
# ... run Test 37 or Test 40 ...
./stop.sh
```

### Manual Connection

```bash
isql-fb -user SYSDBA -password "$FIREBIRD_SYSDBA_PASSWORD" localhost/3050:/var/lib/firebird/data/testfb.fdb
```

## Related

- [FIREBIRD.md](/docs/H/plans/FIREBIRD.md) — full engine plan
- [SECRETS.md](/docs/H/SECRETS.md) — environment variable reference
- `brotli_udf_firebird/` — Brotli decompression UDR (Phase 6, not yet created)
