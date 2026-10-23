-- Migration: courses_4084.lua
-- QueryRef #4506 - Staff: Save Course Prices
--
-- 4506 is not a file number. One transaction writes cad, usd, eur,
-- and gbp for one course. stripe_price_id is never changed here.
-- Stripe.EnsureProduct action=rotate mints a new Price later.
-- A cleared amount sets is_active to 0 and leaves the cents.
-- A missing currency with a blank amount stays absent.
-- Each bind name is used at least once. The client sends every
-- integer, including the zeros.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-23 - Staff upsert of course_prices for four currencies

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "4084"
cfg.QUERY_REF = "4506"
cfg.QUERY_NAME = "Staff: Save Course Prices"
if engine == "mysql" or engine == "mariadb" then
    cfg.CAST_INTEGER = "signed"
else
    cfg.CAST_INTEGER = cfg.INTEGER
end
if engine == "db2" then
    cfg.FROM_ONE = "FROM SYSIBM.SYSDUMMY1"
elseif engine == "firebird" then
    cfg.FROM_ONE = "FROM RDB$DATABASE"
else
    cfg.FROM_ONE = ""
end

local currencies = {
    { code = "cad", prefix = "CAD" },
    { code = "usd", prefix = "USD" },
    { code = "eur", prefix = "EUR" },
    { code = "gbp", prefix = "GBP" },
}

local function currency_block(code, prefix)
    return string.format([[
                    UPDATE ${SCHEMA}course_prices
                       SET unit_amount_cents = CASE
                             WHEN CAST(:%s_KEEP AS ${CAST_INTEGER}) = 1 THEN unit_amount_cents
                             ELSE CAST(:%s_CENTS AS ${CAST_INTEGER})
                           END,
                           is_active = CAST(:%s_ACTIVE AS ${CAST_INTEGER}),
                           updated_id = CAST(:USERID AS ${CAST_INTEGER}),
                           updated_at = ${NOW}
                     WHERE course_id = CAST(:COURSEID AS ${CAST_INTEGER})
                       AND currency = '%s'
                       AND CAST(:%s_PRESENT AS ${CAST_INTEGER}) = 1;
                    -- STATEMENT
                    INSERT INTO ${SCHEMA}course_prices (
                        price_id,
                        course_id,
                        currency,
                        unit_amount_cents,
                        stripe_price_id,
                        is_active,
                        created_id,
                        created_at,
                        updated_id,
                        updated_at
                    )
                    SELECT
                        COALESCE((SELECT MAX(price_id) FROM ${SCHEMA}course_prices), 0) + 1,
                        CAST(:COURSEID AS ${CAST_INTEGER}),
                        '%s',
                        CAST(:%s_CENTS AS ${CAST_INTEGER}),
                        NULL,
                        CAST(:%s_ACTIVE AS ${CAST_INTEGER}),
                        CAST(:USERID AS ${CAST_INTEGER}),
                        ${NOW},
                        CAST(:USERID AS ${CAST_INTEGER}),
                        ${NOW}
                    ${FROM_ONE}
                     WHERE CAST(:%s_PRESENT AS ${CAST_INTEGER}) = 1
                       AND CAST(:%s_KEEP AS ${CAST_INTEGER}) = 0
                       AND CAST(:%s_CENTS AS ${CAST_INTEGER}) > 0
                       AND NOT EXISTS (
                           SELECT 1
                             FROM ${SCHEMA}course_prices existing_price
                            WHERE existing_price.course_id = CAST(:COURSEID AS ${CAST_INTEGER})
                              AND existing_price.currency = '%s'
                       );]],
        prefix, prefix, prefix, code, prefix,
        code, prefix, prefix,
        prefix, prefix, prefix, code)
end

local blocks = {}
for index, item in ipairs(currencies) do
    blocks[index] = currency_block(item.code, item.prefix)
end

local sql_new = [[
                    -- TRANSACTION_START
]] .. table.concat(blocks, "\n                    -- STATEMENT\n") .. [[
                    -- TRANSACTION_END]]

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
                ${TYPE_SQL}                                                         AS query_type_a28,
                ${DIALECT}                                                          AS query_dialect_a30,
                ${QTC_SLOW}                                                         AS query_queue_a58,
                ${TIMEOUT}                                                          AS query_timeout,
                [==[
]] .. sql_new .. [[
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    Not public (TYPE_SQL). One transaction for cad, usd,
                    eur, and gbp. INTEGER binds, all required:

                    COURSEID, USERID, and for each of CAD, USD, EUR, GBP:
                    PRESENT, KEEP, CENTS, ACTIVE.

                    PRESENT 0 skips that currency. KEEP 1 leaves
                    unit_amount_cents and sets is_active from ACTIVE.
                    KEEP 0 writes CENTS. An insert runs only when the
                    row is missing, KEEP is 0, and CENTS is greater
                    than 0. stripe_price_id stays null on insert and
                    is not updated. MySQL and MariaDB cast as signed.
                    DB2 and Firebird select from the dummy row.
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
            # Forward Migration ${MIGRATION}: Populate QueryRef #${QUERY_REF}

            Inserts the staff price save. Course rows stay.
            stripe_price_id is not rewritten.
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
            WHERE query_ref = ${QUERY_REF}
              AND query_type_a28 = ${TYPE_SQL};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove QueryRef #${QUERY_REF}'                                     AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove QueryRef #${QUERY_REF}

            Deletes the TYPE_SQL row only. course_prices rows stay.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
