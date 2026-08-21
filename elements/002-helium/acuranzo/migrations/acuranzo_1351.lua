-- Migration: acuranzo_1351.lua
-- PRIORITIZE 2.35: Enroll.FreeCourse fail-closed on retired
--
-- Do not restamp 1344.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-08-21 - FreeCourse not_offered when retired (PRIORITIZE 2.35)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1351"
cfg.GROUP_NAME = "Enroll"
cfg.SCRIPT_NAME = "FreeCourse"
-- ----------------------------------------------------------------------------
-- Forward: replace Enroll.FreeCourse body
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
-- Enroll.FreeCourse (Phase 49; 1321 user_enrollments; 1328 LogEvent; 1344 find-dont-create; 1351 retired)
-- PRIORITIZE 2.1: never create a Canvas user. Attach if JIT/search finds one.
-- After Canvas enroll (or already-enrolled), UPSERT Helium
-- user_enrollments source=free. Any existing row (including
-- archived) is left alone — archive is sticky.
-- New grants also append enrollment_events via Enroll.LogEvent.
-- JWT POST /api/conduit/script. Identity from params._hydrogen.
-- Enroll the caller into params.canvas_course_id only if that catalog
-- course is published, not retired, and pricing_type = free.
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

local function pick(row, a, b)
    if not row then return nil end
    local v = row[a]
    if v == nil then v = row[b] end
    return v
end

local function try_log_event(opts)
    local ok, mod = pcall(require, "Enroll.LogEvent")
    if not ok or type(mod) ~= "table" or type(mod.record) ~= "function" then
        H.log.warn("FreeCourse: LogEvent unavailable: %s", tostring(mod))
        return
    end
    local r = mod.record(opts)
    if type(r) == "table" and r.ok == false then
        H.log.warn("FreeCourse: LogEvent: %s", tostring(r.code or r.message))
    end
end

local function next_enrollment_id()
    local _nr = H.query_sync([[
        SELECT COALESCE(MAX(enrollment_id), 0) + 1 AS next_id
        FROM ${SCHEMA}user_enrollments
    ]], {})
    local rows = qrows(_nr)
    return tonumber(pick(rows and rows[1], "next_id", "NEXT_ID")) or 1
end

local function any_entitlement(account_id, course_id)
    local _er, qerr = H.query_sync([[
        SELECT enrollment_id
        FROM ${SCHEMA}user_enrollments
        WHERE account_id = :ACCOUNTID
          AND course_id = :COURSEID
        ORDER BY enrollment_id DESC
    ]], { ACCOUNTID = account_id, COURSEID = course_id })
    if qerr then return nil, qerr end
    local rows = qrows(_er)
    if rows and rows[1] then
        return tonumber(pick(rows[1], "enrollment_id", "ENROLLMENT_ID")), nil
    end
    return nil, nil
end

local function grant_entitlement(account_id, course_id, canvas_course_id, source, renew_policy, lifetime)
    if not account_id or not course_id then
        return "missing_ids"
    end
    local existing, eerr = any_entitlement(account_id, course_id)
    if eerr then return "entitlement_lookup_failed" end
    if existing then return "exists" end
    local eid = next_enrollment_id()
    local expires_sql = lifetime and "NULL" or "NOW() + INTERVAL '90 days'"
    local sql = [[
        INSERT INTO ${SCHEMA}user_enrollments (
            enrollment_id, account_id, course_id,
            canvas_enrollment_id, canvas_course_id,
            status, enrolled_at, expires_at,
            completed_at, progress_percent, progress_synced_at,
            archived_at, renew_policy, source, order_id,
            valid_after, valid_until, created_id, created_at, updated_id, updated_at
        ) VALUES (
            :EID, :ACCOUNTID, :COURSEID,
            NULL, :CANVASCOURSE,
            'active', NOW(), ]] .. expires_sql .. [[,
            NULL, 0, NULL,
            NULL, :RENEWPOLICY, :SOURCE, NULL,
            NULL, NULL, 0, NOW(), 0, NOW()
        )
    ]]
    local canvas_bind = canvas_course_id
    if canvas_bind == nil then canvas_bind = 0 end
    local _, ierr = H.query_sync(sql, {
        EID = eid,
        ACCOUNTID = account_id,
        COURSEID = course_id,
        CANVASCOURSE = canvas_bind,
        RENEWPOLICY = renew_policy,
        SOURCE = source,
    })
    if ierr then
        local again = select(1, any_entitlement(account_id, course_id))
        if again then return "exists" end
        return "entitlement_error"
    end
    return nil
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

