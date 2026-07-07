-- Migration: acuranzo_1213.lua
-- Lookup 065 - Mail Event Status for the Hydrogen Mail Relay Subsystem (Phase 4A)

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-07-07 - Initial creation for MAILRELAY_PLAN Phase 4A (mail event status lookup)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1213"
cfg.LOOKUP_ID = "065"
cfg.LOOKUP_NAME = "Mail Event Status"

-- ----------------------------------------------------------------------------
-- Forward: Populate Lookup 065 - Mail Event Status
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
            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (0, ${LOOKUP_ID}, 1, '${LOOKUP_NAME}', 0, 0, '', '', '{}', ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 0, 1, 'pending', 0, 0,
                    '',
                    [==[
                        # ${LOOKUP_ID} - ${LOOKUP_NAME} - Pending

                        The system event has been recorded but not yet
                        converted into a queued mail message.
                    ]==],
                    '{}',
                    ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 1, 1, 'queued', 1, 1,
                    '',
                    [==[
                        # ${LOOKUP_ID} - ${LOOKUP_NAME} - Queued

                        The event has been accepted and a corresponding mail
                        message has been placed in the outbound queue.
                    ]==],
                    '{}',
                    ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 2, 1, 'sent', 2, 2,
                    '',
                    [==[
                        # ${LOOKUP_ID} - ${LOOKUP_NAME} - Sent

                        The queued mail message derived from this event was
                        delivered successfully.
                    ]==],
                    '{}',
                    ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 3, 1, 'failed', 3, 3,
                    '',
                    [==[
                        # ${LOOKUP_ID} - ${LOOKUP_NAME} - Failed

                        The queued mail message derived from this event could
                        not be delivered and will not be retried.
                    ]==],
                    '{}',
                    ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 4, 1, 'suppressed', 4, 4,
                    '',
                    [==[
                        # ${LOOKUP_ID} - ${LOOKUP_NAME} - Suppressed

                        The event was dropped by a rule, rate limit, or
                        debounce policy and no mail was sent.
                    ]==],
                    '{}',
                    ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Populate Lookup ${LOOKUP_ID} - ${LOOKUP_NAME} in ${TABLE} table'   AS name,
        [=[
            # Forward Migration ${MIGRATION}: Populate Lookup ${LOOKUP_ID} - ${LOOKUP_NAME}

            This migration populates Lookup ${LOOKUP_ID} - ${LOOKUP_NAME} with
            the five event status values used by the Hydrogen Mail Relay
            Subsystem: pending (0), queued (1), sent (2), failed (3), and
            suppressed (4).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse: Remove Lookup 065 - Mail Event Status
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
            WHERE lookup_id = 0
            AND key_idx = ${LOOKUP_ID};

            ${SUBQUERY_DELIMITER}

            DELETE FROM ${SCHEMA}${TABLE}
            WHERE lookup_id = ${LOOKUP_ID}
            AND key_idx IN (0, 1, 2, 3, 4);

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Lookup ${LOOKUP_ID} from ${TABLE} Table'                    AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Lookup ${LOOKUP_ID} from ${TABLE} Table

            This is provided for completeness when testing the migration system.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries
end
