
INSERT INTO TEST.lookups
(
    lookup_id,
    key_idx,
    status_a1,
    value_txt,
    value_int,
    sort_seq,
    code,
    summary,
    collection,
                            valid_after,
            valid_until,
            created_id,
            created_at,
            updated_id,
            updated_at
        
)
VALUES (
    0,                              -- lookup_id
    001,                   -- key_idx
    1,                              -- status_a1
    'Lookup Status',               -- value_txt
    0,                              -- value_int
    0,                              -- sort_seq
    '',                             -- code
    TEST.BASE64DECODE('CiMgIyAwMDAgLSBMb29rdXAgU3RhdHVzCgpVc2VkIGFzIGdlbmVyYWwgc3RhdHVzIGZsYWcgZm9yIExvb2t1cHMgdGFibGUuCg=='),                           -- summary
    TEST.json_ingest(
    TEST.BASE64DECODE('CnsKICAgICJEZWZhdWx0IjogIkhUTUxFZGl0b3IiLAogICAgIkNTU0VkaXRvciI6IGZhbHNlLAogICAgIkhUTUxFZGl0b3IiOiB0cnVlLAogICAgIkpTT05FZGl0b3IiOiBmYWxzZSwKICAgICJMb29rdXBFZGl0b3IiOiBmYWxzZQp9Cg==')
    ),             -- collection
                            NULL,
            NULL,
            0,
            CURRENT TIMESTAMP,
            0,
            CURRENT TIMESTAMP
        
);

-- SUBQUERY DELIMITER

INSERT INTO TEST.lookups
    (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection,                         valid_after,
            valid_until,
            created_id,
            created_at,
            updated_id,
            updated_at
        )
VALUES
    (001, 0, 1, 'Inactive',    0, 0, '', '', TEST.json_ingest(TEST.BASE64DECODE('eyJJY29uIjoiPGkgY2xhc3M9J2ZhIGZhLXhtYXJrIGZhLWZ3IGZhLXN3YXAtb3BhY2l0eScgc3R5bGU9J2NvbG9yOiAjRkYwMDAwOyBmaWx0ZXI6IHZhcigtLUFDWi1zaGFkb3ctNCk7Jz48L2k+In0=')),                         NULL,
            NULL,
            0,
            CURRENT TIMESTAMP,
            0,
            CURRENT TIMESTAMP
        ),
    (001, 1, 1, 'Active',      0, 1, '', '', TEST.json_ingest(TEST.BASE64DECODE('eyJJY29uIjoiPGkgY2xhc3M9J2ZhIGZhLWNoZWNrIGZhLWZ3IGZhLXN3YXAtb3BhY2l0eScgc3R5bGU9J2NvbG9yOiAjMDBGRjAwOyBmaWx0ZXI6IHZhcigtLUFDWi1zaGFkb3ctNCk7Jz48L2k+In0=')),                         NULL,
            NULL,
            0,
            CURRENT TIMESTAMP,
            0,
            CURRENT TIMESTAMP
        ),
    (001, 2, 1, 'Deprecated',  0, 2, '', '', TEST.json_ingest(TEST.BASE64DECODE('eyJJY29uIjoiPGkgY2xhc3M9J2ZhIGZhLW1pbnVzIGZhLWZ3IGZhLXN3YXAtb3BhY2l0eScgc3R5bGU9J2NvbG9yOiAjRkZGRjAwOyBmaWx0ZXI6IHZhcigtLUFDWi1zaGFkb3ctNCk7Jz48L2k+In0=')),                         NULL,
            NULL,
            0,
            CURRENT TIMESTAMP,
            0,
            CURRENT TIMESTAMP
        );

-- SUBQUERY DELIMITER

UPDATE TEST.queries
  SET query_type_a28 = 1003
WHERE query_ref = 1026
  and query_type_a28 = 1000;

