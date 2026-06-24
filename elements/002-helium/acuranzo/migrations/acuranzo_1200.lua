-- Migration: acuranzo_1200.lua
-- QueryRef #086 - Insert Contact Submission (Cap-protected, slow queue)

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-06-24 - Initial creation for Cap Phase 2b

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1200"
cfg.QUERY_REF = "086"
cfg.QUERY_NAME = "Insert Contact Submission"
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
                ${TYPE_PROTECTED}                                                   AS query_type_a28,
                ${DIALECT}                                                          AS query_dialect_a30,
                ${QTC_SLOW}                                                         AS query_queue_a58,
                ${TIMEOUT}                                                          AS query_timeout,
                [==[
                    ${INSERT_KEY_START} submission_id ${INSERT_KEY_END}
                        INSERT INTO ${SCHEMA}contact_submissions (
                            submission_id,
                            name,
                            email,
                            subject,
                            message,
                            chacha_token,
                            chacha_site,
                            ip_address,
                            review_status,
                            valid_after,
                            valid_until,
                            created_id,
                            created_at,
                            updated_id,
                            updated_at
                        )
                        WITH next_submission_id AS (
                            SELECT COALESCE(MAX(submission_id), 0) + 1 AS new_submission_id
                            FROM ${SCHEMA}contact_submissions
                        )
                        SELECT
                            new_submission_id,
                            :NAME,
                            :EMAIL,
                            :SUBJECT,
                            :MESSAGE,
                            :CHACHA_TOKEN,
                            :CHACHA_SITE,
                            :IP_ADDRESS,
                            0,
                            NULL,
                            NULL,
                            0,
                            ${NOW},
                            0,
                            ${NOW}
                        FROM next_submission_id
                    ${INSERT_KEY_RETURN} submission_id
                    ;
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    Inserts a single row into `${SCHEMA}contact_submissions`
                    for the "Contact Us" public form. Only callable via the
                    Cap-protected `/api/conduit/cap_query` endpoint; any other
                    invocation is rejected before this query ever runs.

                    ## Parameters

                    - `NAME` (string, required): Submitter name.
                    - `EMAIL` (string, required): Submitter e-mail.
                    - `SUBJECT` (string, required): Short subject line.
                    - `MESSAGE` (string, required): HTML body (SunEditor).
                    - `CHACHA_TOKEN` (string, required): Cap siteverify token.
                    - `CHACHA_SITE` (string, required): Cap public site id.
                    - `IP_ADDRESS` (string, optional): Server-side client IP.

                    ## Returns

                    - `submission_id`: The newly inserted submission PK.

                    ## Tables

                    - `${SCHEMA}contact_submissions` (migration 1199).

                    ## Notes

                    - `query_type_a28` is `${TYPE_PROTECTED}` (11) so this row
                      is only returned by `lookup_database_and_protected_query`.
                    - `query_queue_a58` is `${QTC_SLOW}` (0). The `cap_query`
                      handler additionally forces the slow queue regardless.
                    - `review_status` defaults to 0 (pending review).
                    - `${INSERT_KEY_START}` / `${INSERT_KEY_RETURN}` are
                      engine-specific wrappers that produce a result row
                      from the INSERT (e.g. `RETURNING` on PostgreSQL).
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

            This migration creates the protected INSERT query for the
            "Contact Us" form (QueryRef #086).
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
