# JSON Value Extraction UDR for Firebird

This directory contains the Firebird User-Defined Routine (UDR) for JSON
value extraction (`JSON_VALUE`), used because Firebird 4 has no native
`JSON_VALUE` function (that was added in Firebird 6).

## Overview

The `${JRS}` / `${JRM}` / `${JRE}` macros in `database_firebird.lua`
expand to `JSON_VALUE(col, '$.path' DEFAULT NULL ON ERROR)`. This UDR
provides the `JSON_VALUE` function so those QueryRefs work on Firebird
SuperServer.

The UDR uses the **jansson** JSON library (already linked by Hydrogen
for its own JSON parsing) to parse JSON documents and extract values at
the given path. Paths use SQL/JSON-style dot notation with a required
`$.` prefix:

- `$.key` — extract object value
- `$.key.subkey` — nested extraction
- `$.key[0]` — array index

Missing paths return SQL `NULL`, matching the `DEFAULT NULL ON ERROR`
semantics in the macros.

## Files

| File | Description |
| --- | --- |
| `json_value.cpp` | C++ source implementing the `json_value` UDR function |
| `Makefile` | Build and installation automation |
| `README.md` | This file |
| `test_json.sql` | SQL test fixture (run with `isql-fb`) |

## Dependencies

- **Firebird 4.0** development headers (`firebird-devel`)
- **libjansson** JSON library (`jansson-devel` on Fedora)
- A C++ compiler (`g++`)

### Installing Dependencies (Fedora)

```bash
sudo dnf install firebird-devel jansson-devel gcc-c++
```

## Building

```bash
make clean
make
```

This produces `json_udfn.so` — the shared library loaded by Firebird's
UDR engine at runtime.

## Installation

```bash
sudo make install
```

This copies `json_udfn.so` to the Firebird UDR plugin directory
(default: `/plugins/udr`).

## Registering the Function

The UDR is registered by migration `acuranzo_1000.lua` (and
`gaius_2000`/`helium_4000`/`glm_3000`) via the `${JSON_VALUE_FUNCTION}`
macro from `database_firebird.lua`:

```sql
CREATE OR ALTER FUNCTION JSON_VALUE(json_doc BLOB SUB_TYPE TEXT, json_path VARCHAR(255))
RETURNS BLOB SUB_TYPE TEXT
EXTERNAL NAME 'json_udfn!json_value'
ENGINE UDR;
```

**Important:** This function must be created **before** `json_ingest`
(the PSQL function), because `json_ingest` calls `JSON_VALUE` internally
for validity checking.

## Usage in Migrations

After registration, QueryRefs that use `${JRS}`/`${JRE}` work normally:

```sql
-- ${JRS}${TABLE}.icon${JRE} expands to:
JSON_VALUE(query.icon, '$.icon' DEFAULT NULL ON ERROR)
```

## Testing

`test_json.sql` contains fixtures for:

- Valid JSON with simple key extraction
- Nested key extraction
- Missing path → NULL result
- NULL input → NULL result

Run with:

```bash
isql-fb -user SYSDBA -password <password> testfb.fdb < test_json.sql
```

## How It Works

The UDR reads the JSON document BLOB into memory, parses it with jansson,
navigates the path (supporting `$.key`, `$.key.subkey`, and `$.key[N]`
array indexing), and serializes the result value back to JSON text as
a BLOB. If any path component is not found, the result is SQL NULL.

## Relationship to json_ingest

The PSQL `json_ingest` function (defined in `database_firebird.lua`)
calls `JSON_VALUE(s, '$' DEFAULT NULL ON ERROR)` to test whether
the input is valid JSON on the fast path. This means the JSON_VALUE UDR
must be available before `json_ingest` is created.

Migration 1000 emits `${JSON_VALUE_FUNCTION}` for firebird **before**
`${JSON_INGEST_FUNCTION}`, ensuring correct ordering.
