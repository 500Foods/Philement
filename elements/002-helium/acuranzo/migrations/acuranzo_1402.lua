-- Migration: acuranzo_1402.lua
-- Lookup 060 key 0 Profile Sections: add page indices
--
-- Lithium Profile Manager expects each section tuple to be
-- ["Group", "icon", "Label", index]. acuranzo_1178 seeded only
-- three fields, so the client fell back to position+1 and collided
-- with manager IDs; Account (-1) never selected. Indices match
-- profile-manager page-registry (Photo = -12). Notifications is
-- dropped until it has its own index and handler.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-27 - Profile Sections tuples gain page indices

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1402"
cfg.LOOKUP_ID = "060"
cfg.KEY_IDX = "0"
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
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
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[
{
    "default": [
        [
            "General",
            "<fa fa-id-card></fa>",
            "Account",
            -1
        ],
        [
            "General",
            "<fa fa-id-badge></fa>",
            "Names",
            -2
        ],
        [
            "General",
            "<fa fa-address-book></fa>",
            "Addresses",
            -3
        ],
        [
            "General",
            "<fa fa-at></fa>",
            "E-Mail",
            -4
        ],
        [
            "General",
            "<fa fa-phone></fa>",
            "Phone",
            -5
        ],
        [
            "Security",
            "<fa fa-key></fa>",
            "Authentication",
            -6
        ],
        [
            "Security",
            "<fa fa-receipt></fa>",
            "Login History",
            -17
        ],
        [
            "Security",
            "<fa fa-user-key></fa>",
            "Tokens",
            -7
        ],
        [
            "Formatting",
            "<fa fa-globe></fa>",
            "Language",
            -8
        ],
        [
            "Formatting",
            "<fa fa-calendar></fa>",
            "Date Formats",
            -9
        ],
        [
            "Formatting",
            "<fa fa-00></fa>",
            "Number Formats",
            -10
        ],
        [
            "Application",
            "<fa fa-atom-simple></fa>",
            "Startup",
            -11
        ],
        [
            "Application",
            "<fa fa-camera></fa>",
            "Photo",
            -12
        ],
        [
            "Application",
            "<fa fa-bell-concierge></fa>",
            "Concierge",
            -13
        ],
        [
            "Application",
            "<fa fa-note-sticky></fa>",
            "Annotations",
            -14
        ],
        [
            "Application",
            "<fa fa-signs-post></fa>",
            "Tours",
            -15
        ],
        [
            "Application",
            "<fa fa-graduation-cap></fa>",
            "Training",
            -16
        ]
    ],
    "en-US": [
        [
            "General",
            "<fa fa-id-card></fa>",
            "Account",
            -1
        ],
        [
            "General",
            "<fa fa-id-badge></fa>",
            "Names",
            -2
        ],
        [
            "General",
            "<fa fa-address-book></fa>",
            "Addresses",
            -3
        ],
        [
            "General",
            "<fa fa-at></fa>",
            "E-Mail",
            -4
        ],
        [
            "General",
            "<fa fa-phone></fa>",
            "Phone",
            -5
        ],
        [
            "Security",
            "<fa fa-key></fa>",
            "Authentication",
            -6
        ],
        [
            "Security",
            "<fa fa-receipt></fa>",
            "Login History",
            -17
        ],
        [
            "Security",
            "<fa fa-user-key></fa>",
            "Tokens",
            -7
        ],
        [
            "Formatting",
            "<fa fa-globe></fa>",
            "Language",
            -8
        ],
        [
            "Formatting",
            "<fa fa-calendar></fa>",
            "Date Formats",
            -9
        ],
        [
            "Formatting",
            "<fa fa-00></fa>",
            "Number Formats",
            -10
        ],
        [
            "Application",
            "<fa fa-atom-simple></fa>",
            "Startup",
            -11
        ],
        [
            "Application",
            "<fa fa-camera></fa>",
            "Photo",
            -12
        ],
        [
            "Application",
            "<fa fa-bell-concierge></fa>",
            "Concierge",
            -13
        ],
        [
            "Application",
            "<fa fa-note-sticky></fa>",
            "Annotations",
            -14
        ],
        [
            "Application",
            "<fa fa-signs-post></fa>",
            "Tours",
            -15
        ],
        [
            "Application",
            "<fa fa-graduation-cap></fa>",
            "Training",
            -16
        ]
    ]
}
]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
            AND key_idx = ${KEY_IDX};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Add page indices to Lookup ${LOOKUP_ID} key ${KEY_IDX}'                                                    AS name,
        [=[
            # Forward Migration ${MIGRATION}: Profile Sections indices

            Rewrites Lookup ${LOOKUP_ID} key ${KEY_IDX} (Profile Sections).
            Each entry is ["Group", "icon", "Label", index] with negative
            indices for internal Profile Manager pages. Photo is -12.
            Fixes the Lithium navigator that could not select Account.

        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
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
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[
{
    "default": [
        [
            "General",
            "<fa fa-id-card></fa>",
            "Account"
        ],
        [
            "General",
            "<fa fa-id-badge></fa>",
            "Names"
        ],
        [
            "General",
            "<fa fa-address-book></fa>",
            "Addresses"
        ],
        [
            "General",
            "<fa fa-at></fa>",
            "E-Mail"
        ],
        [
            "General",
            "<fa fa-phone></fa>",
            "Phone"
        ],
        [
            "Security",
            "<fa fa-key></fa>",
            "Authentication"
        ],
        [
            "Security",
            "<fa fa-receipt></fa>",
            "Login History"
        ],
        [
            "Security",
            "<fa fa-user-key></fa>",
            "Tokens"
        ],
        [
            "Formatting",
            "<fa fa-globe></fa>",
            "Langauge"
        ],
        [
            "Formatting",
            "<fa fa-calendar></fa>",
            "Date/Time Formats"
        ],
        [
            "Formatting",
            "<fa fa-00></fa>",
            "Number Formats"
        ],
        [
            "Application",
            "<fa fa-flag-pennant></fa>",
            "Startup"
        ],
        [
            "Application",
            "<fa fa-bell></fa>",
            "Notifications"
        ],
        [
            "Application",
            "<fa fa-bell-concierge></fa>",
            "Concierge"
        ],
        [
            "Application",
            "<fa fa-note-sticky></fa>",
            "Annotations"
        ],
        [
            "Application",
            "<fa fa-note-sticky></fa>",
            "Tours"
        ],
        [
            "Application",
            "<fa fa-note-sticky></fa>",
            "Training"
        ]
    ],
    "en-US": [
        [
            "General",
            "<fa fa-id-card></fa>",
            "Account"
        ],
        [
            "General",
            "<fa fa-id-badge></fa>",
            "Names"
        ],
        [
            "General",
            "<fa fa-address-book></fa>",
            "Addresses"
        ],
        [
            "General",
            "<fa fa-at></fa>",
            "E-Mail"
        ],
        [
            "General",
            "<fa fa-phone></fa>",
            "Phone"
        ],
        [
            "Security",
            "<fa fa-key></fa>",
            "Authentication"
        ],
        [
            "Security",
            "<fa fa-receipt></fa>",
            "Login History"
        ],
        [
            "Security",
            "<fa fa-user-key></fa>",
            "Tokens"
        ],
        [
            "Formatting",
            "<fa fa-globe></fa>",
            "Langauge"
        ],
        [
            "Formatting",
            "<fa fa-calendar></fa>",
            "Date/Time Formats"
        ],
        [
            "Formatting",
            "<fa fa-00></fa>",
            "Number Formats"
        ],
        [
            "Application",
            "<fa fa-flag-pennant></fa>",
            "Startup"
        ],
        [
            "Application",
            "<fa fa-bell></fa>",
            "Notifications"
        ],
        [
            "Application",
            "<fa fa-bell-concierge></fa>",
            "Concierge"
        ],
        [
            "Application",
            "<fa fa-note-sticky></fa>",
            "Annotations"
        ],
        [
            "Application",
            "<fa fa-note-sticky></fa>",
            "Tours"
        ],
        [
            "Application",
            "<fa fa-note-sticky></fa>",
            "Training"
        ]
    ]
}
]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
            AND key_idx = ${KEY_IDX};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Restore Lookup ${LOOKUP_ID} key ${KEY_IDX} three-field tuples'                                                    AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Profile Sections

            Restores the acuranzo_1178.lua three-field Profile Sections
            collection (no page indices).

        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
