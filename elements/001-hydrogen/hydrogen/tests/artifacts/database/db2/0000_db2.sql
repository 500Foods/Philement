CREATE TABLE ACURANZO.queries (
    query_id INTEGER NOT NULL,
    query_ref INTEGER NOT NULL,
    query_type_lua_28 INTEGER NOT NULL,
    query_dialect_lua_30 INTEGER NOT NULL,
    name VARCHAR(100) NOT NULL,
    summary VARCHAR(255),
    query_code VARCHAR(255) NOT NULL,
    query_status_lua_27 INTEGER NOT NULL,
    collection VARCHAR(4000),
    valid_after TIMESTAMP,
    valid_until TIMESTAMP,
    created_id INTEGER NOT NULL,
    created_at TIMESTAMP NOT NULL,
    updated_id INTEGER NOT NULL,
    updated_at TIMESTAMP NOT NULL
);
INSERT INTO ACURANZO.queries (
    query_id,
    query_ref,
    query_type_lua_28,
    query_dialect_lua_30,
    name,
    summary,
    query_code,
    query_status_lua_27,
    collection,
    valid_after,
    valid_until,
    created_id,
    created_at,
    updated_id,
    updated_at
)
VALUES (
    10,
    0,
    1,
    4,
    'Utility: Count pending migrations',
    '
# Utility Query 10
Counts pending migrations in the `queries` table for a given engine.
    ',
    '
SELECT COUNT(*)
FROM ACURANZO.queries
WHERE query_status_lua_27 = 1 AND query_dialect_lua_30 = ?
    ',
    3,
    NULL,
    NULL,
    NULL,
    0,
    CURRENT TIMESTAMP,
    0,
    CURRENT TIMESTAMP
);
INSERT INTO ACURANZO.queries (
    query_id,
    query_ref,
    query_type_lua_28,
    query_dialect_lua_30,
    name,
    summary,
    query_code,
    query_status_lua_27,
    collection,
    valid_after,
    valid_until,
    created_id,
    created_at,
    updated_id,
    updated_at
)
VALUES (
    11,
    0,
    1,
    4,
    'Utility: Fetch next migration',
    '
# Utility Query 11
Fetches the next pending migration from the `queries` table for a given engine.
    ',
    '
SELECT query_id, query_code
FROM ACURANZO.queries
WHERE query_status_lua_27 = 1
  AND query_dialect_lua_30 = ?
ORDER BY query_id ASC
LIMIT 1
    ',
    3,
    NULL,
    NULL,
    NULL,
    0,
    CURRENT TIMESTAMP,
    0,
    CURRENT TIMESTAMP
);