local function ids_equal(a, b)
    if not a or not b then return false end
    return string.lower(tostring(a)) == string.lower(tostring(b))
end

local function collect_exact_ids(body, email, sis_subject)
    local exact = {}
    local seen = {}
    local function add(id)
        if id and not seen[id] then
            seen[id] = true
            exact[#exact + 1] = id
        end
    end
    for _, obj in ipairs(each_object(body or "")) do
        local id = object_id(obj)
        if id then
            local login_id = object_field(obj, "login_id") or ""
            local em = object_field(obj, "email") or ""
            local sis = object_field(obj, "sis_user_id") or ""
            if emails_equal(login_id, email) or emails_equal(em, email)
                or emails_equal(sis, email)
                or (sis_subject and sis_subject ~= "" and ids_equal(sis, sis_subject)) then
                add(id)
            end
        end
    end
    return exact
end

local function canvas_get_user_sis(sis_id)
    if not sis_id or sis_id == "" then
        return nil, "not_found"
    end
    local url = BASE .. "/api/v1/users/sis_user_id:" .. url_encode(sis_id)
    local res, err = http_get(url)
    if not res then
        return nil, "canvas_sis_failed:" .. tostring(err)
    end
    if res.status == 404 then
        return nil, "not_found"
    end
    if res.status < 200 or res.status >= 300 then
        return nil, "canvas_sis_http_" .. tostring(res.status)
    end
    local id = object_id(res.body or "")
    if not id then
        return nil, "incomplete"
    end
    return id, nil
end

local function canvas_user_matches(user_id, email, sis_subject)
    local url = BASE .. "/api/v1/users/" .. tostring(user_id) .. "/logins?per_page=50"
    local res = http_get(url)
    if res and res.status >= 200 and res.status < 300 then
        for _, obj in ipairs(each_object(res.body or "")) do
            local uid = object_field(obj, "unique_id") or ""
            local sis = object_field(obj, "sis_user_id") or ""
            if emails_equal(uid, email) or emails_equal(sis, email)
                or (sis_subject and sis_subject ~= "" and (
                    ids_equal(sis, sis_subject) or ids_equal(uid, sis_subject))) then
                return true
            end
        end
    end
    url = BASE .. "/api/v1/users/" .. tostring(user_id)
        .. "/communication_channels?per_page=50"
    res = http_get(url)
    if res and res.status >= 200 and res.status < 300 then
        for _, obj in ipairs(each_object(res.body or "")) do
            local addr = object_field(obj, "address") or ""
            if emails_equal(addr, email) then
                return true
            end
        end
    end
    return false
end

local function canvas_find_user(email, sis_subject)
    if not email or email == "" then
        return nil, "not_found"
    end
    local id, err = canvas_get_user_sis(email)
    if id then return id, nil end
    if err == "incomplete" then
        return nil, "incomplete"
    end
    if sis_subject and sis_subject ~= "" then
        id, err = canvas_get_user_sis(sis_subject)
        if id then return id, nil end
        if err == "incomplete" then
            return nil, "incomplete"
        end
    end
    local url = BASE .. "/api/v1/accounts/1/users?search_term=" .. url_encode(email)
        .. "&per_page=50"
    local res, herr = http_get(url)
    if not res then
        return nil, "canvas_search_failed:" .. tostring(herr)
    end
    if res.status < 200 or res.status >= 300 then
        return nil, "canvas_search_http_" .. tostring(res.status)
    end
    local exact = collect_exact_ids(res.body, email, sis_subject)
    if #exact == 1 then return exact[1], nil end
    if #exact > 1 then return nil, "email_ambiguous" end
    local probed = {}
    local seen = {}
    for _, obj in ipairs(each_object(res.body or "")) do
        local uid = object_id(obj)
        if uid and not seen[uid] then
            seen[uid] = true
            if canvas_user_matches(uid, email, sis_subject) then
                probed[#probed + 1] = uid
            end
        end
    end
    if #probed == 1 then return probed[1], nil end
    if #probed > 1 then return nil, "email_ambiguous" end
    if sis_subject and sis_subject ~= ""
        and string.lower(tostring(sis_subject)) ~= string.lower(tostring(email)) then
        url = BASE .. "/api/v1/accounts/1/users?search_term="
            .. url_encode(sis_subject) .. "&per_page=50"
        res, herr = http_get(url)
        if not res then
            return nil, "canvas_search_failed:" .. tostring(herr)
        end
        if res.status < 200 or res.status >= 300 then
            return nil, "canvas_search_http_" .. tostring(res.status)
        end
        exact = collect_exact_ids(res.body, email, sis_subject)
        if #exact == 1 then return exact[1], nil end
        if #exact > 1 then return nil, "email_ambiguous" end
        for _, obj in ipairs(each_object(res.body or "")) do
            local uid = object_id(obj)
            if uid and not seen[uid] then
                seen[uid] = true
                if canvas_user_matches(uid, email, sis_subject) then
                    probed[#probed + 1] = uid
                end
            end
        end
        if #probed == 1 then return probed[1], nil end
        if #probed > 1 then return nil, "email_ambiguous" end
    end
    return nil, "not_found"
end

local function link_lookup(account_id)
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

local function oidc_subject(account_id)
    local _sr, serr = H.query_sync([[
        SELECT subject
        FROM ${SCHEMA}account_oidc_identities
        WHERE account_id = :ACCOUNTID
        ORDER BY identity_id
    ]], { ACCOUNTID = account_id })
    if serr then
        return ""
    end
    local rows = qrows(_sr)
    if rows and rows[1] then
        return tostring(rows[1].subject or rows[1].SUBJECT or "")
    end
    return ""
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
        return nil, "not_provisioned"
    end
    local canvas_id, ferr = canvas_find_user(email, oidc_subject(account_id))
    if not canvas_id then
        H.log.info("FreeCourse: wait JIT account_id=%s reason=%s",
            tostring(account_id), tostring(ferr or "not_found"))
        return nil, "not_provisioned"
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
    SELECT course_id, slug, pricing_type, published, canvas_course_id, retired
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
local retired = tonumber(course.retired or course.RETIRED) or 0
if retired ~= 0 then
    return fail("not_offered", "This course is no longer offered")
end
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

local helium_course_id = tonumber(course.course_id or course.COURSE_ID)

local function persist_free()
    local gerr = grant_entitlement(
        account_id, helium_course_id, canvas_course_id,
        "free", "free_renew", false)
    if gerr == "exists" then
        return
    end
    if gerr then
        H.log.warn("FreeCourse: entitlement write err: %s", tostring(gerr))
        return
    end
    try_log_event({
        account_id = account_id,
        course_id = helium_course_id,
        canvas_user_id = canvas_user_id,
        canvas_course_id = canvas_course_id,
        event_type = "enrolled",
        actor = "self",
    })
end

if already_enrolled(canvas_user_id, canvas_course_id) then
    persist_free()
    H.set_current_state("done")
    return succeed(true, canvas_user_id, canvas_course_id)
end

local enrolled, eerr = enroll_student(canvas_user_id, canvas_course_id)
if not enrolled then
    return fail(eerr or "canvas_enroll_failed", "Enrollment failed")
end

persist_free()
H.set_current_state("done")
return succeed(eerr == "already", canvas_user_id, canvas_course_id)
            ]==]
            WHERE group_name = 'Enroll'
              AND script_name = 'FreeCourse';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'FreeCourse fail-closed on retired' AS name,
        [=[
            # Forward Migration ${MIGRATION}: FreeCourse fail-closed on retired=1 (do not restamp 1344)
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
-- Enroll.FreeCourse (Phase 49; 1321 user_enrollments; 1328 LogEvent; 1344 find-dont-create)
-- PRIORITIZE 2.1: never create a Canvas user. Attach if JIT/search finds one.
-- After Canvas enroll (or already-enrolled), UPSERT Helium
-- user_enrollments source=free. Any existing row (including
-- archived) is left alone — archive is sticky.
-- New grants also append enrollment_events via Enroll.LogEvent.
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

local function pick(row, a, b)
    if not row then return nil end
    local v = row[a]
    if v == nil then v = row[b] end
    return v
end

local function try_log_event(opts)
    local ok, mod = pcall(require, "Enroll.LogEvent")
    if not ok or type(mod) ~= "table" or type(mod.record) ~= "function" then
        H.log.warn("FreeCourse: LogEvent unavailable: %s", tostring(mod))
        return
    end
    local r = mod.record(opts)
    if type(r) == "table" and r.ok == false then
        H.log.warn("FreeCourse: LogEvent: %s", tostring(r.code or r.message))
    end
end

local function next_enrollment_id()
    local _nr = H.query_sync([[
        SELECT COALESCE(MAX(enrollment_id), 0) + 1 AS next_id
        FROM ${SCHEMA}user_enrollments
    ]], {})
    local rows = qrows(_nr)
    return tonumber(pick(rows and rows[1], "next_id", "NEXT_ID")) or 1
end

local function any_entitlement(account_id, course_id)
    local _er, qerr = H.query_sync([[
        SELECT enrollment_id
        FROM ${SCHEMA}user_enrollments
        WHERE account_id = :ACCOUNTID
          AND course_id = :COURSEID
        ORDER BY enrollment_id DESC
    ]], { ACCOUNTID = account_id, COURSEID = course_id })
    if qerr then return nil, qerr end
    local rows = qrows(_er)
    if rows and rows[1] then
        return tonumber(pick(rows[1], "enrollment_id", "ENROLLMENT_ID")), nil
    end
    return nil, nil
end

local function grant_entitlement(account_id, course_id, canvas_course_id, source, renew_policy, lifetime)
    if not account_id or not course_id then
        return "missing_ids"
    end
    local existing, eerr = any_entitlement(account_id, course_id)
    if eerr then return "entitlement_lookup_failed" end
    if existing then return "exists" end
    local eid = next_enrollment_id()
    local expires_sql = lifetime and "NULL" or "NOW() + INTERVAL '90 days'"
    local sql = [[
        INSERT INTO ${SCHEMA}user_enrollments (
            enrollment_id, account_id, course_id,
            canvas_enrollment_id, canvas_course_id,
            status, enrolled_at, expires_at,
            completed_at, progress_percent, progress_synced_at,
            archived_at, renew_policy, source, order_id,
            valid_after, valid_until, created_id, created_at, updated_id, updated_at
        ) VALUES (
            :EID, :ACCOUNTID, :COURSEID,
            NULL, :CANVASCOURSE,
            'active', NOW(), ]] .. expires_sql .. [[,
            NULL, 0, NULL,
            NULL, :RENEWPOLICY, :SOURCE, NULL,
            NULL, NULL, 0, NOW(), 0, NOW()
        )
    ]]
    local canvas_bind = canvas_course_id
    if canvas_bind == nil then canvas_bind = 0 end
    local _, ierr = H.query_sync(sql, {
        EID = eid,
        ACCOUNTID = account_id,
        COURSEID = course_id,
        CANVASCOURSE = canvas_bind,
        RENEWPOLICY = renew_policy,
        SOURCE = source,
    })
    if ierr then
        local again = select(1, any_entitlement(account_id, course_id))
        if again then return "exists" end
        return "entitlement_error"
    end
    return nil
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

local function ids_equal(a, b)
    if not a or not b then return false end
    return string.lower(tostring(a)) == string.lower(tostring(b))
end

local function collect_exact_ids(body, email, sis_subject)
    local exact = {}
    local seen = {}
    local function add(id)
        if id and not seen[id] then
            seen[id] = true
            exact[#exact + 1] = id
        end
    end
    for _, obj in ipairs(each_object(body or "")) do
        local id = object_id(obj)
        if id then
            local login_id = object_field(obj, "login_id") or ""
            local em = object_field(obj, "email") or ""
            local sis = object_field(obj, "sis_user_id") or ""
            if emails_equal(login_id, email) or emails_equal(em, email)
                or emails_equal(sis, email)
                or (sis_subject and sis_subject ~= "" and ids_equal(sis, sis_subject)) then
                add(id)
            end
        end
    end
    return exact
end

local function canvas_get_user_sis(sis_id)
    if not sis_id or sis_id == "" then
        return nil, "not_found"
    end
    local url = BASE .. "/api/v1/users/sis_user_id:" .. url_encode(sis_id)
    local res, err = http_get(url)
    if not res then
        return nil, "canvas_sis_failed:" .. tostring(err)
    end
    if res.status == 404 then
        return nil, "not_found"
    end
    if res.status < 200 or res.status >= 300 then
        return nil, "canvas_sis_http_" .. tostring(res.status)
    end
    local id = object_id(res.body or "")
    if not id then
        return nil, "incomplete"
    end
    return id, nil
end

local function canvas_user_matches(user_id, email, sis_subject)
    local url = BASE .. "/api/v1/users/" .. tostring(user_id) .. "/logins?per_page=50"
    local res = http_get(url)
    if res and res.status >= 200 and res.status < 300 then
        for _, obj in ipairs(each_object(res.body or "")) do
            local uid = object_field(obj, "unique_id") or ""
            local sis = object_field(obj, "sis_user_id") or ""
            if emails_equal(uid, email) or emails_equal(sis, email)
                or (sis_subject and sis_subject ~= "" and (
                    ids_equal(sis, sis_subject) or ids_equal(uid, sis_subject))) then
                return true
            end
        end
    end
    url = BASE .. "/api/v1/users/" .. tostring(user_id)
        .. "/communication_channels?per_page=50"
    res = http_get(url)
    if res and res.status >= 200 and res.status < 300 then
        for _, obj in ipairs(each_object(res.body or "")) do
            local addr = object_field(obj, "address") or ""
            if emails_equal(addr, email) then
                return true
            end
        end
    end
    return false
end

local function canvas_find_user(email, sis_subject)
    if not email or email == "" then
        return nil, "not_found"
    end
    local id, err = canvas_get_user_sis(email)
    if id then return id, nil end
    if err == "incomplete" then
        return nil, "incomplete"
    end
    if sis_subject and sis_subject ~= "" then
        id, err = canvas_get_user_sis(sis_subject)
        if id then return id, nil end
        if err == "incomplete" then
            return nil, "incomplete"
        end
    end
    local url = BASE .. "/api/v1/accounts/1/users?search_term=" .. url_encode(email)
        .. "&per_page=50"
    local res, herr = http_get(url)
    if not res then
        return nil, "canvas_search_failed:" .. tostring(herr)
    end
    if res.status < 200 or res.status >= 300 then
        return nil, "canvas_search_http_" .. tostring(res.status)
    end
    local exact = collect_exact_ids(res.body, email, sis_subject)
    if #exact == 1 then return exact[1], nil end
    if #exact > 1 then return nil, "email_ambiguous" end
    local probed = {}
    local seen = {}
    for _, obj in ipairs(each_object(res.body or "")) do
        local uid = object_id(obj)
        if uid and not seen[uid] then
            seen[uid] = true
            if canvas_user_matches(uid, email, sis_subject) then
                probed[#probed + 1] = uid
            end
        end
    end
    if #probed == 1 then return probed[1], nil end
    if #probed > 1 then return nil, "email_ambiguous" end
    if sis_subject and sis_subject ~= ""
        and string.lower(tostring(sis_subject)) ~= string.lower(tostring(email)) then
        url = BASE .. "/api/v1/accounts/1/users?search_term="
            .. url_encode(sis_subject) .. "&per_page=50"
        res, herr = http_get(url)
        if not res then
            return nil, "canvas_search_failed:" .. tostring(herr)
        end
        if res.status < 200 or res.status >= 300 then
            return nil, "canvas_search_http_" .. tostring(res.status)
        end
        exact = collect_exact_ids(res.body, email, sis_subject)
        if #exact == 1 then return exact[1], nil end
        if #exact > 1 then return nil, "email_ambiguous" end
        for _, obj in ipairs(each_object(res.body or "")) do
            local uid = object_id(obj)
            if uid and not seen[uid] then
                seen[uid] = true
                if canvas_user_matches(uid, email, sis_subject) then
                    probed[#probed + 1] = uid
                end
            end
        end
        if #probed == 1 then return probed[1], nil end
        if #probed > 1 then return nil, "email_ambiguous" end
    end
    return nil, "not_found"
end

local function link_lookup(account_id)
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

local function oidc_subject(account_id)
    local _sr, serr = H.query_sync([[
        SELECT subject
        FROM ${SCHEMA}account_oidc_identities
        WHERE account_id = :ACCOUNTID
        ORDER BY identity_id
    ]], { ACCOUNTID = account_id })
    if serr then
        return ""
    end
    local rows = qrows(_sr)
    if rows and rows[1] then
        return tostring(rows[1].subject or rows[1].SUBJECT or "")
    end
    return ""
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
        return nil, "not_provisioned"
    end
    local canvas_id, ferr = canvas_find_user(email, oidc_subject(account_id))
    if not canvas_id then
        H.log.info("FreeCourse: wait JIT account_id=%s reason=%s",
            tostring(account_id), tostring(ferr or "not_found"))
        return nil, "not_provisioned"
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

local helium_course_id = tonumber(course.course_id or course.COURSE_ID)

local function persist_free()
    local gerr = grant_entitlement(
        account_id, helium_course_id, canvas_course_id,
        "free", "free_renew", false)
    if gerr == "exists" then
        return
    end
    if gerr then
        H.log.warn("FreeCourse: entitlement write err: %s", tostring(gerr))
        return
    end
    try_log_event({
        account_id = account_id,
        course_id = helium_course_id,
        canvas_user_id = canvas_user_id,
        canvas_course_id = canvas_course_id,
        event_type = "enrolled",
        actor = "self",
    })
end

if already_enrolled(canvas_user_id, canvas_course_id) then
    persist_free()
    H.set_current_state("done")
    return succeed(true, canvas_user_id, canvas_course_id)
end

local enrolled, eerr = enroll_student(canvas_user_id, canvas_course_id)
if not enrolled then
    return fail(eerr or "canvas_enroll_failed", "Enrollment failed")
end

persist_free()
H.set_current_state("done")
return succeed(eerr == "already", canvas_user_id, canvas_course_id)
            ]==]
            WHERE group_name = 'Enroll'
              AND script_name = 'FreeCourse';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Restore Enroll.FreeCourse without retired check' AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Restore prior Enroll.FreeCourse
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
