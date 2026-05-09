-- Migration: acuranzo_1190.lua
-- Relax accounts.password_hash to NULL for OIDC-provisioned accounts (OIDC RP Phase 16)

-- NOTE: This migration is one of the rare ones that must produce different
--       SQL per engine. PostgreSQL and DB2 use ALTER COLUMN ... DROP NOT NULL.
--       MySQL must MODIFY COLUMN with the full type. SQLite has no native
--       column-nullability ALTER and requires a table-rebuild dance.

-- db2
-- mysql
-- postgresql
-- sqlite

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-05-09 - Initial creation for OIDC Phase 16

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "accounts"
cfg.MIGRATION = "1190"
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- Forward Migration: Drop NOT NULL from accounts.password_hash
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
if engine == 'postgresql' then table.insert(queries,{sql=[[

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
            ALTER TABLE ${SCHEMA}${TABLE}
                ALTER COLUMN password_hash DROP NOT NULL;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Relax ${TABLE}.password_hash to NULL'                              AS name,
        [=[
            # Forward Migration ${MIGRATION}: Relax ${TABLE}.password_hash to NULL

            This migration relaxes the NOT NULL constraint on
            ${SCHEMA}${TABLE}.password_hash so that OIDC-provisioned accounts
            (Phase 20) can be created without a password hash.

            Existing rows are untouched.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]}) end
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
if engine == 'mysql' then table.insert(queries,{sql=[[

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
            ALTER TABLE ${SCHEMA}${TABLE}
                MODIFY COLUMN password_hash ${CHAR_128} NULL;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Relax ${TABLE}.password_hash to NULL'                              AS name,
        [=[
            # Forward Migration ${MIGRATION}: Relax ${TABLE}.password_hash to NULL

            This migration relaxes the NOT NULL constraint on
            ${SCHEMA}${TABLE}.password_hash so that OIDC-provisioned accounts
            (Phase 20) can be created without a password hash.

            Existing rows are untouched. MySQL requires MODIFY COLUMN with
            the full type signature; the type ${CHAR_128} matches the
            original definition from acuranzo_1005.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]}) end
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
if engine == 'db2' then table.insert(queries,{sql=[[

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
            ALTER TABLE ${SCHEMA}${TABLE}
                ALTER COLUMN password_hash DROP NOT NULL;

            ${SUBQUERY_DELIMITER}

            CALL SYSPROC.ADMIN_CMD('REORG TABLE ${SCHEMA}${TABLE}');

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Relax ${TABLE}.password_hash to NULL'                              AS name,
        [=[
            # Forward Migration ${MIGRATION}: Relax ${TABLE}.password_hash to NULL

            This migration relaxes the NOT NULL constraint on
            ${SCHEMA}${TABLE}.password_hash so that OIDC-provisioned accounts
            (Phase 20) can be created without a password hash.

            Existing rows are untouched. DB2 requires a REORG after an
            ALTER COLUMN to make the table accessible again.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]}) end
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
if engine == 'sqlite' then table.insert(queries,{sql=[[

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
            CREATE TABLE ${SCHEMA}${TABLE}_new
            (
                account_id              ${INTEGER}          NOT NULL,
                status_a16              ${INTEGER}          NOT NULL,
                iana_timezone_a17       ${INTEGER}          NOT NULL,
                name                    ${TEXT}             NOT NULL,
                first_name              ${TEXT}                     ,
                middle_name             ${TEXT}                     ,
                last_name               ${TEXT}             NOT NULL,
                password_hash           ${CHAR_128}                 ,
                summary                 ${TEXT_BIG}         NOT NULL,
                collection              ${JSON}                     ,
                ${COMMON_CREATE}
                ${PRIMARY}(account_id)
            );

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}_new
                SELECT * FROM ${SCHEMA}${TABLE};

            ${SUBQUERY_DELIMITER}

            DROP TABLE ${SCHEMA}${TABLE};

            ${SUBQUERY_DELIMITER}

            ALTER TABLE ${SCHEMA}${TABLE}_new RENAME TO ${TABLE};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Relax ${TABLE}.password_hash to NULL'                              AS name,
        [=[
            # Forward Migration ${MIGRATION}: Relax ${TABLE}.password_hash to NULL

            This migration relaxes the NOT NULL constraint on
            ${SCHEMA}${TABLE}.password_hash so that OIDC-provisioned accounts
            (Phase 20) can be created without a password hash.

            SQLite does not support ALTER COLUMN to change nullability, so
            the migration uses the standard SQLite table-rebuild pattern:

            1. Create ${TABLE}_new with the relaxed schema (password_hash
               nullable; every other column matches acuranzo_1005 exactly).
            2. Copy every row from ${TABLE} to ${TABLE}_new.
            3. Drop the old ${TABLE}.
            4. Rename ${TABLE}_new to ${TABLE}.

            ${TABLE} has no non-PK indexes (per acuranzo_1005), so no index
            recreation is required. If a future migration adds an index to
            ${TABLE}, this rebuild path must be updated to recreate it.

            Existing rows are preserved verbatim.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]}) end
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- Reverse Migration: Re-add NOT NULL to accounts.password_hash
-- WARNING: Will fail if any rows have NULL password_hash. Operators must
-- delete or assign passwords to OIDC-provisioned rows before reversing.
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
if engine == 'postgresql' then table.insert(queries,{sql=[[

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
            ALTER TABLE ${SCHEMA}${TABLE}
                ALTER COLUMN password_hash SET NOT NULL;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Re-tighten ${TABLE}.password_hash to NOT NULL'                     AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Re-tighten ${TABLE}.password_hash to NOT NULL

            Re-applies the NOT NULL constraint. Will fail if any
            ${TABLE} row has NULL in password_hash; operators must
            delete or assign passwords to such rows first.

            Provided for completeness when testing the migration system
            to ensure that forward and reverse migrations are complete.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]}) end
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
if engine == 'mysql' then table.insert(queries,{sql=[[

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
            ALTER TABLE ${SCHEMA}${TABLE}
                MODIFY COLUMN password_hash ${CHAR_128} NOT NULL;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Re-tighten ${TABLE}.password_hash to NOT NULL'                     AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Re-tighten ${TABLE}.password_hash to NOT NULL

            Re-applies the NOT NULL constraint. Will fail if any
            ${TABLE} row has NULL in password_hash; operators must
            delete or assign passwords to such rows first.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]}) end
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
if engine == 'db2' then table.insert(queries,{sql=[[

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
            ALTER TABLE ${SCHEMA}${TABLE}
                ALTER COLUMN password_hash SET NOT NULL;

            ${SUBQUERY_DELIMITER}

            CALL SYSPROC.ADMIN_CMD('REORG TABLE ${SCHEMA}${TABLE}');

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Re-tighten ${TABLE}.password_hash to NOT NULL'                     AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Re-tighten ${TABLE}.password_hash to NOT NULL

            Re-applies the NOT NULL constraint. Will fail if any
            ${TABLE} row has NULL in password_hash; operators must
            delete or assign passwords to such rows first.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]}) end
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
if engine == 'sqlite' then table.insert(queries,{sql=[[

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
            CREATE TABLE ${SCHEMA}${TABLE}_new
            (
                account_id              ${INTEGER}          NOT NULL,
                status_a16              ${INTEGER}          NOT NULL,
                iana_timezone_a17       ${INTEGER}          NOT NULL,
                name                    ${TEXT}             NOT NULL,
                first_name              ${TEXT}                     ,
                middle_name             ${TEXT}                     ,
                last_name               ${TEXT}             NOT NULL,
                password_hash           ${CHAR_128}         NOT NULL,
                summary                 ${TEXT_BIG}         NOT NULL,
                collection              ${JSON}                     ,
                ${COMMON_CREATE}
                ${PRIMARY}(account_id)
            );

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}_new
                SELECT * FROM ${SCHEMA}${TABLE};

            ${SUBQUERY_DELIMITER}

            DROP TABLE ${SCHEMA}${TABLE};

            ${SUBQUERY_DELIMITER}

            ALTER TABLE ${SCHEMA}${TABLE}_new RENAME TO ${TABLE};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Re-tighten ${TABLE}.password_hash to NOT NULL'                     AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Re-tighten ${TABLE}.password_hash to NOT NULL

            Reverses the SQLite forward path by rebuilding ${TABLE} once
            more, this time with password_hash NOT NULL. Will fail at the
            INSERT step if any row has NULL in password_hash; operators
            must delete or assign passwords to such rows first.

            Provided for completeness when testing the migration system
            to ensure that forward and reverse migrations are complete.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]}) end
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- Diagram Migration: re-declare password_hash as nullable so test_71's
-- aggregator picks up the relaxed constraint when rendering the schema
-- diagram.
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

            Updates the ${TABLE} diagram to reflect that password_hash is
            now nullable (relaxed in this migration's forward query).
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
                                "name": "password_hash",
                                "datatype": "${CHAR_128}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            }
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
