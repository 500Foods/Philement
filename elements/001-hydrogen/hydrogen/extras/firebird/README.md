# Firebird Extras

This directory provides convenience scripts and documentation for running
a local Firebird 4.0 server (SuperServer) and creating test databases.
Firebird is used as the replacement engine in the Acuranzo
migration matrix (Test 37) and demo suites (Test 40+).

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

See [SECRETS.md](/docs/H/SECRETS.md) for `FIREBIRD_SYSDBA_PASSWORD`,
`FIREBIRD_DB_PATH_TEST`, and `FIREBIRD_DB_PATH_DEMO`.
(`FIREBIRD_DB_PATH` singular is deprecated — prefer the dual vars.)

## SuperServer vs Embedded

| Mode | When | Connection |
| --- | --- | --- |
| **SuperServer (Test 37 / 40)** | Networked SQL, like the Firebird slot | `localhost/3050:/path/to/hydrogen_test.fdb` |
| **Embedded** | Appliance / no daemon | Database path, empty Host — `libfbclient` loads Engine plugin |

Tests lock SuperServer unless Phase 0 amends.

## Database Files

| File | Env var | Purpose |
| --- | --- | --- |
| `hydrogen_test.fdb` | `FIREBIRD_DB_PATH_TEST` | Acuranzo Test 37 migration isolation DB |
| `hydrogen_demo.fdb` | `FIREBIRD_DB_PATH_DEMO` | Demo / Test 40+ (auth, scripting, OIDC, …) |

Default location when env vars are unset:

`${HYDROGEN_ROOT}/tests/artifacts/database/firebird/`

(Old defaults `testfb.fdb` / `demofb.fdb` are retired.)

Example (Andrew):

```bash
export FIREBIRD_DB_PATH_DEMO="/mnt/extra/Projects/Philement/elements/001-hydrogen/hydrogen/tests/artifacts/database/firebird/hydrogen_demo.fdb"
export FIREBIRD_DB_PATH_TEST="/mnt/extra/Projects/Philement/elements/001-hydrogen/hydrogen/tests/artifacts/database/firebird/hydrogen_test.fdb"
```

## Scripts

- `start.sh` — Starts the Firebird systemd service. **Requires `sudo`** (the
  scripts use `sudo` internally to call `systemctl start firebird`).
- `stop.sh` — Stops the Firebird systemd service. **Requires `sudo`**.
- `create_test_db.sh` — Creates `hydrogen_test.fdb` and/or `hydrogen_demo.fdb`
  (modes: `both` default, `test`, `demo`, or a one-off `.fdb` path). Prefer
  `FIREBIRD_DB_PATH_TEST` / `FIREBIRD_DB_PATH_DEMO`. Run via `sudo` or
  `run_create.sh` (pkexec).
- `run_create.sh` — pkexec wrapper; forwards dual env vars and mode args.

Creating **test** only does **not** drop/recreate **demo** (and vice versa).

## Setup

After installing the packages, verify the service and user exist:

```bash
sudo systemctl status firebird       # or firebird-superserver
id firebird
```

If the service is named differently (e.g. `firebird-superserver`), the scripts
auto-detect it.

## Usage

### Quick Start (both DBs)

```bash
export HYDROGEN_ROOT="/path/to/hydrogen"
export FIREBIRD_SYSDBA_PASSWORD="masterkey"
export FIREBIRD_DB_PATH_TEST="…/tests/artifacts/database/firebird/hydrogen_test.fdb"
export FIREBIRD_DB_PATH_DEMO="…/tests/artifacts/database/firebird/hydrogen_demo.fdb"
./start.sh
# both (default):
./run_create.sh
# or: sudo ./create_test_db.sh both
# test only / demo only:
./run_create.sh test
./run_create.sh demo
# ... run Test 37 (TEST) or Test 40+ (DEMO) ...
./stop.sh
```

### Manual Connection

```bash
isql-fb -user SYSDBA -password "$FIREBIRD_SYSDBA_PASSWORD" \
  "localhost/3050:${FIREBIRD_DB_PATH_TEST}"
isql-fb -user SYSDBA -password "$FIREBIRD_SYSDBA_PASSWORD" \
  "localhost/3050:${FIREBIRD_DB_PATH_DEMO}"
```

## Related

- [FIREBIRD.md](/docs/H/plans/FIREBIRD.md) — full engine plan
- [SECRETS.md](/docs/H/SECRETS.md) — environment variable reference
- `brotli_udf_firebird/` — Brotli decompression UDR (Phase 6)
- `json_udf_firebird/` — JSON_VALUE extraction UDR (Phase 6)
