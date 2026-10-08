-- Migration: acuranzo_1307.lua
-- Creates the user_enrollments table (Band M Phase 59)
--
-- Hydrogen entitlement SoT for My Courses. Canvas ids are actions only.
-- Design: FINISHLINE FL-58.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-08-13 - Create user_enrollments (Phase 59 / FL-58)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "user_enrollments"
cfg.MIGRATION = "1307"
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
                enrollment_id           ${INTEGER}          NOT NULL,
                account_id              ${INTEGER}          NOT NULL,
                course_id               ${INTEGER}          NOT NULL,
                canvas_enrollment_id    ${INTEGER}                  ,
                canvas_course_id        ${INTEGER}                  ,
                status                  ${VARCHAR_20}       NOT NULL DEFAULT 'active',
                enrolled_at             ${TIMESTAMP_TZ}     NOT NULL,
                expires_at              ${TIMESTAMP_TZ}             ,
                completed_at            ${TIMESTAMP_TZ}             ,
                progress_percent        ${INTEGER}          NOT NULL DEFAULT 0,
                progress_synced_at      ${TIMESTAMP_TZ}             ,
                archived_at             ${TIMESTAMP_TZ}             ,
                renew_policy            ${VARCHAR_20}       NOT NULL,
                source                  ${VARCHAR_20}       NOT NULL,
                order_id                ${INTEGER}                  ,
                ${COMMON_CREATE}
                ${PRIMARY}(enrollment_id)
            );

            ${SUBQUERY_DELIMITER}

            CREATE INDEX ${TABLE}_idx_account_status
                ON ${SCHEMA}${TABLE}(account_id, status);

            ${SUBQUERY_DELIMITER}

            CREATE INDEX ${TABLE}_idx_expires
                ON ${SCHEMA}${TABLE}(expires_at);

            ${SUBQUERY_DELIMITER}

            CREATE INDEX ${TABLE}_idx_account_archived
                ON ${SCHEMA}${TABLE}(account_id, archived_at);

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

            Band M Phase 59 — Reception enrollment entitlements (FL-58).

            Hydrogen **entitlement** is the source of truth for whether
            Reception grants access. Canvas enrollment / progress ids are
            action handles (Dashboard deep link, Phase 60 sync, re-enroll).
            Reception never reads Canvas REST.

            This migration creates the empty table only. It does **not**
            backfill intro provision (1290) or `Enroll.FreeCourse` (1300)
            Canvas seats. Those scripts still POST Canvas only; a follow-up
            seed/script change writes Helium rows (FL-59).

            ## Schema

            - **enrollment_id**: Surrogate PK (MAX+1). Phase 56 idempotency
              keys use this name.
            - **account_id**: Hydrogen `accounts.account_id` (JWT `user_id`).
              Logical FK — no SQL FK (same as user_preferences).
            - **course_id**: `courses.course_id` — not the Canvas id as the
              join key.
            - **canvas_enrollment_id** / **canvas_course_id**: nullable LMS
              handles. FreeCourse may not return the enroll id today.
            - **status**: stored `pending` | `active` | `completed` |
              `superseded`. Default `active`. Do **not** store `expired`
              — derive from `expires_at`.
            - **enrolled_at**: first grant of **this** row. Free renew does
              not reset. Domain timestamp (not a second `created_at`).
            - **expires_at**: NULL = lifetime (intro). Phase 56 skips NULL.
              Free/paid catalog = `enrolled_at + 90 days` (script constant).
            - **completed_at** / **progress_percent** / **progress_synced_at**:
              Phase 60 cache. Stale-OK. Default progress 0.
            - **archived_at**: Reception hide overlay. NULL = not archived.
              Does not unenroll Canvas.
            - **renew_policy**: `free_renew` | `paid_renew` | `no_renew`.
              Frozen from `courses.pricing_type` at grant.
            - **source**: `intro` | `free` | `paid` | `renew`. Audit only.
            - **order_id**: future Stripe order. NULL until Band J.

            `COMMON_CREATE` supplies audit `created_at` / `updated_at` —
            do **not** add a second domain `created_at` (FL-29c).

            ## Indexes

            - PRIMARY KEY on `enrollment_id`.
            - INDEX `(account_id, status)` — My Courses tabs.
            - INDEX `(expires_at)` — Phase 56 window scan.
            - INDEX `(account_id, archived_at)` — Archived tab.

            **One current row per `(account_id, course_id)`** where
            `status IN ('active','completed','pending')` is an **app
            invariant** (Enroll.* writers). Not a partial UNIQUE here:
            `CREATE UNIQUE INDEX ... WHERE` is not portable across
            MySQL/DB2. `superseded` rows may repeat the pair (paid
            history). Archive does not free the slot.

            ## Writers (not in this migration)

            - Intro (1290 follow-up): `source=intro`, `expires_at=NULL`,
              `renew_policy=free_renew`, `status=active`.
            - `Enroll.FreeCourse`: UPSERT current row `source=free`,
              `renew_policy=free_renew`, `expires_at=now()+90d`. Second
              call on a current non-expired row stays `already_enrolled`.
            - Free renew: UPDATE same row `expires_at=now()+90d`.
            - Paid fulfill (Band J): supersede current + INSERT new.

            Additive. Existing `accounts` / `courses` / Canvas seats are
            not touched.
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

            Provided for completeness when testing the migration system to
            ensure forward and reverse migrations are complete. Production
            rollback is documented in the FINISHLINE plan but not automated.
            Indexes drop with the table.
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

            ## Diagram Tables: ${SCHEMA}${TABLE}

            This is the JSON Diagram code for the ${TABLE} table.
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
                            {
                                "name": "enrollment_id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": true,
                                "unique": true
                            },
                            {
                                "name": "account_id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "course_id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "canvas_enrollment_id",
                                "datatype": "${INTEGER}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "canvas_course_id",
                                "datatype": "${INTEGER}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "status",
                                "datatype": "${VARCHAR_20}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "enrolled_at",
                                "datatype": "${TIMESTAMP_TZ}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "expires_at",
                                "datatype": "${TIMESTAMP_TZ}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "completed_at",
                                "datatype": "${TIMESTAMP_TZ}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "progress_percent",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "progress_synced_at",
                                "datatype": "${TIMESTAMP_TZ}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "archived_at",
                                "datatype": "${TIMESTAMP_TZ}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "renew_policy",
                                "datatype": "${VARCHAR_20}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "source",
                                "datatype": "${VARCHAR_20}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "order_id",
                                "datatype": "${INTEGER}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            ${COMMON_DIAGRAM}
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
