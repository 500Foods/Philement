-- Migration: acuranzo_1058.lua
-- Defaults for Lookup 033 - Login Controls

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2025-11-24 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1058"
cfg.LOOKUP_ID = "033"
cfg.LOOKUP_NAME = "Login Controls"
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
                0,                              -- lookup_id
                ${LOOKUP_ID},                   -- key_idx
                1,                              -- status_a1
                '${LOOKUP_NAME}',               -- value_txt
                0,                              -- value_int
                0,                              -- sort_seq
                '',                             -- code
                [==[
                    # ${LOOKUP_ID} - ${LOOKUP_NAME}

                    Login Controls  - flags for login features and settings.
                ]==],                           -- summary
                ${JSON_INGEST_START}
                [==[
                    {
                        "Default": "HTMLEditor",
                        "CSSEditor": false,
                        "HTMLEditor": true,
                        "JSONEditor": false,
                        "LookupEditor": false
                    }
                ]==]
                ${JSON_INGEST_END},             -- collection
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID},   0, 1, 'Login Log Account',                                 0,   0, '', '', '{}', ${COMMON_VALUES}),
                (${LOOKUP_ID},   1, 1, 'Retry Attempts',                                    0,   1, '', '', '{}', ${COMMON_VALUES}),
                (${LOOKUP_ID},   2, 1, 'Retry Window (minutes)',                            0,   2, '', '', '{}', ${COMMON_VALUES}),
                (${LOOKUP_ID},   3, 1, 'Block Duration (minutes)',                          0,   3, '', '', '{}', ${COMMON_VALUES}),
                (${LOOKUP_ID},   4, 1, 'Min Username Length',                               0,   4, '', '', '{}', ${COMMON_VALUES}),
                (${LOOKUP_ID},   5, 1, 'Min Password Length',                               0,   5, '', '', '{}', ${COMMON_VALUES}),
                (${LOOKUP_ID},   6, 1, 'Min Timezone Length',                               0,   6, '', '', '{}', ${COMMON_VALUES}),
                (${LOOKUP_ID},   7, 1, 'Min API Key Length',                                0,   7, '', '', '{}', ${COMMON_VALUES}),
                (${LOOKUP_ID},   8, 1, 'JWT Duration',                                      0,   8, '', '', '{}', ${COMMON_VALUES}),
                (${LOOKUP_ID}, 100, 1, 'Insufficent Username Length',                       0, 100, '', '', '{}', ${COMMON_VALUES}),
                (${LOOKUP_ID}, 101, 1, 'Insufficent Password Length',                       0, 101, '', '', '{}', ${COMMON_VALUES}),
                (${LOOKUP_ID}, 102, 1, 'Insufficent Timezone Length',                       0, 102, '', '', '{}', ${COMMON_VALUES}),
                (${LOOKUP_ID}, 103, 1, 'Insufficent API Key Length',                        0, 103, '', '', '{}', ${COMMON_VALUES}),
                (${LOOKUP_ID}, 104, 1, 'Invalid Timezone',                                  0, 104, '', '', '{}', ${COMMON_VALUES}),
                (${LOOKUP_ID}, 105, 1, 'API Key was not authenticated',                     0, 105, '', '', '{}', ${COMMON_VALUES}),
                (${LOOKUP_ID}, 106, 1, 'IP Address has been temporarily blocked',           0, 106, '', '', '{}', ${COMMON_VALUES}),
                (${LOOKUP_ID}, 107, 1, 'Too many login attempts - Please try again later',  0, 107, '', '', '{}', ${COMMON_VALUES}),
                (${LOOKUP_ID}, 108, 1, 'Invalid Username',                                  0, 108, '', '', '{}', ${COMMON_VALUES}),
                (${LOOKUP_ID}, 109, 1, 'Non-unique Username detected',                      0, 109, '', '', '{}', ${COMMON_VALUES}),
                (${LOOKUP_ID}, 110, 1, 'Username authentication disabled',                  0, 110, '', '', '{}', ${COMMON_VALUES}),
                (${LOOKUP_ID}, 111, 1, 'Account not authorized',                            0, 111, '', '', '{}', ${COMMON_VALUES}),
                (${LOOKUP_ID}, 112, 1, 'Invalid Password',                                  0, 112, '', '', '{}', ${COMMON_VALUES}),
                (${LOOKUP_ID}, 113, 1, 'E-mail address not found',                          0, 113, '', '', '{}', ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Populate Lookup ${LOOKUP_ID} in ${TABLE} table'                    AS name,
        [=[
            # Forward Migration ${MIGRATION}: Poulate Lookup ${LOOKUP_ID} - ${LOOKUP_NAME}

            This migration creates the lookup values for Lookup ${LOOKUP_ID} - ${LOOKUP_NAME}
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
            AND key_idx IN (0, 1, 2, 3, 4, 5, 6, 7, 8, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113);

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Lookup ${LOOKUP_ID} from ${TABLE} Table'                             AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Lookup ${LOOKUP_ID} - ${LOOKUP_NAME} from ${TABLE} Table

            This is provided for completeness when testing the migration system
            to ensure that forward and reverse migrations are complete.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
