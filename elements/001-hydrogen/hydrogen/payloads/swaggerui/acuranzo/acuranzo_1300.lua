-- Migration: acuranzo_1300.lua
-- Phase 49: seed Enroll.FreeCourse (invokable) for Reception free enroll
--
-- Interactive enroll via JWT POST /api/conduit/script. Reuses the Canvas
-- enroll POST proven in Provision.EnsureCanvasUser (acuranzo_1290) but is
-- callable by the SPA. No Hydrogen C. Data-only seed (no diagram).

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-08-12 - Seed Enroll.FreeCourse invokable=1 (FINISHLINE FL-49)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1300"
cfg.GROUP_NAME = "Enroll"
cfg.SCRIPT_NAME = "FreeCourse"
-- ----------------------------------------------------------------------------
-- Forward: seed Enroll.FreeCourse (invokable)
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
            INSERT INTO ${SCHEMA}scripts (
                group_name,
                script_name,
                script_type,
                schedule,
                next_run,
                last_run_start,
                last_run_end,
                status,
                code,
                summary,
                invokable,
                ${COMMON_FIELDS}
            )
            VALUES (
                '${GROUP_NAME}',
                '${SCRIPT_NAME}',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
-- Enroll.FreeCourse (Phase 49)
-- JWT POST /api/conduit/script. Identity from params._hydrogen.
-- Enroll the caller into params.canvas_course_id only if that catalog
-- course is published and pricing_type = free.
-- Secrets: os.getenv("CANVAS_API_KEY"), os.getenv("CANVAS_BASE_URL").

local function getenv(k, default)
    if type(os) == "table" and type(os.getenv) == "function" then
        local v = os.getenv(k)
        if v and v ~= "" then return v end
    end
    return default
end

local function qrows(res)
    if type(res) ~= "table" then return nil end
    if type(res.rows) == "table" then return res.rows end
    return res
end

local function fail(code, message)
    H.set_result_json({ ok = false, code = code, message = message or code })
    return 0
end

local function succeed(already, canvas_user_id, canvas_course_id)
    H.set_result_json({
        ok = true,
        already_enrolled = already and true or false,
        canvas_user_id = canvas_user_id,
        canvas_course_id = canvas_course_id,
    })
    return 0
end

local BASE = (getenv("CANVAS_BASE_URL", "https://canvas.500courses.com")):gsub("/+$", "")
local TOKEN = getenv("CANVAS_API_KEY", nil)
local ACCOUNT_PATH = "/api/v1/accounts/1"

local function auth_headers()
    return {
        Authorization = "Bearer " .. TOKEN,
        Accept = "application/json",
    }
end

local function http_get(url)
    local res, err = H.http.get_sync(url, auth_headers(), { timeout = 10 })
    if err then return nil, err end
    return res, nil
end

local function http_post_json(url, body)
    local headers = auth_headers()
    headers["Content-Type"] = "application/json"
    local res, err = H.http.post_sync(url, body, headers, {
        timeout = 10,
        content_type = "application/json",
    })
    if err then return nil, err end
    return res, nil
end

local function url_encode(s)
    s = tostring(s or "")
    return (s:gsub("([^%w%-_%.~])", function(c)
        return string.format("%%%02X", string.byte(c))
    end))
end

local function each_object(body)
    local objs = {}
    if type(body) ~= "string" or body == "" then return objs end
    for obj in body:gmatch("%b{}") do
        objs[#objs + 1] = obj
    end
    return objs
end

local function object_id(obj)
    return tonumber(obj:match('"id"%s*:%s*(%d+)'))
end

local function object_field(obj, key)
    return obj:match('"' .. key .. '"%s*:%s*"([^"]*)"')
end

local function emails_equal(a, b)
    if not a or not b then return false end
    return string.lower(a) == string.lower(b)
end

local function canvas_find_by_email(email)
    local url = BASE .. ACCOUNT_PATH .. "/users?search_term=" .. url_encode(email)
        .. "&per_page=50"
    local res, err = http_get(url)
    if not res then
        return nil, "canvas_search_failed:" .. tostring(err)
    end
    if res.status < 200 or res.status >= 300 then
        return nil, "canvas_search_http_" .. tostring(res.status)
    end
    local exact = {}
    for _, obj in ipairs(each_object(res.body or "")) do
        local id = object_id(obj)
        if id then
            local login_id = object_field(obj, "login_id") or ""
            local em = object_field(obj, "email") or ""
            if emails_equal(login_id, email) or emails_equal(em, email) then
                exact[#exact + 1] = id
            end
        end
    end
    if #exact == 1 then return exact[1], nil end
    if #exact > 1 then return nil, "email_ambiguous" end
    return nil, "not_found"
end

local function canvas_create_user(email, name)
    local display = name
    if not display or display == "" then
        display = email
    end
    local body = string.format(
        '{"user":{"name":%q,"skip_registration":true},"pseudonym":{"unique_id":%q,"send_confirmation":false},"communication_channel":{"type":"email","address":%q,"skip_confirmation":true}}',
        display, email, email
    )
    local url = BASE .. ACCOUNT_PATH .. "/users"
    local res, err = http_post_json(url, body)
    if not res then
        return nil, "canvas_create_failed:" .. tostring(err)
    end
    if res.status < 200 or res.status >= 300 then
        H.log.warn("FreeCourse: create HTTP %s body=%s",
            tostring(res.status), string.sub(tostring(res.body or ""), 1, 300))
        return nil, "canvas_create_http_" .. tostring(res.status)
    end
    local id = object_id(res.body or "")
    if not id then
        return nil, "canvas_create_no_id"
    end
    return id, nil
end

local function link_lookup(account_id)
    local _qr, err = H.query_sync([[
        SELECT link_id, account_id, canvas_user_id, canvas_email, last_seen_at
        FROM ${SCHEMA}account_canvas_links
        WHERE account_id = :ACCOUNTID
    ]], { ACCOUNTID = account_id })
    if err then return nil, err end
    local rows = qrows(_qr)
    if rows and rows[1] then return rows[1], nil end
    return nil, nil
end

local function link_insert(account_id, canvas_user_id, canvas_email)
    local _, err = H.query_sync([[
        INSERT INTO ${SCHEMA}account_canvas_links (
            link_id, account_id, canvas_user_id, canvas_email, last_seen_at,
            valid_after, valid_until, created_id, created_at, updated_id, updated_at
        )
        SELECT
            (SELECT COALESCE(MAX(link_id), 0) + 1 FROM ${SCHEMA}account_canvas_links),
            a.account_id,
            :CANVASUSERID,
            :CANVASEMAIL,
            NOW(),
            '2025-01-01 00:00:00',
            '2035-01-01 00:00:00',
            0, NOW(), 0, NOW()
        FROM ${SCHEMA}accounts a
        WHERE a.account_id = :ACCOUNTID
          AND NOT EXISTS (
              SELECT 1 FROM ${SCHEMA}account_canvas_links acl
              WHERE acl.account_id = :ACCOUNTID
          )
    ]], {
        ACCOUNTID = account_id,
        CANVASUSERID = canvas_user_id,
        CANVASEMAIL = canvas_email,
    })
    if err then return nil, err end
    local existing = select(1, link_lookup(account_id))
    if existing then
        return existing.canvas_user_id or existing.CANVAS_USER_ID, nil
    end
    return nil, nil
end

local function ensure_link(account_id, email, display_name)
    local existing, lerr = link_lookup(account_id)
    if lerr then
        return nil, "link_lookup_failed"
    end
    if existing then
        return existing.canvas_user_id or existing.CANVAS_USER_ID, nil
    end
    if not email or email == "" then
        return nil, "unlinked"
    end
    local canvas_id, ferr = canvas_find_by_email(email)
    if ferr == "email_ambiguous" then
        return nil, "email_ambiguous"
    end
    if ferr and ferr ~= "not_found" then
        return nil, tostring(ferr)
    end
    if not canvas_id then
        canvas_id, ferr = canvas_create_user(email, display_name)
        if not canvas_id then
            return nil, tostring(ferr or "canvas_create_failed")
        end
        H.log.info("FreeCourse: created canvas_user_id=%s for account_id=%s",
            tostring(canvas_id), tostring(account_id))
    end
    local linked, ierr = link_insert(account_id, canvas_id, email)
    if ierr then
        return nil, "link_insert_failed"
    end
    if not linked then
        existing = select(1, link_lookup(account_id))
        if existing then
            return existing.canvas_user_id or existing.CANVAS_USER_ID, nil
        end
        return nil, "link_insert_failed"
    end
    return linked, nil
end

local function already_enrolled(canvas_user_id, course_id)
    local url = BASE .. "/api/v1/courses/" .. tostring(course_id)
        .. "/enrollments?user_id=" .. tostring(canvas_user_id)
        .. "&state[]=active&per_page=5"
    local res = http_get(url)
    if not res or res.status < 200 or res.status >= 300 then
        return false
    end
    for _, obj in ipairs(each_object(res.body or "")) do
        if object_id(obj) then
            return true
        end
    end
    return false
end

local function enroll_student(canvas_user_id, course_id)
    local body = string.format(
        '{"enrollment":{"user_id":%d,"type":"StudentEnrollment","enrollment_state":"active","notify":false}}',
        canvas_user_id
    )
    local url = BASE .. "/api/v1/courses/" .. tostring(course_id) .. "/enrollments"
    local res, err = http_post_json(url, body)
    if not res then
        return false, "canvas_enroll_failed:" .. tostring(err)
    end
    if res.status >= 200 and res.status < 300 then
        return true, nil
    end
    local snippet = string.lower(string.sub(tostring(res.body or ""), 1, 400))
    if string.find(snippet, "already", 1, true) then
        return true, "already"
    end
    H.log.warn("FreeCourse: enroll HTTP %s body=%s",
        tostring(res.status), string.sub(tostring(res.body or ""), 1, 300))
    return false, "canvas_enroll_http_" .. tostring(res.status)
end

H.set_current_state("start")

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

local canvas_course_id = tonumber(params.canvas_course_id)
if not canvas_course_id then
    return fail("validation", "Missing canvas course id")
end

if not TOKEN or TOKEN == "" then
    H.log.error("FreeCourse: CANVAS_API_KEY not set in environment")
    return fail("canvas_unconfigured", "Enrollment is temporarily unavailable")
end

local _qr, qerr = H.query_sync([[
    SELECT course_id, slug, pricing_type, published, canvas_course_id
    FROM ${SCHEMA}courses
    WHERE canvas_course_id = :CANVASCOURSEID
      AND published = 1
]], { CANVASCOURSEID = canvas_course_id })
if qerr then
    H.log.warn("FreeCourse: catalog lookup err: %s", tostring(qerr))
    return fail("catalog_error", "Could not verify course")
end
local rows = qrows(_qr)
if not rows or not rows[1] then
    return fail("course_not_found", "Course is not available")
end
local course = rows[1]
local pricing = string.lower(tostring(course.pricing_type or course.PRICING_TYPE or ""))
if pricing ~= "free" then
    return fail("not_free", "This course is not free")
end
if params.course_id ~= nil and params.course_id ~= "" then
    local want = tonumber(params.course_id)
    local got = tonumber(course.course_id or course.COURSE_ID)
    if want and got and want ~= got then
        return fail("course_mismatch", "Course does not match catalog")
    end
end

local email = h.email
local display = h.username or email
local canvas_user_id, lerr = ensure_link(account_id, email, display)
if not canvas_user_id then
    return fail(lerr or "unlinked", "Canvas account is not ready yet")
end

if already_enrolled(canvas_user_id, canvas_course_id) then
    H.set_current_state("done")
    return succeed(true, canvas_user_id, canvas_course_id)
end

local enrolled, eerr = enroll_student(canvas_user_id, canvas_course_id)
if not enrolled then
    return fail(eerr or "canvas_enroll_failed", "Enrollment failed")
end

H.set_current_state("done")
return succeed(eerr == "already", canvas_user_id, canvas_course_id)
                ]==],
                'Phase 49: invokable free-course enroll (Canvas H.http)',
                1,
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Enroll.FreeCourse invokable script'                           AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Enroll.FreeCourse

            Inserts `Enroll.FreeCourse` with `invokable = 1` for Reception
            `POST /api/conduit/script` (FINISHLINE Phase 49).

            The worker:

            1. Reads `params._hydrogen` (`user_id` / `sub`, `email`).
            2. Resolves or creates `account_canvas_links` (same Canvas
               search/create as Phase 30).
            3. Loads `${SCHEMA}courses` by `canvas_course_id`; requires
               `published = 1` and `pricing_type = 'free'`.
            4. Enrolls via Canvas `POST /api/v1/courses/:id/enrollments`
               (`StudentEnrollment` / `active`). Already-enrolled is
               `ok=true, already_enrolled=true`.

            Business outcomes use `H.set_result_json` (HTTP 200 +
            `status=completed`). Secrets stay in env (`CANVAS_*`).
            No diagram (data seed only, same as 1296).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Reverse
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
            DELETE FROM ${SCHEMA}scripts
            WHERE group_name = 'Enroll'
              AND script_name = 'FreeCourse';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Enroll.FreeCourse script'                                   AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Enroll.FreeCourse
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
