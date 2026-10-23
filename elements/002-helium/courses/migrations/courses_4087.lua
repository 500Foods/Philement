-- Migration: courses_4087.lua
-- QueryRef #4506 - TRANSACTION_END on its own line
--
-- Does not edit courses_4084.lua. Same caller ref 4506. Lua drops
-- the newline that follows [[, so 4084 glued -- TRANSACTION_END
-- onto the last ");". Hydrogen then sees an open transaction
-- block. This file stores the marker on its own line. Reverse
-- puts the glued text back. The TYPE_SQL row must already exist.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-23 - QueryRef 4506 ends the transaction on its own line

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "4087"
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

local function marked_sql(closing)
    return [[
                    -- TRANSACTION_START
]] .. table.concat(blocks, "\n                    -- STATEMENT\n") .. closing
end

-- 4084's tail has no leading newline, so END shares the last line.
local sql_old = marked_sql("                    -- TRANSACTION_END\n")
local sql_new = marked_sql("\n                    -- TRANSACTION_END\n")

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
            UPDATE ${SCHEMA}${QUERIES}
            SET code = [==[
]] .. sql_new .. [[
            ]==]
            WHERE query_ref = ${QUERY_REF}
            AND query_type_a28 = ${TYPE_SQL};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Rewrite QueryRef #${QUERY_REF} - ${QUERY_NAME}'                    AS name,
        [=[
            # Forward Migration ${MIGRATION}: QueryRef #${QUERY_REF}

            Replaces the SQL for QueryRef #${QUERY_REF}.
            -- TRANSACTION_END is on its own line. stripe_price_id
            is still not written. The row must already exist.
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
            UPDATE ${SCHEMA}${QUERIES}
            SET code = [==[
]] .. sql_old .. [[
            ]==]
            WHERE query_ref = ${QUERY_REF}
            AND query_type_a28 = ${TYPE_SQL};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Restore QueryRef #${QUERY_REF} - ${QUERY_NAME}'                    AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Restore QueryRef #${QUERY_REF}

            Puts the courses_4084.lua SQL back, including the
            TRANSACTION_END that shares a line with the last insert.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
