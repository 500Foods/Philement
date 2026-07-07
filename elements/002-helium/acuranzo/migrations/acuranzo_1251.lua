-- Migration: acuranzo_1251.lua
-- QueryRef #121 - Update Inbound Route

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-07-07 - Initial creation for MAILRELAY_PLAN Phase 4C.4

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1251"
cfg.QUERY_REF = "121"
cfg.QUERY_NAME = "Update Inbound Route"
-- ----------------------------------------------------------------------------
-- Forward: Populate QueryRef #121 - Update Inbound Route
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
                ${TYPE_INTERNAL_SQL}                                                AS query_type_a28,
                ${DIALECT}                                                          AS query_dialect_a30,
                ${QTC_SLOW}                                                         AS query_queue_a58,
                ${TIMEOUT}                                                          AS query_timeout,
                [==[
                    UPDATE ${SCHEMA}mail_routes
                    SET
                        status_a68 = :STATUS_A68,
                        source_network = :SOURCE_NETWORK,
                        sender_domain = :SENDER_DOMAIN,
                        sender_pattern = :SENDER_PATTERN,
                        recipient_domain = :RECIPIENT_DOMAIN,
                        recipient_pattern = :RECIPIENT_PATTERN,
                        auth_required = :AUTH_REQUIRED,
                        require_tls = :REQUIRE_TLS,
                        template_key = :TEMPLATE_KEY,
                        rewrite_from = :REWRITE_FROM,
                        rewrite_to = :REWRITE_TO,
                        add_recipients_json = :ADD_RECIPIENTS_JSON,
                        priority = :PRIORITY,
                        sort_seq = :SORT_SEQ,
                        updated_at = ${NOW}
                    WHERE
                        route_id = :ROUTE_ID
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    Updates an existing inbound SMTP route. The `route_id`
                    identifies the row to update; all other fields are
                    replaced.

                    ## Parameters

                    - `ROUTE_ID` (integer, required): Route row to update.
                    - `STATUS_A68` (smallint, required): Route status from
                      Lookup 068.
                    - `SOURCE_NETWORK` (string, optional): Allowed client
                      network/CIDR.
                    - `SENDER_DOMAIN` (string, optional): Sender domain to match.
                    - `SENDER_PATTERN` (string, optional): Sender wildcard/regex
                      pattern.
                    - `RECIPIENT_DOMAIN` (string, optional): Recipient domain to
                      match.
                    - `RECIPIENT_PATTERN` (string, optional): Recipient
                      wildcard/regex pattern.
                    - `AUTH_REQUIRED` (smallint, required): Auth requirement flag.
                    - `REQUIRE_TLS` (smallint, required): TLS requirement flag.
                    - `TEMPLATE_KEY` (string, optional): Template used when
                      rewriting.
                    - `REWRITE_FROM` (string, optional): Override From address.
                    - `REWRITE_TO` (string, optional): Override To address.
                    - `ADD_RECIPIENTS_JSON` (json, optional): Additional
                      recipients.
                    - `PRIORITY` (smallint, required): Route priority.
                    - `SORT_SEQ` (integer, required): Tie-breaker sort sequence.

                    ## Returns

                    - No result rows.

                    ## Tables

                    - `${SCHEMA}mail_routes` (migration 1222).

                    ## Security Notes

                    - `query_type_a28` is `${TYPE_INTERNAL_SQL}` (0) so this query
                      is not reachable via the REST API.
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
        'Populate QueryRef #${QUERY_REF} - ${QUERY_NAME}'                   AS name,
        [=[
            # Forward Migration ${MIGRATION}: Populate QueryRef #${QUERY_REF} - ${QUERY_NAME}

            This migration creates the internal UPDATE query for inbound routes
            (QueryRef #${QUERY_REF}).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse: Remove QueryRef #121 - Update Inbound Route
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
        'Remove QueryRef #${QUERY_REF} - ${QUERY_NAME}'                     AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove QueryRef #${QUERY_REF} - ${QUERY_NAME}

            This is provided for completeness when testing the migration system.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
