-- Migration: acuranzo_1348.lua
-- PRIORITIZE 2.35: Catalog.GetBySlug SELECT retired
--
-- Do not restamp 1339. Same published-or-enrolled WHERE.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-08-21 - GetBySlug SELECT retired (PRIORITIZE 2.35)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1348"
cfg.GROUP_NAME = "Catalog"
cfg.SCRIPT_NAME = "GetBySlug"
-- ----------------------------------------------------------------------------
-- Forward: replace Catalog.GetBySlug body
-- ----------------------------------------------------------------------------
table.insert(queries,{sql=[====[

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
            UPDATE ${SCHEMA}scripts
            SET code = [==[
-- Catalog.GetBySlug (PRIORITIZE 2.27; 1348 retired)
-- JWT POST /api/conduit/script. Identity from params._hydrogen.
-- #148-shaped row + published. Bypass published=1 when the caller
-- has a user_enrollments row (any status, including archived).

local function qrows(res)
    if type(res) ~= "table" then return nil end
    if type(res.rows) == "table" then return res.rows end
    return res
end

local function fail(code, message)
    H.set_result_json({ ok = false, code = code, message = message or code })
    return 0
end

local function pick(row, a, b)
    if not row then return nil end
    local v = row[a]
    if v == nil then v = row[b] end
    return v
end

local FIELDS = {
    "course_id", "code", "slug", "title", "summary", "description",
    "image_path", "canvas_course_id", "pricing_type", "is_featured",
    "sort_order", "level", "delivery_style", "license", "source",
    "has_ai", "has_quizzes", "has_badge", "has_certificate", "tags",
    "unit_amount_cents", "currency", "price_fallback", "stripe_price_id",
    "published", "retired",
}

local function row_to_course(row)
    local course = {}
    for i = 1, #FIELDS do
        local k = FIELDS[i]
        course[k] = pick(row, k, string.upper(k))
    end
    return course
end

if type(params) ~= "table" then
    return fail("validation", "Missing params")
end

local h = params._hydrogen
if type(h) ~= "table" then
    return fail("missing_identity", "Sign in required")
end

local account_id = tonumber(h.user_id) or tonumber(h.sub)
if not account_id then
    return fail("missing_identity", "Sign in required")
end

local slug = tostring(params.slug or params.SLUG or "")
slug = string.gsub(slug, "^%s+", "")
slug = string.gsub(slug, "%s+$", "")
if slug == "" then
    return fail("validation", "slug is required")
end

local currency = tostring(params.currency or params.CURRENCY or "cad")
currency = string.lower(currency)
if currency == "" or currency == "null" then
    currency = "cad"
end

local _qr, qerr = H.query_sync([[
    SELECT
        c.course_id,
        c.code,
        c.slug,
        c.title,
        c.summary,
        c.description,
        c.image_path,
        c.canvas_course_id,
        c.pricing_type,
        c.is_featured,
        c.sort_order,
        c.level,
        c.delivery_style,
        c.license,
        c.source,
        c.has_ai,
        c.has_quizzes,
        c.has_badge,
        c.has_certificate,
        c.tags,
        c.published,
        c.retired,
        CASE
            WHEN c.pricing_type = 'free' THEN 0
            ELSE COALESCE(p.unit_amount_cents, p_cad.unit_amount_cents)
        END AS unit_amount_cents,
        CASE
            WHEN c.pricing_type = 'free' THEN LOWER(CAST(:CURRENCY AS VARCHAR(20)))
            WHEN p.price_id IS NOT NULL THEN p.currency
            WHEN p_cad.price_id IS NOT NULL THEN p_cad.currency
            ELSE LOWER(CAST(:CURRENCY AS VARCHAR(20)))
        END AS currency,
        CASE
            WHEN c.pricing_type = 'free' THEN 0
            WHEN p.price_id IS NULL
             AND p_cad.price_id IS NOT NULL
             AND LOWER(CAST(:CURRENCY AS VARCHAR(20))) <> 'cad'
            THEN 1
            ELSE 0
        END AS price_fallback,
        CASE
            WHEN c.pricing_type = 'free' THEN NULL
            WHEN p.price_id IS NOT NULL THEN p.stripe_price_id
            ELSE p_cad.stripe_price_id
        END AS stripe_price_id
    FROM ${SCHEMA}courses c
    LEFT JOIN ${SCHEMA}course_prices p
      ON p.course_id = c.course_id
     AND p.is_active = 1
     AND p.currency = LOWER(CAST(:CURRENCY AS VARCHAR(20)))
    LEFT JOIN ${SCHEMA}course_prices p_cad
      ON p_cad.course_id = c.course_id
     AND p_cad.is_active = 1
     AND p_cad.currency = 'cad'
    WHERE c.slug = :SLUG
      AND (
          c.published = 1
          OR EXISTS (
              SELECT 1
                FROM ${SCHEMA}user_enrollments e
               WHERE e.course_id = c.course_id
                 AND e.account_id = :ACCOUNTID
          )
      )
]], { SLUG = slug, CURRENCY = currency, ACCOUNTID = account_id })
if qerr then
    H.log.warn("GetBySlug: lookup err: %s", tostring(qerr))
    return fail("lookup_error", "Could not load course")
end

local rows = qrows(_qr)
if not rows or not rows[1] then
    H.set_result_json({ ok = true, course = false })
    return 0
end

H.set_result_json({
    ok = true,
    course = row_to_course(rows[1]),
})
return 0
                ]==]
            WHERE group_name = 'Catalog'
              AND script_name = 'GetBySlug';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'GetBySlug SELECT retired' AS name,
        [=[
            # Forward Migration ${MIGRATION}: GetBySlug SELECT retired (do not restamp 1339)
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Reverse: restore prior body
-- ----------------------------------------------------------------------------
table.insert(queries,{sql=[====[

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
            UPDATE ${SCHEMA}scripts
            SET code = [==[
-- Catalog.GetBySlug (PRIORITIZE 2.27)
-- JWT POST /api/conduit/script. Identity from params._hydrogen.
-- #148-shaped row + published. Bypass published=1 when the caller
-- has a user_enrollments row (any status, including archived).

local function qrows(res)
    if type(res) ~= "table" then return nil end
    if type(res.rows) == "table" then return res.rows end
    return res
end

local function fail(code, message)
    H.set_result_json({ ok = false, code = code, message = message or code })
    return 0
end

local function pick(row, a, b)
    if not row then return nil end
    local v = row[a]
    if v == nil then v = row[b] end
    return v
end

local FIELDS = {
    "course_id", "code", "slug", "title", "summary", "description",
    "image_path", "canvas_course_id", "pricing_type", "is_featured",
    "sort_order", "level", "delivery_style", "license", "source",
    "has_ai", "has_quizzes", "has_badge", "has_certificate", "tags",
    "unit_amount_cents", "currency", "price_fallback", "stripe_price_id",
    "published",
}

local function row_to_course(row)
    local course = {}
    for i = 1, #FIELDS do
        local k = FIELDS[i]
        course[k] = pick(row, k, string.upper(k))
    end
    return course
end

if type(params) ~= "table" then
    return fail("validation", "Missing params")
end

local h = params._hydrogen
if type(h) ~= "table" then
    return fail("missing_identity", "Sign in required")
end

local account_id = tonumber(h.user_id) or tonumber(h.sub)
if not account_id then
    return fail("missing_identity", "Sign in required")
end

local slug = tostring(params.slug or params.SLUG or "")
slug = string.gsub(slug, "^%s+", "")
slug = string.gsub(slug, "%s+$", "")
if slug == "" then
    return fail("validation", "slug is required")
end

local currency = tostring(params.currency or params.CURRENCY or "cad")
currency = string.lower(currency)
if currency == "" or currency == "null" then
    currency = "cad"
end

local _qr, qerr = H.query_sync([[
    SELECT
        c.course_id,
        c.code,
        c.slug,
        c.title,
        c.summary,
        c.description,
        c.image_path,
        c.canvas_course_id,
        c.pricing_type,
        c.is_featured,
        c.sort_order,
        c.level,
        c.delivery_style,
        c.license,
        c.source,
        c.has_ai,
        c.has_quizzes,
        c.has_badge,
        c.has_certificate,
        c.tags,
        c.published,
        CASE
            WHEN c.pricing_type = 'free' THEN 0
            ELSE COALESCE(p.unit_amount_cents, p_cad.unit_amount_cents)
        END AS unit_amount_cents,
        CASE
            WHEN c.pricing_type = 'free' THEN LOWER(CAST(:CURRENCY AS VARCHAR(20)))
            WHEN p.price_id IS NOT NULL THEN p.currency
            WHEN p_cad.price_id IS NOT NULL THEN p_cad.currency
            ELSE LOWER(CAST(:CURRENCY AS VARCHAR(20)))
        END AS currency,
        CASE
            WHEN c.pricing_type = 'free' THEN 0
            WHEN p.price_id IS NULL
             AND p_cad.price_id IS NOT NULL
             AND LOWER(CAST(:CURRENCY AS VARCHAR(20))) <> 'cad'
            THEN 1
            ELSE 0
        END AS price_fallback,
        CASE
            WHEN c.pricing_type = 'free' THEN NULL
            WHEN p.price_id IS NOT NULL THEN p.stripe_price_id
            ELSE p_cad.stripe_price_id
        END AS stripe_price_id
    FROM ${SCHEMA}courses c
    LEFT JOIN ${SCHEMA}course_prices p
      ON p.course_id = c.course_id
     AND p.is_active = 1
     AND p.currency = LOWER(CAST(:CURRENCY AS VARCHAR(20)))
    LEFT JOIN ${SCHEMA}course_prices p_cad
      ON p_cad.course_id = c.course_id
     AND p_cad.is_active = 1
     AND p_cad.currency = 'cad'
    WHERE c.slug = :SLUG
      AND (
          c.published = 1
          OR EXISTS (
              SELECT 1
                FROM ${SCHEMA}user_enrollments e
               WHERE e.course_id = c.course_id
                 AND e.account_id = :ACCOUNTID
          )
      )
]], { SLUG = slug, CURRENCY = currency, ACCOUNTID = account_id })
if qerr then
    H.log.warn("GetBySlug: lookup err: %s", tostring(qerr))
    return fail("lookup_error", "Could not load course")
end

local rows = qrows(_qr)
if not rows or not rows[1] then
    H.set_result_json({ ok = true, course = false })
    return 0
end

H.set_result_json({
    ok = true,
    course = row_to_course(rows[1]),
})
return 0
                ]==]
            WHERE group_name = 'Catalog'
              AND script_name = 'GetBySlug';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Restore Catalog.GetBySlug without retired' AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Restore prior Catalog.GetBySlug
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
