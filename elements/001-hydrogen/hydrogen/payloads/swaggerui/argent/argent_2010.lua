-- Migration: argent_2010.lua
-- Seeds lookup 2004, transaction kind

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-10-07 - Seed lookup 2004 (transfer through other)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "2010"
cfg.LOOKUP_ID = "2004"
cfg.LOOKUP_NAME = "Transaction Kind"
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
        SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
        FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id                                                        AS query_id,
        ${MIGRATION}                                                        AS query_ref,
        ${STATUS_ACTIVE}                                                    AS query_status_a27,
        ${TYPE_FORWARD_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            INSERT INTO ${SCHEMA}${TABLE}
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
                ${COMMON_FIELDS}
            )
            VALUES (
                0,
                ${LOOKUP_ID},
                1,
                '${LOOKUP_NAME}',
                0,
                0,
                'txn_kind',
                [==[
                    # ${LOOKUP_ID} - ${LOOKUP_NAME}

                    Transaction kind for Argent. Keys 1 through 11.
                    Column: transactions.kind_a2004.
                    Kinds 7 statement and 8 period_close may be zero amount.
                    The balance query does not special-case them.
                ]==],
                ${JSON_INGEST_START}
                [==[
                    {}
                ]==]
                ${JSON_INGEST_END},
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID},  1, 1, 'Transfer',       0,  1, 'transfer',       '', ${JIS}[==[{"icon":"<fa fa-right-left></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  2, 1, 'Purchase',       0,  2, 'purchase',       '', ${JIS}[==[{"icon":"<fa fa-cart-shopping></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  3, 1, 'Payment',        0,  3, 'payment',        '', ${JIS}[==[{"icon":"<fa fa-money-bill></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  4, 1, 'Fee',            0,  4, 'fee',            '', ${JIS}[==[{"icon":"<fa fa-receipt></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  5, 1, 'Interest',       0,  5, 'interest',       '', ${JIS}[==[{"icon":"<fa fa-percent></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  6, 1, 'Opening',        0,  6, 'opening',        '', ${JIS}[==[{"icon":"<fa fa-flag></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  7, 1, 'Statement',      0,  7, 'statement',      '', ${JIS}[==[{"icon":"<fa fa-file-lines></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  8, 1, 'Period close',   0,  8, 'period_close',   '', ${JIS}[==[{"icon":"<fa fa-lock></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  9, 1, 'Invoice record', 0,  9, 'invoice_record', '', ${JIS}[==[{"icon":"<fa fa-file-invoice></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 10, 1, 'Adjustment',     0, 10, 'adjustment',     '', ${JIS}[==[{"icon":"<fa fa-sliders></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 11, 1, 'Other',          0, 11, 'other',          '', ${JIS}[==[{"icon":"<fa fa-ellipsis></fa>"}]==]${JIE}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Populate Lookup ${LOOKUP_ID} in ${TABLE} table'                    AS name,
        [=[
            # Forward Migration ${MIGRATION}: Populate Lookup ${LOOKUP_ID} - ${LOOKUP_NAME}

            One family. The directory row is lookup_id 0, key_idx ${LOOKUP_ID}.
            Value keys are 1 through 11. No new table. No caller-facing QueryRef.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
        SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
        FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id                                                        AS query_id,
        ${MIGRATION}                                                        AS query_ref,
        ${STATUS_ACTIVE}                                                    AS query_status_a27,
        ${TYPE_REVERSE_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            DELETE FROM ${SCHEMA}${TABLE}
            WHERE lookup_id = 0
              AND key_idx = ${LOOKUP_ID};

            ${SUBQUERY_DELIMITER}

            DELETE FROM ${SCHEMA}${TABLE}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx IN (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11);

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Lookup ${LOOKUP_ID} from ${TABLE} Table'                    AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Lookup ${LOOKUP_ID} - ${LOOKUP_NAME}

            Deletes the directory row and keys 1 through 11. No other lookup_id.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
        SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
        FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id                                                        AS query_id,
        ${MIGRATION}                                                        AS query_ref,
        ${STATUS_ACTIVE}                                                    AS query_status_a27,
        ${TYPE_DIAGRAM_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        'JSON lookup definition in collection'                              AS code,
        'Diagram Lookup ${LOOKUP_ID}'                                       AS name,
        [=[
            # Diagram Migration ${MIGRATION}

            Lookup ${LOOKUP_ID} keys. This does not define a table.
        ]=]
                                                                            AS summary,
                                                                            -- DIAGRAM_START
        ${JSON_INGEST_START}
        [=[
            {
                "diagram": [
                    {
                        "object_type": "lookup",
                        "object_id": "lookup.${LOOKUP_ID}",
                        "object_ref": "${MIGRATION}",
                        "lookup_id": "${LOOKUP_ID}",
                        "name": "${LOOKUP_NAME}",
                        "keys": [
                            {"key_idx": 1, "code": "transfer", "value_txt": "Transfer"},
                            {"key_idx": 2, "code": "purchase", "value_txt": "Purchase"},
                            {"key_idx": 3, "code": "payment", "value_txt": "Payment"},
                            {"key_idx": 4, "code": "fee", "value_txt": "Fee"},
                            {"key_idx": 5, "code": "interest", "value_txt": "Interest"},
                            {"key_idx": 6, "code": "opening", "value_txt": "Opening"},
                            {"key_idx": 7, "code": "statement", "value_txt": "Statement"},
                            {"key_idx": 8, "code": "period_close", "value_txt": "Period close"},
                            {"key_idx": 9, "code": "invoice_record", "value_txt": "Invoice record"},
                            {"key_idx": 10, "code": "adjustment", "value_txt": "Adjustment"},
                            {"key_idx": 11, "code": "other", "value_txt": "Other"}
                        ]
                    }
                ]
            }
        ]=]
        ${JSON_INGEST_END}
                                                                            -- DIAGRAM_END
                                                                            AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
