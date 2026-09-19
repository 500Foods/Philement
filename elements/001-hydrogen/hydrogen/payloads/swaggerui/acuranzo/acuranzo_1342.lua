-- Migration: acuranzo_1342.lua
-- PRIORITIZE 2.1: replace Provision.EnsureCanvasUser
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-08-21 - EnsureCanvasUser find-dont-create

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1342"
cfg.GROUP_NAME = "Provision"
cfg.SCRIPT_NAME = "EnsureCanvasUser"
-- ----------------------------------------------------------------------------
-- Forward: replace Provision.EnsureCanvasUser body
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
-- Provision.EnsureCanvasUser (Phase 30/31; 1342 find-don't-create)
-- PRIORITIZE 2.1: keep Canvas OIDC JIT. Search sis_user_id / logins /
-- pending communication channels. Never create a second Canvas user.
-- Miss / ambiguous / incomplete -> retry next poll (Orchestrator 15s).
-- After accounts.created_at + 2 minutes still unlinked: one ops mail
-- (ops.canvas_user_unlinked) then keep polling.
-- After intro Canvas enroll, insert Helium user_enrollments
-- source=intro, expires_at NULL, renew_policy=free_renew when no
-- row exists for (account_id, course_id). Archive is sticky.
-- Secrets: os.getenv("CANVAS_API_KEY"), os.getenv("CANVAS_BASE_URL"),
-- os.getenv("CANVAS_PROVISION_ALERT_EMAIL").
-- Intro course: os.getenv("CANVAS_INTRO_COURSE_ID") (fallback 1001).
-- No JSON library: minimal pattern parse of Canvas JSON bodies.

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

local function pick(row, a, b)
    if not row then return nil end
    local v = row[a]
    if v == nil then v = row[b] end
    return v
end

local ALERT_EMAIL = getenv("CANVAS_PROVISION_ALERT_EMAIL", nil)

local function already_alerted(account_id)
    local _ar, aerr = H.query_sync([[
        SELECT alert_id
        FROM ${SCHEMA}account_canvas_alerts
        WHERE account_id = :ACCOUNTID
    ]], { ACCOUNTID = account_id })
    if aerr then
        H.log.warn("EnsureCanvasUser: alert lookup err account_id=%s: %s",
            tostring(account_id), tostring(aerr))
        return true
    end
    local rows = qrows(_ar)
    return rows and rows[1] ~= nil
end

local function mark_alerted(account_id, reason)
    local reason_s = tostring(reason or "not_found")
    if #reason_s > 50 then
        reason_s = reason_s:sub(1, 50)
    end
    local _, merr = H.query_sync([[
        INSERT INTO ${SCHEMA}account_canvas_alerts (
            alert_id, account_id, last_reason, alerted_at,
            valid_after, valid_until, created_id, created_at, updated_id, updated_at
        )
        SELECT
            (SELECT COALESCE(MAX(alert_id), 0) + 1 FROM ${SCHEMA}account_canvas_alerts),
            :ACCOUNTID,
            :REASON,
            NOW(),
            NULL, NULL, 0, NOW(), 0, NOW()
        WHERE NOT EXISTS (
            SELECT 1 FROM ${SCHEMA}account_canvas_alerts a
            WHERE a.account_id = :ACCOUNTID
        )
    ]], { ACCOUNTID = account_id, REASON = reason_s })
    if merr then
        H.log.warn("EnsureCanvasUser: alert mark err account_id=%s: %s",
            tostring(account_id), tostring(merr))
    end
end

local function send_unlinked_alert(account_id, email, reason)
    if already_alerted(account_id) then
        return
    end
    if not ALERT_EMAIL then
        H.log.warn("EnsureCanvasUser: CANVAS_PROVISION_ALERT_EMAIL unset; skip mail account_id=%s",
            tostring(account_id))
        mark_alerted(account_id, reason)
        return
    end
    if type(H) ~= "table" or type(H.mail) ~= "table"
        or type(H.mail.send_sync) ~= "function" then
        H.log.warn("EnsureCanvasUser: mail unavailable account_id=%s", tostring(account_id))
        return
    end
    local _, serr = H.mail.send_sync({
        template = "ops.canvas_user_unlinked",
        to = ALERT_EMAIL,
        params = {
            ACCOUNT_ID = tostring(account_id),
            EMAIL = tostring(email or ""),
            REASON = tostring(reason or "not_found"),
        },
        idempotency_key = "canvas-unlinked:" .. tostring(account_id),
    })
    if serr then
        H.log.warn("EnsureCanvasUser: alert send err account_id=%s: %s",
            tostring(account_id), tostring(serr))
        return
    end
    mark_alerted(account_id, reason)
    H.log.info("EnsureCanvasUser: alerted account_id=%s reason=%s",
        tostring(account_id), tostring(reason))
end

local function maybe_alert(account_id, email, reason, aged)
    if tonumber(aged) ~= 1 then
        return
    end
    send_unlinked_alert(account_id, email, reason)
end


local function try_log_event(opts)
    local ok, mod = pcall(require, "Enroll.LogEvent")
    if not ok or type(mod) ~= "table" or type(mod.record) ~= "function" then
        H.log.warn("EnsureCanvasUser: LogEvent unavailable: %s", tostring(mod))
        return
    end
    local r = mod.record(opts)
    if type(r) == "table" and r.ok == false then
        H.log.warn("EnsureCanvasUser: LogEvent: %s", tostring(r.code or r.message))
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


local BASE = (getenv("CANVAS_BASE_URL", "https://canvas.500courses.com")):gsub("/+$/", "")
local TOKEN = getenv("CANVAS_API_KEY", nil)
local ACCOUNT_PATH = "/api/v1/accounts/1"
local INTRO_COURSE_ID = getenv("CANVAS_INTRO_COURSE_ID", "1001")

local function auth_headers()
    return {
        Authorization = "Bearer " .. TOKEN,
        Accept = "application/json",
    }
end

local function http_get(url)
    local res, err = H.http.get_sync(url, auth_headers(), { timeout = 30 })
    if err then return nil, err end
    return res, nil
end

local function http_post_json(url, body)
    local headers = auth_headers()
    headers["Content-Type"] = "application/json"
    local res, err = H.http.post_sync(url, body, headers, {
        timeout = 30,
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

local function enroll_intro_course(canvas_user_id, email)
    local course_id = tonumber(INTRO_COURSE_ID)
    if not course_id then
        H.log.warn("EnsureCanvasUser: INTRO_COURSE_ID '%s' not numeric; skip enroll",
            tostring(INTRO_COURSE_ID))
        return false
    end
    local body = string.format(
        '{"enrollment":{"user_id":%d,"type":"StudentEnrollment","enrollment_state":"active","notify":false}}',
        canvas_user_id
    )
    local url = BASE .. "/api/v1/courses/" .. tostring(course_id) .. "/enrollments"
    local res, err = http_post_json(url, body)
    if not res then
        H.log.warn("EnsureCanvasUser: enroll failed account email=%s: %s",
            tostring(email), tostring(err))
        return false
    end
    if res.status >= 200 and res.status < 300 then
        H.log.info("EnsureCanvasUser: enrolled canvas_user_id=%s into course %s",
            tostring(canvas_user_id), tostring(course_id))
        return true
    end
    H.log.info("EnsureCanvasUser: enroll HTTP %s (likely already enrolled) email=%s course=%s",
        tostring(res.status), tostring(email), tostring(course_id))
    return false
end


local function catalog_by_canvas(canvas_course_id)
    local _cr, cerr = H.query_sync([[
        SELECT course_id, pricing_type
        FROM ${SCHEMA}courses
        WHERE canvas_course_id = :CID
    ]], { CID = canvas_course_id })
    if cerr then return nil, cerr end
    local rows = qrows(_cr)
    if rows and rows[1] then return rows[1], nil end
    return nil, nil
end

local function grant_intro_entitlement(account_id, canvas_course_id, canvas_user_id)
    local cid = tonumber(canvas_course_id)
    if not account_id or not cid then return end
    local course, cerr = catalog_by_canvas(cid)
    if cerr then
        H.log.warn("EnsureCanvasUser: intro catalog err: %s", tostring(cerr))
        return
    end
    if not course then
        H.log.warn("EnsureCanvasUser: intro course %s not in catalog", tostring(cid))
        return
    end
    local helium_id = tonumber(course.course_id or course.COURSE_ID)
    local gerr = grant_entitlement(
        account_id, helium_id, cid, "intro", "free_renew", true)
    if gerr == "exists" then
        return
    end
    if gerr then
        H.log.warn("EnsureCanvasUser: intro entitlement err: %s", tostring(gerr))
        return
    end
    try_log_event({
        account_id = account_id,
        course_id = helium_id,
        canvas_user_id = canvas_user_id,
        canvas_course_id = cid,
        event_type = "enrolled",
        actor = "system",
    })
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
    local _qr, err = H.query_sync([[
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
        RETURNING link_id
    ]], {
        ACCOUNTID = account_id,
        CANVASUSERID = canvas_user_id,
        CANVASEMAIL = canvas_email,
    })
    if err then return nil, err end
    local rows = qrows(_qr)
    if rows and rows[1] then return rows[1].link_id or rows[1].LINK_ID, nil end
    return nil, nil
end

local function link_touch(link_id, canvas_email)
    local _, err = H.query_sync([[
        UPDATE ${SCHEMA}account_canvas_links
        SET last_seen_at = NOW(),
            canvas_email = :CANVASEMAIL,
            updated_at = NOW()
        WHERE link_id = :LINKID
    ]], { LINKID = link_id, CANVASEMAIL = canvas_email })
    if err then
        H.log.warn("EnsureCanvasUser: touch failed link_id=%s err=%s",
            tostring(link_id), tostring(err))
    end
end

local function ensure_one(account_id, email, display_name, sis_subject, aged)
    local existing, lerr = link_lookup(account_id)
    if lerr then
        H.log.warn("EnsureCanvasUser: lookup err account_id=%s: %s",
            tostring(account_id), tostring(lerr))
        return false
    end
    local canvas_user_id
    if existing then
        local lid = existing.link_id or existing.LINK_ID
        link_touch(lid, email)
        canvas_user_id = existing.canvas_user_id or existing.CANVAS_USER_ID
        H.log.info("EnsureCanvasUser: already linked account_id=%s canvas_user_id=%s",
            tostring(account_id), tostring(canvas_user_id))
    else
        local canvas_id, ferr = canvas_find_user(email, sis_subject)
        if not canvas_id then
            local reason = ferr or "not_found"
            H.log.info("EnsureCanvasUser: wait JIT account_id=%s reason=%s",
                tostring(account_id), tostring(reason))
            maybe_alert(account_id, email, reason, aged)
            return false
        end
        H.log.info("EnsureCanvasUser: matched canvas_user_id=%s for account_id=%s",
            tostring(canvas_id), tostring(account_id))
        local link_id, ierr = link_insert(account_id, canvas_id, email)
        if ierr then
            H.log.warn("EnsureCanvasUser: link insert err account_id=%s: %s",
                tostring(account_id), tostring(ierr))
            return false
        end
        if not link_id then
            existing = select(1, link_lookup(account_id))
            if existing then
                link_touch(existing.link_id or existing.LINK_ID, email)
                canvas_id = existing.canvas_user_id or existing.CANVAS_USER_ID
            else
                H.log.warn("EnsureCanvasUser: link insert returned 0 rows account_id=%s",
                    tostring(account_id))
                return false
            end
        end
        canvas_user_id = canvas_id
    end

    if canvas_user_id then
        local ok, eerr = pcall(enroll_intro_course, canvas_user_id, email)
        if not ok then
            H.log.warn("EnsureCanvasUser: enroll pcall err email=%s: %s",
                tostring(email), tostring(eerr))
        end
        grant_intro_entitlement(account_id, INTRO_COURSE_ID, canvas_user_id)
    end
    return true
end

H.set_current_state("start")
if not TOKEN or TOKEN == "" then
    H.log.error("EnsureCanvasUser: CANVAS_API_KEY not set in environment")
    return
end

H.log.info("EnsureCanvasUser: intro course target=%s", tostring(INTRO_COURSE_ID))

local _qr, err = H.query_sync([[
    SELECT a.account_id,
           oi.email,
           oi.subject,
           a.name,
           a.first_name,
           a.last_name,
           CASE WHEN a.created_at <= NOW() - INTERVAL '2 minutes' THEN 1 ELSE 0 END AS aged
    FROM ${SCHEMA}account_oidc_identities oi
    JOIN ${SCHEMA}accounts a ON a.account_id = oi.account_id
    WHERE oi.email IS NOT NULL
      AND oi.email <> ''
      AND NOT EXISTS (
          SELECT 1 FROM ${SCHEMA}account_canvas_links acl
          WHERE acl.account_id = a.account_id
      )
    ORDER BY a.account_id
    LIMIT 50
]])
if err then
    H.log.error("EnsureCanvasUser: candidate query failed: %s", tostring(err))
    return
end
local rows = qrows(_qr)
if not rows or #rows == 0 then
    H.log.info("EnsureCanvasUser: no unlinked OIDC accounts")
    return
end

H.log.info("EnsureCanvasUser: %d candidate(s)", #rows)
local ok_n, fail_n = 0, 0
for _, row in ipairs(rows) do
    local account_id = row.account_id or row.ACCOUNT_ID
    local email = row.email or row.EMAIL
    local sis_subject = row.subject or row.SUBJECT or ""
    local aged = row.aged or row.AGED
    local name = row.name or row.NAME
    local fn = row.first_name or row.FIRST_NAME or ""
    local ln = row.last_name or row.LAST_NAME or ""
    local display = name
    if (not display or display == "") and (fn ~= "" or ln ~= "") then
        display = (fn .. " " .. ln):gsub("^%s+", ""):gsub("%s+$", "")
    end
    if ensure_one(account_id, email, display, sis_subject, aged) then
        ok_n = ok_n + 1
    else
        fail_n = fail_n + 1
    end
end
H.log.info("EnsureCanvasUser: done ok=%d fail=%d", ok_n, fail_n)
H.set_current_state("done")
            ]==]
            WHERE group_name = 'Provision'
              AND script_name = 'EnsureCanvasUser';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'EnsureCanvasUser find-dont-create'                                                       AS name,
        [=[
            # Forward Migration ${MIGRATION}: EnsureCanvasUser find-dont-create

            PRIORITIZE 2.1. Do not restamp 1331. Search-dont-create; 2m ops alert; keep polling.
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
-- Provision.EnsureCanvasUser (Phase 30/31; 1322 intro user_enrollments)
-- After intro Canvas enroll, insert Helium user_enrollments
-- source=intro, expires_at NULL, renew_policy=free_renew when no
-- row exists for (account_id, course_id). Archive is sticky.
-- Poll accounts with an OIDC identity but no account_canvas_links row.
-- For each: Canvas email search -> create if missing -> #145 link / #146 touch
--           -> enroll into the intro course (Phase 31, best-effort).
-- Secrets: os.getenv("CANVAS_API_KEY"), os.getenv("CANVAS_BASE_URL").
-- Intro course: os.getenv("CANVAS_INTRO_COURSE_ID") (fallback 1001 = 5C-001-W5C-EN).
-- No JSON library: minimal pattern parse of Canvas JSON bodies.

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

local function pick(row, a, b)
    if not row then return nil end
    local v = row[a]
    if v == nil then v = row[b] end
    return v
end

local function try_log_event(opts)
    local ok, mod = pcall(require, "Enroll.LogEvent")
    if not ok or type(mod) ~= "table" or type(mod.record) ~= "function" then
        H.log.warn("EnsureCanvasUser: LogEvent unavailable: %s", tostring(mod))
        return
    end
    local r = mod.record(opts)
    if type(r) == "table" and r.ok == false then
        H.log.warn("EnsureCanvasUser: LogEvent: %s", tostring(r.code or r.message))
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


local BASE = (getenv("CANVAS_BASE_URL", "https://canvas.500courses.com")):gsub("/+$/", "")
local TOKEN = getenv("CANVAS_API_KEY", nil)
local ACCOUNT_PATH = "/api/v1/accounts/1"
local INTRO_COURSE_ID = getenv("CANVAS_INTRO_COURSE_ID", "1001")

local function auth_headers()
    return {
        Authorization = "Bearer " .. TOKEN,
        Accept = "application/json",
    }
end

local function http_get(url)
    local res, err = H.http.get_sync(url, auth_headers(), { timeout = 30 })
    if err then return nil, err end
    return res, nil
end

local function http_post_json(url, body)
    local headers = auth_headers()
    headers["Content-Type"] = "application/json"
    local res, err = H.http.post_sync(url, body, headers, {
        timeout = 30,
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
    local url = BASE .. "/api/v1/accounts/1/users?search_term=" .. url_encode(email)
        .. "&per_page=50"
    local res, err = http_get(url)
    if not res then
        return nil, "canvas_search_failed:" .. tostring(err)
    end
    if res.status < 200 or res.status >= 300 then
        return nil, "canvas_search_http_" .. tostring(res.status)
    end
    local matches = {}
    for _, obj in ipairs(each_object(res.body or "")) do
        local id = object_id(obj)
        if id then
            local login_id = object_field(obj, "login_id") or ""
            local em = object_field(obj, "email") or ""
            local sortable = object_field(obj, "sortable_name") or ""
            if emails_equal(login_id, email) or emails_equal(em, email) then
                matches[#matches + 1] = id
            elseif #matches == 0 and email ~= "" and (
                string.find(string.lower(login_id), string.lower(email), 1, true)
                or string.find(string.lower(em), string.lower(email), 1, true)
                or string.find(string.lower(sortable), string.lower(email), 1, true)
            ) then
                matches[#matches + 1] = id
            end
        end
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
    if #matches == 1 then return matches[1], nil end
    if #matches > 1 then return nil, "email_ambiguous" end
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
        H.log.warn("EnsureCanvasUser: create HTTP %s body=%s",
            tostring(res.status), string.sub(tostring(res.body or ""), 1, 300))
        return nil, "canvas_create_http_" .. tostring(res.status)
    end
    local id = object_id(res.body or "")
    if not id then
        return nil, "canvas_create_no_id"
    end
    return id, nil
end

local function enroll_intro_course(canvas_user_id, email)
    local course_id = tonumber(INTRO_COURSE_ID)
    if not course_id then
        H.log.warn("EnsureCanvasUser: INTRO_COURSE_ID '%s' not numeric; skip enroll",
            tostring(INTRO_COURSE_ID))
        return false
    end
    local body = string.format(
        '{"enrollment":{"user_id":%d,"type":"StudentEnrollment","enrollment_state":"active","notify":false}}',
        canvas_user_id
    )
    local url = BASE .. "/api/v1/courses/" .. tostring(course_id) .. "/enrollments"
    local res, err = http_post_json(url, body)
    if not res then
        H.log.warn("EnsureCanvasUser: enroll failed account email=%s: %s",
            tostring(email), tostring(err))
        return false
    end
    if res.status >= 200 and res.status < 300 then
        H.log.info("EnsureCanvasUser: enrolled canvas_user_id=%s into course %s",
            tostring(canvas_user_id), tostring(course_id))
        return true
    end
    H.log.info("EnsureCanvasUser: enroll HTTP %s (likely already enrolled) email=%s course=%s",
        tostring(res.status), tostring(email), tostring(course_id))
    return false
end


local function catalog_by_canvas(canvas_course_id)
    local _cr, cerr = H.query_sync([[
        SELECT course_id, pricing_type
        FROM ${SCHEMA}courses
        WHERE canvas_course_id = :CID
    ]], { CID = canvas_course_id })
    if cerr then return nil, cerr end
    local rows = qrows(_cr)
    if rows and rows[1] then return rows[1], nil end
    return nil, nil
end

local function grant_intro_entitlement(account_id, canvas_course_id, canvas_user_id)
    local cid = tonumber(canvas_course_id)
    if not account_id or not cid then return end
    local course, cerr = catalog_by_canvas(cid)
    if cerr then
        H.log.warn("EnsureCanvasUser: intro catalog err: %s", tostring(cerr))
        return
    end
    if not course then
        H.log.warn("EnsureCanvasUser: intro course %s not in catalog", tostring(cid))
        return
    end
    local helium_id = tonumber(course.course_id or course.COURSE_ID)
    local gerr = grant_entitlement(
        account_id, helium_id, cid, "intro", "free_renew", true)
    if gerr == "exists" then
        return
    end
    if gerr then
        H.log.warn("EnsureCanvasUser: intro entitlement err: %s", tostring(gerr))
        return
    end
    try_log_event({
        account_id = account_id,
        course_id = helium_id,
        canvas_user_id = canvas_user_id,
        canvas_course_id = cid,
        event_type = "enrolled",
        actor = "system",
    })
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
    local _qr, err = H.query_sync([[
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
        RETURNING link_id
    ]], {
        ACCOUNTID = account_id,
        CANVASUSERID = canvas_user_id,
        CANVASEMAIL = canvas_email,
    })
    if err then return nil, err end
    local rows = qrows(_qr)
    if rows and rows[1] then return rows[1].link_id or rows[1].LINK_ID, nil end
    return nil, nil
end

local function link_touch(link_id, canvas_email)
    local _, err = H.query_sync([[
        UPDATE ${SCHEMA}account_canvas_links
        SET last_seen_at = NOW(),
            canvas_email = :CANVASEMAIL,
            updated_at = NOW()
        WHERE link_id = :LINKID
    ]], { LINKID = link_id, CANVASEMAIL = canvas_email })
    if err then
        H.log.warn("EnsureCanvasUser: touch failed link_id=%s err=%s",
            tostring(link_id), tostring(err))
    end
end

local function ensure_one(account_id, email, display_name)
    local existing, lerr = link_lookup(account_id)
    if lerr then
        H.log.warn("EnsureCanvasUser: lookup err account_id=%s: %s",
            tostring(account_id), tostring(lerr))
        return false
    end
    local canvas_user_id
    if existing then
        local lid = existing.link_id or existing.LINK_ID
        link_touch(lid, email)
        canvas_user_id = existing.canvas_user_id or existing.CANVAS_USER_ID
        H.log.info("EnsureCanvasUser: already linked account_id=%s canvas_user_id=%s",
            tostring(account_id), tostring(canvas_user_id))
    else
        local canvas_id, ferr = canvas_find_by_email(email)
        if ferr == "email_ambiguous" then
            H.log.error("EnsureCanvasUser: email_ambiguous account_id=%s email=%s",
                tostring(account_id), tostring(email))
            return false
        end
        if ferr and ferr ~= "not_found" then
            H.log.warn("EnsureCanvasUser: search err account_id=%s: %s",
                tostring(account_id), tostring(ferr))
            return false
        end
        if not canvas_id then
            canvas_id, ferr = canvas_create_user(email, display_name)
            if not canvas_id then
                H.log.warn("EnsureCanvasUser: create failed account_id=%s: %s",
                    tostring(account_id), tostring(ferr))
                return false
            end
            H.log.info("EnsureCanvasUser: created canvas_user_id=%s for account_id=%s",
                tostring(canvas_id), tostring(account_id))
        else
            H.log.info("EnsureCanvasUser: matched canvas_user_id=%s for account_id=%s",
                tostring(canvas_id), tostring(account_id))
        end

        local link_id, ierr = link_insert(account_id, canvas_id, email)
        if ierr then
            H.log.warn("EnsureCanvasUser: link insert err account_id=%s: %s",
                tostring(account_id), tostring(ierr))
            return false
        end
        if not link_id then
            existing = select(1, link_lookup(account_id))
            if existing then
                link_touch(existing.link_id or existing.LINK_ID, email)
                canvas_id = existing.canvas_user_id or existing.CANVAS_USER_ID
            else
                H.log.warn("EnsureCanvasUser: link insert returned 0 rows account_id=%s",
                    tostring(account_id))
                return false
            end
        end
        canvas_user_id = canvas_id
    end

    if canvas_user_id then
        local ok, eerr = pcall(enroll_intro_course, canvas_user_id, email)
        if not ok then
            H.log.warn("EnsureCanvasUser: enroll pcall err email=%s: %s",
                tostring(email), tostring(eerr))
        end
        grant_intro_entitlement(account_id, INTRO_COURSE_ID, canvas_user_id)
    end
    return true
end

H.set_current_state("start")
if not TOKEN or TOKEN == "" then
    H.log.error("EnsureCanvasUser: CANVAS_API_KEY not set in environment")
    return
end

H.log.info("EnsureCanvasUser: intro course target=%s", tostring(INTRO_COURSE_ID))

local _qr, err = H.query_sync([[
    SELECT a.account_id,
           oi.email,
           a.name,
           a.first_name,
           a.last_name
    FROM ${SCHEMA}account_oidc_identities oi
    JOIN ${SCHEMA}accounts a ON a.account_id = oi.account_id
    WHERE oi.email IS NOT NULL
      AND oi.email <> ''
      AND NOT EXISTS (
          SELECT 1 FROM ${SCHEMA}account_canvas_links acl
          WHERE acl.account_id = a.account_id
      )
    ORDER BY a.account_id
    LIMIT 50
]])
if err then
    H.log.error("EnsureCanvasUser: candidate query failed: %s", tostring(err))
    return
end
local rows = qrows(_qr)
if not rows or #rows == 0 then
    H.log.info("EnsureCanvasUser: no unlinked OIDC accounts")
    return
end

H.log.info("EnsureCanvasUser: %d candidate(s)", #rows)
local ok_n, fail_n = 0, 0
for _, row in ipairs(rows) do
    local account_id = row.account_id or row.ACCOUNT_ID
    local email = row.email or row.EMAIL
    local name = row.name or row.NAME
    local fn = row.first_name or row.FIRST_NAME or ""
    local ln = row.last_name or row.LAST_NAME or ""
    local display = name
    if (not display or display == "") and (fn ~= "" or ln ~= "") then
        display = (fn .. " " .. ln):gsub("^%s+", ""):gsub("%s+$", "")
    end
    if ensure_one(account_id, email, display) then
        ok_n = ok_n + 1
    else
        fail_n = fail_n + 1
    end
end
H.log.info("EnsureCanvasUser: done ok=%d fail=%d", ok_n, fail_n)
H.set_current_state("done")
            ]==]
            WHERE group_name = 'Provision'
              AND script_name = 'EnsureCanvasUser';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Restore prior Provision.EnsureCanvasUser body'                               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: restore prior EnsureCanvasUser

            Restores the pre-2.1 Provision.EnsureCanvasUser body.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
