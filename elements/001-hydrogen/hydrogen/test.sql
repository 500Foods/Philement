CREATE TABLE TEST.lookups
(
    lookup_id               INTEGER          NOT NULL,
    key_idx                 INTEGER          NOT NULL,
    status_a1               INTEGER          NOT NULL,
    value_txt               VARCHAR(250)                     ,
    value_int               INTEGER                  ,
    sort_seq                INTEGER          NOT NULL,
    code                    CLOB(1M)                 ,
    summary                 CLOB(1M)                 ,
    collection              CLOB(1M)                     ,
    valid_after             TIMESTAMP             ,
    valid_until             TIMESTAMP             ,
    created_id              INTEGER          NOT NULL,
    created_at              TIMESTAMP     NOT NULL,
    updated_id              INTEGER          NOT NULL,
    updated_at              TIMESTAMP     NOT NULL,
    
PRIMARY KEY(lookup_id, key_idx)
);
