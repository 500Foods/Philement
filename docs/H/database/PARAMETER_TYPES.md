# Database Parameter Types

Hydrogen binds query parameters through typed JSON, not positional `?` lists
in API payloads. SQL templates use named markers (`:userId`); the database
layer parses JSON, orders binds by appearance in SQL, and applies
engine-specific placeholders and native bind APIs.

Implementation: [`database_params.h`](/elements/001-hydrogen/hydrogen/src/database/database_params.h),
[`database_params.c`](/elements/001-hydrogen/hydrogen/src/database/database_params.c).
Engine binding notes: [PARAMETER_BINDING.md](/docs/H/database/PARAMETER_BINDING.md).
Historical plan: [DATABASE_UPDATE_PLAN_COMPLETE.md](/docs/H/plans/complete/DATABASE_UPDATE_PLAN_COMPLETE.md).

## JSON shape

Parameters are an object whose keys are type group names. Each group is an
object of parameter name → value. Omitted groups are ignored. Empty object
yields zero parameters (still valid).

```json
{
  "INTEGER": {
    "userId": 12345,
    "maxResults": 10
  },
  "STRING": {
    "username": "john_doe",
    "status": "active"
  },
  "BOOLEAN": {
    "verified": true
  },
  "FLOAT": {
    "score": 98.6
  },
  "TEXT": {
    "body": "long text…"
  },
  "DATE": {
    "startDate": "2026-01-14"
  },
  "TIME": {
    "startTime": "14:30:00"
  },
  "DATETIME": {
    "createdAt": "2026-01-14 14:30:00"
  },
  "TIMESTAMP": {
    "eventAt": "2026-01-14 14:30:00.123"
  }
}
```

JSON `null` for a named value sets `TypedParameter.is_null` so engines bind
SQL NULL (MySQL may approximate null string-likes as empty text; see binding
doc).

## Supported types

| Type key | C enum | Value form | Notes |
| --- | --- | --- | --- |
| `INTEGER` | `PARAM_TYPE_INTEGER` | JSON number → `long long` | SQLite bind uses 32-bit `int` today |
| `STRING` | `PARAM_TYPE_STRING` | JSON string | General varchar-style |
| `BOOLEAN` | `PARAM_TYPE_BOOLEAN` | JSON true/false | Bound as 0/1 or `true`/`false` by engine |
| `FLOAT` | `PARAM_TYPE_FLOAT` | JSON number → `double` | |
| `TEXT` | `PARAM_TYPE_TEXT` | JSON string | Large / CLOB-oriented; same storage as string in C union |
| `DATE` | `PARAM_TYPE_DATE` | `YYYY-MM-DD` string | |
| `TIME` | `PARAM_TYPE_TIME` | `HH:MM:SS` string | |
| `DATETIME` | `PARAM_TYPE_DATETIME` | `YYYY-MM-DD HH:MM:SS` | No fractional seconds |
| `TIMESTAMP` | `PARAM_TYPE_TIMESTAMP` | `YYYY-MM-DD HH:MM:SS.fff` | Milliseconds in text form |

## Named SQL markers

Templates use `:name` (identifier after `:`). The same name may appear more
than once; each occurrence gets its own positional slot with the same value.
Markers inside SQL string literals are not replaced.

Example template:

```sql
SELECT id FROM users
 WHERE status = :status
   AND id IN (SELECT id FROM users WHERE status = :status)
 LIMIT :maxResults
```

## Call sites

- Auth database helpers build typed JSON for QueryRefs (login, API key, etc.).
- Conduit `/api/conduit/query`, `/queries`, `/auth_query`, `/auth_queries`
  accept the same parameter object on the request body.
- Lua host API maps script tables into this typed JSON before submit.

## Related tests

- Unity: `database_params_test`, `query_test_*_execute_params`,
  `query_test_db2_extended_types`
- Blackbox: Test 40 (auth), Tests 50–55 (conduit parameter paths)
