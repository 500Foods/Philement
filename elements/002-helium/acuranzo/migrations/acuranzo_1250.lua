-- Migration: acuranzo_1250.lua
-- QueryRef #120 - Insert Inbound Route

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-07-07 - Initial creation for MAILRELAY_PLAN Phase 4C.4

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1250"
cfg.QUERY_REF = "120"
cfg.QUERY_NAME = "Insert Inbound Route"
-- ----------------------------------------------------------------------------
-- Forward: Populate QueryRef #120 - Insert Inbound Route
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
                    ${INSERT_KEY_START} route_id ${INSERT_KEY_END}
                        INSERT INTO ${SCHEMA}mail_routes (
                            route_id,
                            status_a68,
                            source_network,
                            sender_domain,
                            sender_pattern,
                            recipient_domain,
                            recipient_pattern,
                            auth_required,
                            require_tls,
                            template_key,
                            rewrite_from,
                            rewrite_to,
                            add_recipients_json,
                            priority,
                            sort_seq,
                            ${COMMON_FIELDS}
                        )
                        WITH next_route_id AS (
                            SELECT COALESCE(MAX(route_id), 0) + 1 AS new_route_id
                            FROM ${SCHEMA}mail_routes
                        )
                        SELECT
                            new_route_id,
                            :STATUS_A68,
                            :SOURCE_NETWORK,
                            :SENDER_DOMAIN,
                            :SENDER_PATTERN,
                            :RECIPIENT_DOMAIN,
                            :RECIPIENT_PATTERN,
                            :AUTH_REQUIRED,
                            :REQUIRE_TLS,
                            :TEMPLATE_KEY,
                            :REWRITE_FROM,
                            :REWRITE_TO,
                            :ADD_RECIPIENTS_JSON,
                            :PRIORITY,
                            :SORT_SEQ,
                            ${COMMON_VALUES}
                        FROM next_route_id
                    ${INSERT_KEY_RETURN} route_id
                    ;
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    Inserts a new inbound SMTP route/rewrite rule into
                    `${SCHEMA}mail_routes`. The application generates the
                    `route_id` rather than relying on an engine-specific
                    autoincrement.

                    ## Parameters

                    - `STATUS_A68` (smallint, required): Route status from
                      Lookup 068 (0=inactive, 1=active).
                    - `SOURCE_NETWORK` (string, optional): Allowed client
                      network/CIDR.
                    - `SENDER_DOMAIN` (string, optional): Sender domain to match.
                    - `SENDER_PATTERN` (string, optional): Sender wildcard/regex
                      pattern.
                    - `RECIPIENT_DOMAIN` (string, optional): Recipient domain to
                      match.
                    - `RECIPIENT_PATTERN` (string, optional): Recipient
                      wildcard/regex pattern.
                    - `AUTH_REQUIRED` (smallint, required): 0 = no auth
                      requirement, 1 = authenticated clients only.
                    - `REQUIRE_TLS` (smallint, required): 0 = allow plaintext,
                      1 = require TLS.
                    - `TEMPLATE_KEY` (string, optional): Template to use when
                      rewriting the message.
                    - `REWRITE_FROM` (string, optional): Override From address.
                    - `REWRITE_TO` (string, optional): Override To address.
                    - `ADD_RECIPIENTS_JSON` (json, optional): Additional
                      recipients to add.
                    - `PRIORITY` (smallint, required): Route evaluation priority
                      (higher evaluated first).
                    - `SORT_SEQ` (integer, required): Tie-breaker for stable
                      ordering.

                    ## Returns

                    - `route_id`: The newly inserted route primary key.

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

            This migration creates the internal INSERT query for inbound routes
            (QueryRef #${QUERY_REF}).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse: Remove QueryRef #120 - Insert Inbound Route
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
