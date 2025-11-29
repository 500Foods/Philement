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
1001                                                        AS query_ref,
1                                                    AS query_status_a27,
1000                                           AS query_type_a28,
3                                                          AS query_dialect_a30,
0                                                         AS query_queue_a58,
5000                                                          AS query_timeout,
brotli_decompress(FROM_BASE64('GxUEAIzEdGuHHJTjg1iEZMG6+pQ08YQMJj6Y9gHCBhxSnvjAa37PqRyLWIT38+hYBOvfZbcbW1zegGQLlSZRVecy8TNBD0kVhHav6QqeAZM7qLBJB/368749oYAkzxeyX7FK+148Ne3AEFaCRen4v+xXP5K745LMXRpOmLx+L8jcaPSgpvsL7k4poMuv5Lm+jwkCizq7g7xls7kYrJPJ5Jj4yZ9i6oVY0EiGkRWKMvV5jCfOGZstTay1mfsE+BPzF1899WA6LB4oy4eA3H3Ze1lMFZZtDPYcVKSsFnDD1MaUY178f/Sl6AEOHjNdkU3E/bpF9osBFa8WLXuQuScDWT76MBqF28+P9aapcmFW81aQ/Ro='))
AS code,
'Create lookups Table'                                             AS name,
cast(FROM_BASE64('CiMgRm9yd2FyZCBNaWdyYXRpb24gMTAwMTogQ3JlYXRlIGxvb2t1cHMgVGFibGUKClRoaXMgbWlncmF0aW9uIGNyZWF0ZXMgdGhlIGxvb2t1cHMgdGFibGUgZm9yIHN0b3Jpbmcga2V5LXZhbHVlIGxvb2t1cCBkYXRhLgo=') as char character set utf8mb4)
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
1001                                                        AS query_ref,
1                                                    AS query_status_a27,
1001                                           AS query_type_a28,
3                                                          AS query_dialect_a30,
0                                                         AS query_queue_a58,
5000                                                          AS query_timeout,
cast(FROM_BASE64('CiBETyBJRihFWElTVFMoU0VMRUNUIDEgRlJPTSB0ZXN0Lmxvb2t1cHMpLCBjYXN0KCdSZWZ1c2luZyB0byBkcm9wIHRhYmxlIHRlc3QubG9va3VwcyDigJMgaXQgY29udGFpbnMgZGF0YScgQVMgQ0hBUigwKSksIE5VTEwpOwoKLS0gU1VCUVVFUlkgREVMSU1JVEVSCgpEUk9QIFRBQkxFIHRlc3QubG9va3VwczsKCi0tIFNVQlFVRVJZIERFTElNSVRFUgoKVVBEQVRFIHRlc3QucXVlcmllcwogIFNFVCBxdWVyeV90eXBlX2EyOCA9IDEwMDAKV0hFUkUgcXVlcnlfcmVmID0gMTAwMQogIGFuZCBxdWVyeV90eXBlX2EyOCA9IDEwMDM7Cg==') as char character set utf8mb4)
AS code,
'Drop lookups Table'                                               AS name,
cast(FROM_BASE64('CiMgUmV2ZXJzZSBNaWdyYXRpb24gMTAwMTogRHJvcCBsb29rdXBzIFRhYmxlCgpUaGlzIGlzIHByb3ZpZGVkIGZvciBjb21wbGV0ZW5lc3Mgd2hlbiB0ZXN0aW5nIHRoZSBtaWdyYXRpb24gc3lzdGVtCnRvIGVuc3VyZSB0aGF0IGZvcndhcmQgYW5kIHJldmVyc2UgbWlncmF0aW9ucyBhcmUgY29tcGxldGUuCg==') as char character set utf8mb4)
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
1001                                                        AS query_ref,
1                                                    AS query_status_a27,
1002                                           AS query_type_a28,
3                                                          AS query_dialect_a30,
0                                                         AS query_queue_a58,
5000                                                          AS query_timeout,
'JSON Table Definition in collection'                               AS code,
'Diagram Tables: test.lookups'                                 AS name,
cast(FROM_BASE64('CiMgRGlhZ3JhbSBNaWdyYXRpb24gMTAwMQoKIyMgRGlhZ3JhbSBUYWJsZXM6IHRlc3QubG9va3VwcwoKVGhpcyBpcyB0aGUgZmlyc3QgSlNPTiBEaWFncmFtIGNvZGUgZm9yIHRoZSBsb29rdXBzIHRhYmxlLgo=') as char character set utf8mb4)
AS summary,
-- DIAGRAM_START
test.json_ingest(
brotli_decompress(FROM_BASE64('Gw4QABwHdizmcvNEKnNSKrDomNtK/UJyLRsOYROGMPSdoHyDiHT19elPVY94AEPyMiaNjeEArtU1KVQh+gNrtV6tD3XWbcTwcyU/SIKEWAXKz1rtVC6LuMoxRHgbwUWuA/7lwJozN3T6We9fSSnSKFG0MzUgc9rFChtYlVSRkT0CRvm1oqzyyA3CoOZDJu+QWiVVwWEPqmDAqNBLi7G7e+orbDLsTW/N7RVe/To2aQA9Yh0OqHGkgrbZMSKLsWTmTcj8yB7ZI6khpLTiPT60Rwd7/Le4bfH4/DXsDt+wrmqWGbSMAdQcA1JQ0nC3Rs/wWdiMmfGX8bovc6WXN3rUnX324I0Y4wfUyC49N1aqU0+Wy8t2pDyvJatwTfs7nqkrWt6p2127uRhAfHMP'))
)
-- DIAGRAM_END
AS collection,
NULL            AS valid_after ,
NULL            AS valid_until ,
0               AS created_id ,
CURRENT_TIMESTAMP          AS created_at ,
0               AS updated_id ,
CURRENT_TIMESTAMP          AS updated_at
FROM next_query_id;

