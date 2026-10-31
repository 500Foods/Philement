-- Migration: acuranzo_1419.lua
-- QueryRef #162 - AEM Insert Counter

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-10-30 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1419"
cfg.QUERY_REF = "162"
cfg.QUERY_NAME = "AEM Insert Counter"

-- ----------------------------------------------------------------------------
-- Forward: Populate QueryRef #162 - AEM Insert Counter
-- ----------------------------------------------------------------------------
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
            INSERT INTO ${SCHEMA}${QUERIES} (
                ${QUERIES_INSERT}
            )
            WITH next_query_id AS (
                SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
                FROM ${SCHEMA}${QUERIES}
            )
            SELECT
                new_query_id                                                        AS query_id,
                ${QUERY_REF}                                                        AS query_ref,
                ${STATUS_ACTIVE}                                                    AS query_status_a27,
                ${TYPE_SQL}                                                         AS query_type_a28,
                ${DIALECT}                                                          AS query_dialect_a30,
                ${QTC_FAST}                                                         AS query_queue_a58,
                ${TIMEOUT}                                                          AS query_timeout,
                [==[
                    -- PostgreSQL / Yugabyte. One statement. Each parameter name is used once.
                    -- observed_at is stored as sent. This statement does not floor it.
                    -- dims_hash is stored as sent. This statement does not recompute it.
                    -- The same value_num and value_text leave the row, including updated_at, unchanged.
                    -- RETURNING is one row on insert or update, and no row on a no-op.
                    WITH src AS (
                        SELECT
                            CAST(:PROPERTY AS ${INTEGER_SMALL}) AS property_a69,
                            CAST(:CHANNEL AS ${INTEGER_SMALL}) AS channel_a70,
                            CAST(:METRIC AS ${INTEGER_SMALL}) AS metric_a72,
                            1 AS kind_a71,
                            CAST(:SOURCE AS ${INTEGER_SMALL}) AS source_a73,
                            CAST(:OBSERVED_AT AS ${TIMESTAMP_TZ}) AS observed_at,
                            CAST(NULLIF(CAST(:VALUE_NUM AS ${VARCHAR_100}), '') AS ${FLOAT_BIG}) AS value_num,
                            NULLIF(CAST(:VALUE_TEXT AS ${TEXT}), '') AS value_text,
                            CAST(:DIMS AS ${JSON}) AS dims,
                            CAST(:DIMS_HASH AS ${VARCHAR_64}) AS dims_hash
                    )
                    INSERT INTO ${SCHEMA}aem_observations AS obs (
                        property_a69,
                        channel_a70,
                        metric_a72,
                        kind_a71,
                        source_a73,
                        observed_at,
                        value_num,
                        value_text,
                        dims,
                        dims_hash,
                        ${COMMON_FIELDS}
                    )
                    SELECT
                        src.property_a69,
                        src.channel_a70,
                        src.metric_a72,
                        src.kind_a71,
                        src.source_a73,
                        src.observed_at,
                        src.value_num,
                        src.value_text,
                        src.dims,
                        src.dims_hash,
                        ${COMMON_VALUES}
                    FROM src
                    ON CONFLICT (property_a69, channel_a70, metric_a72, observed_at, dims_hash)
                    DO UPDATE SET
                        kind_a71 = EXCLUDED.kind_a71,
                        source_a73 = EXCLUDED.source_a73,
                        value_num = EXCLUDED.value_num,
                        value_text = EXCLUDED.value_text,
                        dims = EXCLUDED.dims,
                        updated_id = 0,
                        updated_at = ${NOW}
                    WHERE obs.value_num IS DISTINCT FROM EXCLUDED.value_num
                       OR obs.value_text IS DISTINCT FROM EXCLUDED.value_text
                    RETURNING
                        obs.property_a69,
                        obs.channel_a70,
                        obs.metric_a72,
                        obs.observed_at,
                        obs.value_num
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    Writes kind 1. The payload is the full bin total, not a delta.
                    No existing row inserts. The same value leaves the row
                    alone. A different value updates value_num, value_text,
                    source, dims, and updated_at.

                    ## Parameters

                    Each name is bound once, as text.

                    - PROPERTY (lookup 069)
                    - CHANNEL (lookup 070)
                    - METRIC (lookup 072)
                    - SOURCE (lookup 073)
                    - OBSERVED_AT (timestamptz text, bin start, such as 2026-10-30T19:00:00Z)
                    - VALUE_NUM (decimal string, or '' for NULL; a sampled zero is 0)
                    - VALUE_TEXT (text, or '' for NULL)
                    - DIMS (canonical JSON; an empty object is {})
                    - DIMS_HASH (SHA-256 hex of DIMS)

                    Kind is not a parameter.

                    ## Call

                    POST /api/conduit/auth_query with a Hydrogen JWT.
                    Public /api/conduit/query only runs SELECT.
                    This statement has no user, role, or property predicate.
                    created_id and updated_id are 0.

                    ## Result

                    One row when a row is inserted or updated.
                    No row when the value is unchanged.

                    ## Table

                    - aem_observations
                ]==]
                                                                                     AS summary,
                '{}'                                                                AS collection,
                ${COMMON_INSERT}
            FROM next_query_id;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Populate QueryRef #${QUERY_REF} - ${QUERY_NAME}'                                                            AS name,
        [=[
            # Forward Migration ${MIGRATION}: Populate QueryRef #${QUERY_REF} - ${QUERY_NAME}

            Inserts QueryRef #${QUERY_REF} as system SQL (type 1).
            Execute it on Yugabyte. Apply only stores the text.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- ----------------------------------------------------------------------------
-- Reverse: Remove QueryRef #162 - AEM Insert Counter
-- ----------------------------------------------------------------------------
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
            WHERE query_ref = ${QUERY_REF};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove QueryRef #${QUERY_REF} - ${QUERY_NAME}'                                                            AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove QueryRef #${QUERY_REF} - ${QUERY_NAME}

            Deletes the runtime QueryRef.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries
end
