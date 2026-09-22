# JSON Value Extraction UDR for Firebird 4

Firebird 4 has no native `JSON_VALUE` (added in Firebird 6). This UDR
provides it for Hydrogen / Helium Firebird QueryRefs and for the
`json_ingest` PSQL helper in `database_firebird.lua`.

## Build artifact

| File | Description |
| --- | --- |
| `libjson_udfn.so` | Shared library Firebird loads for module `json_udfn` |
| `json_value.cpp` | C++ UDR implementation (jansson) |
| `Makefile` | Build / install |
| `test_json.sql` | isql-fb smoke test |

Firebird resolves `EXTERNAL NAME 'json_udfn!json_value'` to:

```text
<firebird>/plugins/udr/libjson_udfn.so
```

On Fedora/RHEL packages that is typically
`/usr/lib64/firebird/plugins/udr/libjson_udfn.so`.

## Dependencies

```bash
sudo dnf install firebird-devel jansson-devel gcc-c++
```

## Build

```bash
cd extras/json_udf_firebird
make clean && make
```

## Install (requires root)

```bash
sudo make install
# or explicitly:
# sudo install -m 755 libjson_udfn.so /usr/lib64/firebird/plugins/udr/
```

No Firebird restart is required for a new `.so`, but an already-failed
`CREATE FUNCTION` may need `CREATE OR ALTER` again after install.

## Register

```sql
CREATE OR ALTER FUNCTION JSON_VALUE(
    json_doc BLOB SUB_TYPE TEXT,
    json_path VARCHAR(255)
)
RETURNS BLOB SUB_TYPE TEXT
EXTERNAL NAME 'json_udfn!json_value'
ENGINE UDR;
```

Migration `acuranzo_1000` (and siblings) emit this via
`${JSON_VALUE_FUNCTION}` before `${JSON_INGEST_FUNCTION}`.

## Path semantics

| Path | Result |
| --- | --- |
| `$` | Whole document (serialized JSON, or string content for string roots) |
| `$.key` | Object member |
| `$.key.sub` | Nested member |
| `$.arr[0]` | Array element |
| missing / invalid JSON | SQL `NULL` |

## Smoke test

```bash
sudo make install
isql-fb -user SYSDBA -password masterkey localhost:/path/to/db.fdb -i test_json.sql
```
