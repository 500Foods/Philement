-- Migration: argent_2001.lua
-- Creates the ledgers table

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-10-05 - Create ledgers on the shared Acuranzo database

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "ledgers"
cfg.MIGRATION = "2001"
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
                ledger_id                   ${INTEGER}          NOT NULL,
                organization_id             ${INTEGER}          NOT NULL,
                parent_id                   ${INTEGER}                  ,
                status_a202                 ${INTEGER}          NOT NULL,
                ledger_type_a201            ${INTEGER}          NOT NULL,
                is_posting                  ${INTEGER_SMALL}    NOT NULL,
                name                        ${TEXT}             NOT NULL,
                currency                    ${VARCHAR_20}       NOT NULL,
                opening_on                  ${DATE}             NOT NULL,
                opening_balance_cents       ${INTEGER_BIG}      NOT NULL,
                opening_txn_id              ${INTEGER}                  ,
                latest_reconciliation_id    ${INTEGER}                  ,
                latest_reconciled_on        ${DATE}                     ,
                calendar_url                ${TEXT}                     ,
                calendar_id                 ${VARCHAR_100}              ,
                mask                        ${VARCHAR_50}               ,
                external_ref                ${VARCHAR_100}              ,
                summary                     ${TEXT_BIG}                 ,
                collection                  ${JSON}                     ,
                ${COMMON_CREATE}
                ${PRIMARY}(ledger_id)
            );

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

            Argent is an optional pack on the Acuranzo database. Apply
            this after `argent_2000` (organizations), on the same schema.
            It inserts into the `queries` table Acuranzo already created.
            It does not bootstrap `queries`, `lookups`, or `scripts`.

            ## Schema

            - **ledger_id**: Surrogate primary key. Assigned by Lua
              (`COALESCE(MAX(ledger_id),0)+1`). No `${SERIAL}`.
            - **organization_id**: Owning organization. Same schema as
              `organizations.organization_id`. No SQL foreign key.
            - **parent_id**: Optional parent ledger in this table. Null is
              a root. Parents are roll-up views (`is_posting` = 0).
              Posting ledgers (`is_posting` = 1) are the only ones that
              receive lines. Enforced in Lua.
            - **status_a202**: Ledger status (open / closed / archive).
              Lookup **202**. The seed is a later migration. No SQL
              foreign key.
            - **ledger_type_a201**: asset, liability, equity, income, or
              expense. Lookup **201**. The seed is a later migration.
              Line sign follows this type in Lua, not a debit/credit pair
              of columns.
            - **is_posting**: 1 accepts lines. 0 is a parent roll-up only.
            - **name**: Display name.
            - **currency**: Fixed for the life of the ledger. Lowercase
              ISO 4217 (`cad`). Lua refuses a change. Not the
              organization default.
            - **opening_on** / **opening_balance_cents**: Per-ledger
              opening. The opening transaction itself is a later
              migration. `opening_txn_id` stays null until that exists.
            - **latest_reconciliation_id** / **latest_reconciled_on**:
              Denormalized recon pointer. The reconciliations table is
              later. No SQL foreign key.
            - **calendar_url** / **calendar_id**: CalDAV assignment.
              Credentials stay in the environment, never in this row.
            - **mask**: Last 4 only. Never a full card number.
            - **external_ref**: Future Plaid or QBO id.
            - **summary** / **collection**: Notes and extra facts
              (folder path, quirks). Collection is JSON.
            - **created_id** / **updated_id**: Acuranzo
              `accounts.account_id` in this same schema.

            ## Indexes

            - PRIMARY KEY on `ledger_id`.
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

            Drops `ledgers` only. Does not touch `organizations` or
            Acuranzo tables. Later Argent tables that reference
            `ledger_id` must be reversed first.
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

            JSON diagram for the ledgers table.
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
                                "name": "ledger_id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": true,
                                "unique": true
                            },
                            {
                                "name": "organization_id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "parent_id",
                                "datatype": "${INTEGER}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "status_a202",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "lookup": true
                            },
                            {
                                "name": "ledger_type_a201",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "lookup": true
                            },
                            {
                                "name": "is_posting",
                                "datatype": "${INTEGER_SMALL}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "name",
                                "datatype": "${TEXT}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "currency",
                                "datatype": "${VARCHAR_20}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "opening_on",
                                "datatype": "${DATE}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "opening_balance_cents",
                                "datatype": "${INTEGER_BIG}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "opening_txn_id",
                                "datatype": "${INTEGER}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "latest_reconciliation_id",
                                "datatype": "${INTEGER}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "latest_reconciled_on",
                                "datatype": "${DATE}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "calendar_url",
                                "datatype": "${TEXT}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "calendar_id",
                                "datatype": "${VARCHAR_100}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "mask",
                                "datatype": "${VARCHAR_50}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "external_ref",
                                "datatype": "${VARCHAR_100}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "summary",
                                "datatype": "${TEXT_BIG}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "collection",
                                "datatype": "${JSON}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false,
                                "standard": false
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
