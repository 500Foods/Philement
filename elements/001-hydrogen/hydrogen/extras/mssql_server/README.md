# Microsoft SQL Server Extras

This directory provides convenience scripts and documentation for running
SQL Server 2022 Linux (Developer edition) in a Podman container and
creating test databases for the Acuranzo migration matrix (Test 39).

SQL Server communicates via **TDS** through **unixODBC + Microsoft ODBC Driver 18**.

## Packages (Fedora 43 / dnf)

### unixODBC (Fedora repository)

```bash
sudo dnf install unixODBC unixODBC-devel
```

### Microsoft ODBC Driver 18 (Microsoft repository)

Microsoft packages are not in the Fedora repositories. Add the Microsoft
RHEL 9 repository:

```bash
# Install the Microsoft GPG key
sudo rpm --import https://packages.microsoft.com/keys/microsoft.asc

# Add the Microsoft repository
sudo tee /etc/yum.repos.d/microsoft-prod.repo > /dev/null <<'REPO'
[microsoft-prod]
name=Microsoft Production Repository
baseurl=https://packages.microsoft.com/rpm/9.4/$basearch/
enabled=1
gpgcheck=1
gpgkey=file:///etc/pki/rpm-gpg/microsoft.gpg
REPO

# Install ODBC Driver 18 and sqlcmd
sudo dnf install -y msodbcsql18 mssql-tools18
```

| Package | Why |
| --- | --- |
| `unixODBC` | ODBC Driver Manager |
| `unixODBC-devel` | Headers (`sql.h`, `sqlext.h`) for linking Hydrogen |
| `msodbcsql18` | Microsoft ODBC Driver 18 for SQL Server |
| `mssql-tools18` | `sqlcmd` CLI for health checks and test DB creation |

### FreeTDS (fallback only)

If `msodbcsql18` will not install on Fedora 43, Phase 1 may amend lock 4
to use FreeTDS. This is **not** the preferred path.

```bash
sudo dnf install freetds freetds-devel
```

## Container

SQL Server 2022 Linux runs as a Podman container. There is **no** Fedora
`mssql-server` RPM — the official Linux container is the only local path.

```bash
podman pull mcr.microsoft.com/mssql/server:2022-latest
```

| Setting | Value |
| --- | --- |
| Image | `mcr.microsoft.com/mssql/server:2022-latest` |
| `ACCEPT_EULA` | `Y` |
| `MSSQL_PID` | `Developer` (free for development/test, NOT production) |
| Memory | Minimum 2 GiB, recommended 4 GiB |
| Port | 1433 (TDS) |
| Environment | `MSSQL_SA_PASSWORD` from env (see SECRETS.md) |

### Connection String

```connection
Driver=ODBC Driver 18 for SQL Server;Server=127.0.0.1,1433;Database=hydrotst;UID=sa;PWD=<password>;TrustServerCertificate=yes;
```

Driver 18 defaults to **Encrypt=yes**. The local container must use
`TrustServerCertificate=yes` to accept the self-signed TLS certificate.
See `docs/H/SECRETS.md` for `MSSQL_SA_PASSWORD`.

## Database Files

| File | Env var | Purpose |
| --- | --- | --- |
| `hydrotst` | `MSSQL_TEST_DB` | Acuranzo Test 39 migration isolation database |

`hydrotst` holds two schemas. `testms` is Test 39 and is reversed with that test. `demoms` is the matrix schema for Tests 40, 43, 45, 46, 47, and 58. `create_test_db.sh` creates both.

Example:

```bash
export MSSQL_SA_PASSWORD="<your-password>"
export MSSQL_TEST_DB="hydrotst"
./extras/mssql_server/start.sh
./extras/mssql_server/create_test_db.sh
# ... run Test 39 ...
./extras/mssql_server/stop.sh
```

## Scripts

- `start.sh` — Starts the SQL Server Podman container (idempotent). Waits
  for port 1433. Requires Podman.
- `stop.sh` — Stops the SQL Server container (only if started by this
  setup).
- `create_test_db.sh` — Creates the `hydrotst` database in the running
  container. Requires `MSSQL_SA_PASSWORD`.

## Threat Notes

- **Developer edition is not production.** Document accordingly in
  `docs/H/SECRETS.md` / `docs/H/DATABASES.md`.
- **Never log** the SA password, hashes, or JWTs.
- **Encrypt:** Driver 18 defaults to Encrypt=yes; `TrustServerCertificate=yes`
  is required for the local container.

## Related

- [`docs/H/plans/complete/MSSQL_COMPLETE.md`](/docs/H/plans/complete/MSSQL_COMPLETE.md) — full engine plan
- [`docs/H/SECRETS.md`](/docs/H/SECRETS.md) — environment variable reference
- [`docs/H/DATABASES.md`](/docs/H/DATABASES.md) — database connection reference
