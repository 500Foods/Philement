-- Migration: acuranzo_1178.lua
-- Defaults for Lookup 060 - App UI Lists

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-04-09 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1178"
cfg.LOOKUP_ID = "060"
cfg.LOOKUP_NAME = "App UI Lists"
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

                    This lookup stores JSON lists for UI elements, like the list of profile
                    sections, that sort of thing. Where we might want it to be internationalized
                    or even controlled in some respects, like limiting per-user or something.

                ]==],                           -- summary
                ${JSON_INGEST_START}
                [==[
                    {
                        "Default": "JSONEditor",
                        "CSSEditor": false,
                        "HTMLEditor": false,
                        "JSONEditor": true,
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
                (${LOOKUP_ID},  0, 1, 'Profile Sections', 0, 0, '', '', ${JIS}[==[
                    {
                        "default": [
                            ["General", "<fa fa-id-card></fa>", "Account"],
                            ["General", "<fa fa-id-badge></fa>", "Names"],
                            ["General", "<fa fa-address-book></fa>", "Addresses"],
                            ["General", "<fa fa-at></fa>", "E-Mail"],
                            ["General", "<fa fa-phone></fa>", "Phone"],
                            ["Security", "<fa fa-key></fa>", "Authentication"]
                            ["Security", "<fa fa-receipt></fa>", "Login History"],
                            ["Security", "<fa fa-user-key></fa>", "Tokens"],
                            ["Formatting", "<fa fa-globe></fa>", "Langauge"],
                            ["Formatting", "<fa fa-calendar></fa>", "Date/Time Formats"],
                            ["Formatting", "<fa fa-00></fa>", "Number Formats"],
                            ["Application", "<fa fa-flag-pennant></fa>", "Startup"],
                            ["Application", "<fa fa-bell></fa>", "Notifications"]
                            ["Application", "<fa fa-bell-concierge></fa>", "Concierge"]
                            ["Application", "<fa fa-note-sticky></fa>", "Annotations"]
                            ["Application", "<fa fa-note-sticky></fa>", "Tours"]
                            ["Application", "<fa fa-note-sticky></fa>", "Training"]
                        ],
                        "en-US": [
                            ["General", "<fa fa-id-card></fa>", "Account"],
                            ["General", "<fa fa-id-badge></fa>", "Names"],
                            ["General", "<fa fa-address-book></fa>", "Addresses"],
                            ["General", "<fa fa-at></fa>", "E-Mail"],
                            ["General", "<fa fa-phone></fa>", "Phone"],
                            ["Security", "<fa fa-key></fa>", "Authentication"]
                            ["Security", "<fa fa-receipt></fa>", "Login History"],
                            ["Security", "<fa fa-user-key></fa>", "Tokens"],
                            ["Formatting", "<fa fa-globe></fa>", "Langauge"],
                            ["Formatting", "<fa fa-calendar></fa>", "Date/Time Formats"],
                            ["Formatting", "<fa fa-00></fa>", "Number Formats"],
                            ["Application", "<fa fa-flag-pennant></fa>", "Startup"],
                            ["Application", "<fa fa-bell></fa>", "Notifications"]
                            ["Application", "<fa fa-bell-concierge></fa>", "Concierge"]
                            ["Application", "<fa fa-note-sticky></fa>", "Annotations"]
                            ["Application", "<fa fa-note-sticky></fa>", "Tours"]
                            ["Application", "<fa fa-note-sticky></fa>", "Training"]
                        ]
                    }
                ]==]${JIE}, ${COMMON_VALUES})

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

            This migration creates the lookup entry for Lookup ${LOOKUP_ID} - ${LOOKUP_NAME}.
            This lookup stores JSON lists of elements used by the application.
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
            AND key_idx IN (0);

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Lookup ${LOOKUP_ID} from ${TABLE} Table'                    AS name,
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
