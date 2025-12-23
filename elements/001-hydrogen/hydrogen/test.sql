INSERT INTO TEST.queries (
    query_id,
    query_ref,
    query_status_a27,
    query_type_a28,
    query_dialect_a30,
    query_queue_a58,
    query_timeout,
    code,
    name,
    summary,
    collection,
    valid_after,
    valid_until,
    created_id,
    created_at,
    updated_id,
    updated_at
)
WITH next_query_id AS (
    SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
    FROM TEST.queries
)
SELECT
new_query_id                                                        AS query_id,
1078                                                        AS query_ref,
1                                                    AS query_status_a27,
1000                                           AS query_type_a28,
4                                                          AS query_dialect_a30,
0                                                         AS query_queue_a58,
5000                                                          AS query_timeout,
TEST.BROTLI_DECOMPRESS(TEST.BASE64DECODEBINARY('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'))
AS code,
'Populate Lookup 045 in lookups table'                    AS name,
TEST.BASE64DECODE('CiMgRm9yd2FyZCBNaWdyYXRpb24gMTA3ODogUG91bGF0ZSBMb29rdXAgMDQ1IC0gQ2xpZW50IFZlcnNpb24gSGlzdG9yeQoKVGhpcyBtaWdyYXRpb24gY3JlYXRlcyB0aGUgbG9va3VwIHZhbHVlcyBmb3IgTG9va3VwIDA0NSAtIENsaWVudCBWZXJzaW9uIEhpc3RvcnkK')
AS summary,
'{}'                                                                AS collection,
NULL            AS valid_after ,
NULL            AS valid_until ,
0               AS created_id ,
CURRENT TIMESTAMP          AS created_at ,
0               AS updated_id ,
CURRENT TIMESTAMP          AS updated_at
FROM next_query_id;