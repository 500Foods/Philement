INSERT INTO DEMO.queries (
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
    FROM DEMO.queries
)
SELECT
new_query_id                                                        AS query_id,
1144                                                        AS query_ref,
1                                                    AS query_status_a27,
1000                                           AS query_type_a28,
4                                                          AS query_dialect_a30,
0                                                         AS query_queue_a58,
5000                                                          AS query_timeout,
DEMO.BROTLI_DECOMPRESS(DEMO.BASE64DECODEBINARY('GxYQAIzDuPE6iJESL5tKAWRRt7/PFbzdHPXgcgUxtf/m0h601iBKd/NnfwpE6iSp4m5uU0ajauv6qgVrk9gYuf/mRhycidi2Qd07jI5Vxb/dqQkvB84CwGSD1jNljiB5TfgP0Xh1N55qHPspm/DIQDdpbPon+fX4MKZoQDA5UlXApIuHfWhhUuGJwDdAorlBQZScUIORBlIXvulhfDNQK489cgxm80zophHAcoikuASptmRNpxx/LVNPVbCdQV5NDbjbvFAaggEY524DG/p100ZpkQl+wY263wQZzDj2u3I9csppXLcd7l4HDW44VW/UN7SDgsU0vdEt44S76/+gYi2P7Q6H779AS7469BncetRYaQfBBt9rOkKOYNbPBImNE50B1vcnr0hVKmLtCLc8TbGBhEW3cT2JVKTVHi2prHEoe3toTkVxsjyfEesQlTPq/iRDzOpl0jpWey/dDyrXLgOEVqmsyezcteA+3A0BdvZZx9KOz9nWCKoK7W66T2EmsvhlwCewReLzI8U20I9KgyK2pQIWNO2/NdNBuB3hiXwCm7IDvjJefbeblcedxH5ZvN4n1Qw8wwcWm7yrD1X56YRKH6HYHeKBHVE0/HDao+v8fANqRgU0NoNKpAvnh9t6Or7dmgqpSp9xGzwjigK3lDdmMbeyZkyS50i30YzquZ/+Zqj7Qt4PFqnhlQL5Tulith6O6acThBJyq6SP7N/CMNuq57FGYxlhNVX5H4dcT0aVfB7vhShh8B4vzA1IQHYgI4aMzGXhjY9+W+uC+TPpMwzboP9qciS6RbXHDUoYGDYIrnqH906ZhpIGcOLbdorXqpFEhQMZE9HOo97BLvxmQtGctHU9vzKANnae8WjU9ifuIMSahuFhEJA+f5kvSO8zvV4WxocZG9p9+gE='))
AS code,
'Populate Test Data - Auth Test Accounts'                          AS name,
DEMO.BASE64DECODE('CiMgRm9yd2FyZCBNaWdyYXRpb24gMTE0NDogUG9wdWxhdGUgVGVzdCBEYXRhIC0gQXV0aCBUZXN0IEFjY291bnRzCgpUaGlzIG1pZ3JhdGlvbiBjcmVhdGVzIHRlc3QgdXNlciBhY2NvdW50cyBmb3IgYXV0aGVudGljYXRpb24gZW5kcG9pbnQgdGVzdGluZy4KSW5jbHVkZXMgdmFyaW91cyBhY2NvdW50IHN0YXRlczogZW5hYmxlZC9kaXNhYmxlZCwgYXV0aG9yaXplZC91bmF1dGhvcml6ZWQuCg==')
AS summary,
'{}'                                                                AS collection,
NULL            AS valid_after ,
NULL            AS valid_until ,
0               AS created_id ,
CURRENT TIMESTAMP          AS created_at ,
0               AS updated_id ,
CURRENT TIMESTAMP          AS updated_at
FROM next_query_id;
-- QUERY DELIMITER
INSERT INTO DEMO.queries (
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
    FROM DEMO.queries
)
SELECT
new_query_id                                                        AS query_id,
1144                                                        AS query_ref,
1                                                    AS query_status_a27,
1001                                           AS query_type_a28,
4                                                          AS query_dialect_a30,
0                                                         AS query_queue_a58,
5000                                                          AS query_timeout,
DEMO.BASE64DECODE('Ci0tIFJlbW92ZSB0ZXN0IGFjY291bnQgY29udGFjdHMgZmlyc3QgKGZvcmVpZ24ga2V5IGNvbnN0cmFpbnQpCkRFTEVURSBGUk9NIERFTU8uYWNjb3VudF9jb250YWN0cwpXSEVSRSBhY2NvdW50X2lkIElOICgKICAgIFNFTEVDVCBhY2NvdW50X2lkIEZST00gREVNTy5hY2NvdW50cwogICAgV0hFUkUgbmFtZSBJTiAoJ3Rlc3R1c2VyJywgJ2FkbWludXNlcicsICdkaXNhYmxlZHVzZXInLCAndW5hdXRob3JpemVkdXNlcicpCik7CgotLSBTVUJRVUVSWSBERUxJTUlURVIKCi0tIFJlbW92ZSB0ZXN0IGFjY291bnRzCkRFTEVURSBGUk9NIERFTU8uYWNjb3VudHMKV0hFUkUgbmFtZSBJTiAoJ3Rlc3R1c2VyJywgJ3Rlc3R1c2VyJywgJ2Rpc2FibGVkdXNlcicsICd1bmF1dGhvcml6ZWR1c2VyJyk7CgotLSBTVUJRVUVSWSBERUxJTUlURVIKClVQREFURSBERU1PLnF1ZXJpZXMKICBTRVQgcXVlcnlfdHlwZV9hMjggPSAxMDAwCldIRVJFIHF1ZXJ5X3JlZiA9IDExNDQKICBhbmQgcXVlcnlfdHlwZV9hMjggPSAxMDAzOwo=')
AS code,
'Remove Test Data - Auth Test Accounts'                             AS name,
DEMO.BASE64DECODE('CiMgUmV2ZXJzZSBNaWdyYXRpb24gMTE0NDogUmVtb3ZlIFRlc3QgRGF0YSAtIEF1dGggVGVzdCBBY2NvdW50cwoKVGhpcyBtaWdyYXRpb24gcmVtb3ZlcyB0aGUgdGVzdCB1c2VyIGFjY291bnRzIGNyZWF0ZWQgZm9yIGF1dGhlbnRpY2F0aW9uIHRlc3RpbmcuCg==')
AS summary,
'{}'                                                                AS collection,
NULL            AS valid_after ,
NULL            AS valid_until ,
0               AS created_id ,
CURRENT TIMESTAMP          AS created_at ,
0               AS updated_id ,
CURRENT TIMESTAMP          AS updated_at
FROM next_query_id;

