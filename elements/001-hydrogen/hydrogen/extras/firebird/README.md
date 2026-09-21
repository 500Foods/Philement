# Firebird Extras

This directory provides convenience scripts and documentation for running
a local Firebird 4.0 server (SuperServer) and creating test databases.
Firebird is used as the replacement engine in the Acuranzo
migration matrix (Test 37).

## Packages (Fedora 43 / dnf)

```bash
sudo dnf install firebird firebird-utils firebird-devel libfbclient2 libfbclient2-devel
```

| Package | Why |
| --- | --- |
| `firebird` | SuperServer daemon on port 3050 |
| `libfbclient2` | Runtime client library (`libfbclient.so`) |
| `libfbclient2-devel` / `firebird-devel` | Headers (`ibase.h`) for linking Hydrogen |
| `firebird-utils` | `isql-fb`, `gfix`, `nbackup` CLI tools |

The `firebird` package creates a **system user** named `firebird` and a **systemd
service** named `firebird` (or `firebird-superserver` on some distributions).
Both are required by the scripts in this directory.

Firebird 4.0 on Fedora 43 is **4.0.7.3271**. Firebird 5 packages are only
available on Fedora 44+ and are **not** required.

## SYSDBA Password

The `SYSDBA` user is the Firebird superuser. On Fedora, the default password
is `masterkey`.

**Never commit the SYSDBA password.** Set it via an environment variable:

```bash
export FIREBIRD_SYSDBA_PASSWORD="masterkey"
```

See [SECRETS.md](/docs/H/SECRETS.md) for `FIREBIRD_SYSDBA_PASSWORD` and
`FIREBIRD_DB_PATH`.

## SuperServer vs Embedded

| Mode | When | Connection |
| --- | --- | --- |
| **SuperServer (Test 37 / 40)** | Networked SQL, like the Firebird slot | `localhost/3050:/var/lib/firebird/data/testfb.fdb` |
| **Embedded** | Appliance / no daemon | Database path, empty Host — `libfbclient` loads Engine plugin |

Tests lock SuperServer unless Phase 0 amends.

## Database Files

| File | Purpose |
| --- | --- |
| `testfb.fdb` | Acuranzo Test 37 / Test 40 test database |
| `demofb.fdb` | Demo / development database |

The default data directory is `/var/lib/firebird/data/`.

## Scripts

- `start.sh` — Starts the Firebird systemd service. **Requires `sudo`** (the
  scripts use `sudo` internally to call `systemctl start firebird`).
- `stop.sh` — Stops the Firebird systemd service. **Requires `sudo`**.
- `create_test_db.sh` — Creates `testfb.fdb` and grants SYSDBA privileges.
  Run as `sudo` or as a user with write access to `/var/lib/firebird/data/`.

## Setup

After installing the packages, verify the service and user exist:

```bash
sudo systemctl status firebird       # or firebird-superserver
id firebird
```

If the service is named differently (e.g. `firebird-superserver`), the scripts
auto-detect it.

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
- `brotli_udf_firebird/` — Brotli decompression UDR (Phase 6)
- `json_udf_firebird/` — JSON_VALUE extraction UDR (Phase 6)
