# Database Parameter Binding by Engine

Flow for every engine:

1. `parse_typed_parameters()` — JSON → `ParameterList`
2. `convert_named_to_positional()` — `:name` → placeholders + ordered array
3. Engine bind / execute
4. Free parameter list and any engine-owned bind buffers

Types and JSON format: [PARAMETER_TYPES.md](/docs/H/database/PARAMETER_TYPES.md).

## Placeholders

| Engine | Placeholder style | Primary execute path |
| --- | --- | --- |
| PostgreSQL, CockroachDB, YugabyteDB | `$1`, `$2`, … | `PQexecParams` / prepared with param arrays |
| MySQL, MariaDB | `?` | Prepared statement + `MYSQL_BIND` |
| SQLite | `?` | `sqlite3_prepare_v2` + bind_* |
| DB2 | `?` | `SQLPrepare` + `SQLBindParameter` |

## PostgreSQL family

Source: [`postgresql/query.c`](/elements/001-hydrogen/hydrogen/src/database/postgresql/query.c)

- `postgresql_convert_param_value()` turns each `TypedParameter` into a heap
  C string (or `NULL` for SQL NULL).
- `PQexecParams` / `PQexecPrepared` receive text-format values.
- BOOLEAN → `"true"` / `"false"`; numbers formatted with `snprintf`.

## MySQL / MariaDB

Source: [`mysql/query.c`](/elements/001-hydrogen/hydrogen/src/database/mysql/query.c)

- `mysql_bind_single_parameter()` fills `MYSQL_BIND` slots.
- INTEGER → `MYSQL_TYPE_LONGLONG`; BOOLEAN → short; FLOAT → double;
  STRING → string; TEXT → long blob; DATE/TIME/DATETIME/TIMESTAMP →
  `MYSQL_TIME` parsed from ISO text.
- Null string-like values bind as empty text when the hand-rolled
  `MYSQL_BIND` layout cannot set client `is_null` safely.
- `mysql_cleanup_bound_values()` frees allocated bind storage after execute.

## SQLite

Source: [`sqlite/query.c`](/elements/001-hydrogen/hydrogen/src/database/sqlite/query.c)

- `sqlite_bind_single_parameter()` on a prepared statement (1-based index).
- INTEGER/BOOLEAN → `sqlite3_bind_int`; FLOAT → `bind_double`;
  STRING/TEXT and temporal types → `bind_text` with `SQLITE_TRANSIENT`;
  `is_null` → `bind_null`.
- Result rows are stepped and serialized to the same JSON shape as other
  engines (no `sqlite3_exec` callback path for parameterized queries).

## DB2

Source: [`db2/query.c`](/elements/001-hydrogen/hydrogen/src/database/db2/query.c)

- `db2_bind_single_parameter()` uses CLI `SQLBindParameter`.
- TEXT → `SQL_LONGVARCHAR`; DATE/TIME/TIMESTAMP structs from parsed ISO
  strings; DATETIME/TIMESTAMP as `SQL_TYPE_TIMESTAMP` (fraction for
  TIMESTAMP only).
- Bound buffers live in `bound_values` until
  `db2_cleanup_bound_values()`.

## Parameterless queries

Engines keep a direct-execution fallback when there is no parameter JSON
or conversion yields zero binds, so existing SQL without `:name` markers
continues to work.

## Example (conceptual)

```text
JSON:  {"INTEGER":{"userId":1},"STRING":{"status":"active"}}
SQL:   SELECT * FROM users WHERE id = :userId AND status = :status

PostgreSQL SQL:  ... WHERE id = $1 AND status = $2
Others SQL:      ... WHERE id = ? AND status = ?

ordered_params: [userId INTEGER 1, status STRING "active"]
```

## Tests

- Unity per engine: `query_test_postgresql_execute_params`,
  `query_test_mysql_execute_params`, `query_test_sqlite_execute_params`,
  `query_test_db2_extended_types`
- Shared parse/convert: `database_params_test`
- Integration: Test 40 auth, Tests 50–55 conduit
