CREATE TABLE test.queries (
    query_id                integer          NOT NULL,
    query_ref               integer          NOT NULL,
    query_status_a27        integer          NOT NULL,
    query_type_a28          integer          NOT NULL,
    query_dialect_a30       integer          NOT NULL,
    query_queue_a58         integer          NOT NULL,
    query_timeout           integer          NOT NULL,
    code                    text         NOT NULL,
    name                    text             NOT NULL,
    summary                 text                 ,
    collection              jsonb                     ,
    valid_after             timestamptz             ,
    valid_until             timestamptz             ,
    created_id              integer          NOT NULL,
    created_at              timestamptz     NOT NULL,
    updated_id              integer          NOT NULL,
    updated_at              timestamptz     NOT NULL,
    PRIMARY KEY(query_id),                  -- Primary Key
    UNIQUE(query_ref, query_type_a28)       -- Unique Column
);
-- QUERY DELIMITER
-- Defined in database_<engine>.lua as a macro
CREATE OR REPLACE FUNCTION test.json_ingest(s text)
RETURNS jsonb
LANGUAGE plpgsql
STRICT
STABLE
AS $json_ingest_fn$
DECLARE
i      int := 1;
L      int := length(s);
ch     text;
out    text := '';
in_str boolean := false;
esc    boolean := false;
BEGIN
-- Fast path: already valid JSON
BEGIN
RETURN s::jsonb;
EXCEPTION WHEN others THEN
-- fall through to fix-up pass
END;
-- Fix-up pass: escape only control chars *inside* JSON strings
WHILE i <= L LOOP
ch := substr(s, i, 1);
IF esc THEN
out := out || ch;
esc := false;
ELSIF ch = E'\\' THEN
out := out || ch;
esc := true;
ELSIF ch = '"' THEN
out := out || ch;
in_str := NOT in_str;
ELSIF in_str AND ch = E'\n' THEN
out := out || E'\\n';
ELSIF in_str AND ch = E'\r' THEN
out := out || E'\\r';
ELSIF in_str AND ch = E'\t' THEN
out := out || E'\\t';
ELSE
out := out || ch;
END IF;
i := i + 1;
END LOOP;
-- Parse after fix-ups
RETURN out::jsonb;
END
$json_ingest_fn$;                       -- ← Tag matches exactly, semicolon attached
-- QUERY DELIMITER
-- PostgreSQL C extension for Brotli decompression
-- Requires: libbrotlidec and brotli_decompress.so in PostgreSQL lib directory
-- Installation handled via extras/brotli_udf_postgresql/
CREATE OR REPLACE FUNCTION test.brotli_decompress(compressed bytea)
RETURNS text
AS 'brotli_decompress', 'brotli_decompress'
LANGUAGE c STRICT IMMUTABLE;
-- QUERY DELIMITER
INSERT INTO test.queries (
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
    FROM test.queries
)
SELECT
new_query_id                                                        AS query_id,
1000                                                        AS query_ref,
1                                                    AS query_status_a27,
1003                                           AS query_type_a28,
1                                                          AS query_dialect_a30,
0                                                         AS query_queue_a58,
5000                                                          AS query_timeout,
test.brotli_decompress(DECODE('GyQEAIzDuLG7lFiVtnqpQRSc7EIA80r14oy8RKQ9mKSJc+FUKoCTh5GeIZZCFTzcDWQ4en7dJiJ0RERXWWjTN4SycgbsZgfT0ZPzQoU/5/0DIaBGteW8A/zAt1cmCrjvDyX9x8qOos+ypd5RxFvWlu/YzC7PBUC+OxBtWduGiy2LLWs3AEfHEl5eT3DswwSO8eY0JwhMDSd/PMLHu2AahrjWvz7A9HYggF7oxDr+6XUD0Fx2MZcu8QwT+q+rz79ET4KLcbp9MHjXe7mRpkXcWj85wNzqwXATcngLqkLuZ6s0mQWMZ+I8708xhqDuxMQ4', 'base64'))
AS code,
'Create queries Table'                                             AS name,
CONVERT_FROM(DECODE('CiMgRm9yd2FyZCBNaWdyYXRpb24gMTAwMDogQ3JlYXRlIHF1ZXJpZXMgVGFibGUKClRoaXMgaXMgdGhlIGZpcnN0IG1pZ3JhdGlvbiB0aGF0LCB0ZWNobmljYWxseSwgaXMgcnVuIGF1dG9tYXRpY2FsbHkKd2hlbiBjb25uZWN0aW5nIHRvIGFuIGVtcHR5IGRhdGFiYXNlIGFuZCBraWNrcyBvZmYgdGhlIG1pZ3JhdGlvbiwKc28gbG9uZyBhcyB0aGUgZGF0YWJhc2UgaGFzIGJlZW4gY29uZmlndXJlZCB3aXRoIEF1dG9NaWdyYXRpb246IHRydWUKaW4gaXRzIGNvbmZpZyAodGhpcyBpcyB0aGUgZGVmYXVsdCBpZiBub3Qgc3VwcGxpZWQpLgo=', 'base64'), 'UTF8')
AS summary,
'{}'                                                                AS collection,
NULL            AS valid_after ,
NULL            AS valid_until ,
0               AS created_id ,
CURRENT_TIMESTAMP          AS created_at ,
0               AS updated_id ,
CURRENT_TIMESTAMP          AS updated_at
FROM next_query_id;
-- QUERY DELIMITER
INSERT INTO test.queries (
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
    FROM test.queries
)
SELECT
new_query_id                                                        AS query_id,
1000                                                        AS query_ref,
1                                                    AS query_status_a27,
1001                                           AS query_type_a28,
1                                                          AS query_dialect_a30,
0                                                         AS query_queue_a58,
5000                                                          AS query_timeout,
CONVERT_FROM(DECODE('CkRFTEVURSBGUk9NIHRlc3QucXVlcmllcwpXSEVSRSBxdWVyeV90eXBlX2EyOCBJTiAoMTAwMCwgMTAwMSwgMTAwMiwgMTAwMyk7CgotLSBTVUJRVUVSWSBERUxJTUlURVIKClNFTEVDVCBwZ19jYXRhbG9nLnBnX3Rlcm1pbmF0ZV9iYWNrZW5kKHBnX2JhY2tlbmRfcGlkKCkpIEZST00gdGVzdC5xdWVyaWVzIExJTUlUIDE7CgotLSBTVUJRVUVSWSBERUxJTUlURVIKCkRST1AgVEFCTEUgdGVzdC5xdWVyaWVzOwo=', 'base64'), 'UTF8')
AS code,
'Drop queries Table'                                               AS name,
CONVERT_FROM(DECODE('CiMgUmV2ZXJzZSBNaWdyYXRpb24gMTAwMDogRHJvcCBxdWVyaWVzIFRhYmxlCgpUaGlzIGlzIHByb3ZpZGVkIGZvciBjb21wbGV0ZW5lc3Mgd2hlbiB0ZXN0aW5nIHRoZSBtaWdyYXRpb24gc3lzdGVtCnRvIGVuc3VyZSB0aGF0IGZvcndhcmQgYW5kIHJldmVyc2UgbWlncmF0aW9ucyBhcmUgY29tcGxldGUuCg==', 'base64'), 'UTF8')
AS summary,
'{}'                                                                  AS collection,
NULL            AS valid_after ,
NULL            AS valid_until ,
0               AS created_id ,
CURRENT_TIMESTAMP          AS created_at ,
0               AS updated_id ,
CURRENT_TIMESTAMP          AS updated_at
FROM next_query_id;
-- QUERY DELIMITER
cd 