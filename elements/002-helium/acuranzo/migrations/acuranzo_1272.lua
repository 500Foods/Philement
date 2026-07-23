-- Migration: acuranzo_1272.lua
-- Creates oauth_refresh_tokens (OIDC IdP Phase 6)

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-07-23 - Initial creation for Hydrogen OIDC Identity Provider

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "oauth_refresh_tokens"
cfg.MIGRATION = "1272"
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
            CREATE TABLE ${SCHEMA}${TABLE}
            (
                token_hash              ${VARCHAR_128}      NOT NULL,
                client_id               ${VARCHAR_128}      NOT NULL,
                account_id              ${INTEGER}          NOT NULL,
                scope                   ${VARCHAR_500}      NOT NULL,
                family_id               ${VARCHAR_128}      NOT NULL,
                expires_at              ${TIMESTAMP_TZ}     NOT NULL,
                revoked_at              ${TIMESTAMP_TZ}             ,
                replaced_by_hash        ${VARCHAR_128}              ,
                ${COMMON_CREATE}
                ${PRIMARY}(token_hash)
            );

            ${SUBQUERY_DELIMITER}

            CREATE INDEX ${TABLE}_idx_client
                ON ${SCHEMA}${TABLE}(client_id);

            ${SUBQUERY_DELIMITER}

            CREATE INDEX ${TABLE}_idx_account
                ON ${SCHEMA}${TABLE}(account_id);

            ${SUBQUERY_DELIMITER}

            CREATE INDEX ${TABLE}_idx_family
                ON ${SCHEMA}${TABLE}(family_id);

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Create ${TABLE} Table'                                             AS name,
        [=[
            # Forward Migration ${MIGRATION}: Create ${TABLE} Table

            Opaque OAuth refresh tokens for Hydrogen IdP. Hash-only storage.
            `family_id` supports rotation + reuse detection (Phase 11).
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
            ${DROP_CHECK};

            ${SUBQUERY_DELIMITER}

            DROP TABLE ${SCHEMA}${TABLE};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Drop ${TABLE} Table'                                               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Drop ${TABLE} Table
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
        'JSON Table Definition in collection'                               AS code,
        'Diagram Tables: ${SCHEMA}${TABLE}'                                 AS name,
        [=[
            # Diagram Migration ${MIGRATION}
        ]=]
                                                                            AS summary,
                                                                            -- DIAGRAM_START
        ${JSON_INGEST_START}
        [=[
            {
                "diagram": [
                    {
                        "object_type": "table",
                        "object_id": "table.${TABLE}",
                        "object_ref": "${MIGRATION}",
                        "table": [
                            {"name": "token_hash", "datatype": "${VARCHAR_128}", "nullable": false, "primary_key": true, "unique": true},
                            {"name": "client_id", "datatype": "${VARCHAR_128}", "nullable": false, "primary_key": false, "unique": false},
                            {"name": "account_id", "datatype": "${INTEGER}", "nullable": false, "primary_key": false, "unique": false},
                            {"name": "scope", "datatype": "${VARCHAR_500}", "nullable": false, "primary_key": false, "unique": false},
                            {"name": "family_id", "datatype": "${VARCHAR_128}", "nullable": false, "primary_key": false, "unique": false},
                            {"name": "expires_at", "datatype": "${TIMESTAMP_TZ}", "nullable": false, "primary_key": false, "unique": false},
                            {"name": "revoked_at", "datatype": "${TIMESTAMP_TZ}", "nullable": true, "primary_key": false, "unique": false},
                            {"name": "replaced_by_hash", "datatype": "${VARCHAR_128}", "nullable": true, "primary_key": false, "unique": false}
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
