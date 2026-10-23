-- Migration: courses_4085.lua
-- QueryRef #4507 - Staff: List Course Orders
--
-- 4507 is not a file number. Orders for one course. ACCOUNTID 0
-- lists every account on that course. A positive ACCOUNTID narrows
-- the list. items_json is the checkout snapshot. PostgreSQL stores
-- it as jsonb, so the match is containment. Text engines match the
-- compact {"courseId":N, token written by Stripe.Checkout.
-- stripe_intent_id and stripe_customer_id are identifiers. The
-- client hides a secret-shaped value. This statement does not write.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-23 - Staff orders for one course, optional account

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "4085"
cfg.QUERY_REF = "4507"
cfg.QUERY_NAME = "Staff: List Course Orders"
if engine == "mysql" or engine == "mariadb" then
    cfg.CAST_INTEGER = "signed"
else
    cfg.CAST_INTEGER = cfg.INTEGER
end

local course_match
if engine == "postgresql" or engine == "postgres" then
    course_match = "o.items_json @> jsonb_build_array(jsonb_build_object('courseId', CAST(:COURSEID AS ${CAST_INTEGER})))"
elseif engine == "mysql" or engine == "mariadb" then
    course_match = "CAST(o.items_json AS CHAR) LIKE CONCAT('%{\"courseId\":', CAST(:COURSEID AS ${CAST_INTEGER}), ',%')"
elseif engine == "mssql" then
    course_match = "CAST(o.items_json AS NVARCHAR(MAX)) LIKE '%{\"courseId\":' + CAST(CAST(:COURSEID AS ${CAST_INTEGER}) AS NVARCHAR(20)) + ',%'"
elseif engine == "db2" or engine == "firebird" then
    course_match = "CAST(o.items_json AS VARCHAR(8000)) LIKE '%{\"courseId\":' || CAST(:COURSEID AS VARCHAR(20)) || ',%'"
else
    course_match = "CAST(o.items_json AS TEXT) LIKE '%{\"courseId\":' || CAST(:COURSEID AS TEXT) || ',%'"
end

local sql_new = [[
                    SELECT o.order_id,
                           o.order_number,
                           o.account_id,
                           o.status,
                           o.currency,
                           o.total_cents,
                           o.stripe_intent_id,
                           o.created_at,
                           o.refunded_at,
                           a.stripe_customer_id
                      FROM ${SCHEMA}orders o
                      LEFT JOIN ${SCHEMA}accounts a
                        ON a.account_id = o.account_id
                     WHERE ]] .. course_match .. [[
                       AND (
                             CAST(:ACCOUNTID AS ${CAST_INTEGER}) = 0
                             OR o.account_id = CAST(:ACCOUNTID AS ${CAST_INTEGER})
                           )
                     ORDER BY o.created_at DESC, o.order_id DESC]]

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
        ${QTC_FAST}                                                         AS query_queue_a58,
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
                ${QTC_FAST}                                                         AS query_queue_a58,
                ${TIMEOUT}                                                          AS query_timeout,
                [==[
]] .. sql_new .. [[
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    Not public (TYPE_SQL). INTEGER COURSEID is required.
                    INTEGER ACCOUNTID 0 means every account for that
                    course. A positive ACCOUNTID narrows the rows.
                    Status may be pending, completed, failed, or
                    refunded. The PaymentIntent id and the customer
                    id are returned for the Dashboard link. MySQL
                    casts the ids as signed. PostgreSQL matches
                    courseId inside items_json with jsonb containment.
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

            Inserts the staff order list. Order rows stay.
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
        ${QTC_FAST}                                                         AS query_queue_a58,
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

            Deletes the TYPE_SQL row only. orders rows stay.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
