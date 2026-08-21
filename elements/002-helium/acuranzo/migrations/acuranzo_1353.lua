-- Migration: acuranzo_1353.lua
-- PRIORITIZE 2.1 leftover: Enroll.PaidCourse Helium-first, find-don't-create
--
-- Do not restamp 1352.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-08-21 - PaidCourse never creates Canvas users (PRIORITIZE 2.1)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1353"
cfg.GROUP_NAME = "Enroll"
cfg.SCRIPT_NAME = "PaidCourse"
-- ----------------------------------------------------------------------------
-- Forward: replace Enroll.PaidCourse body
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
-- Enroll.PaidCourse (STRIPE_PLAN Phase 7/8; 1353 Helium-first)
-- Module. require("Enroll.PaidCourse").fulfill(opts)
-- Not JWT-invokable. Webhook C HMAC is the lock.
-- Never call Enroll.FreeCourse / Enroll.RenewFree.
-- PRIORITIZE 2.1 leftover: find-don't-create. Helium entitlement
-- is SoT; Canvas enroll is best-effort. Never mint a Canvas user.
-- Unlinked / Canvas HTTP miss: still INSERT user_enrollments and
-- complete the order. EnsureCanvasUser (1354) stamps
-- canvas_enrollment_id once JIT exists.

local M = {}

local LOG_TAG = "PaidCourse"
local function try_log_event(opts)
    local ok, mod = pcall(require, "Enroll.LogEvent")
    if not ok or type(mod) ~= "table" or type(mod.record) ~= "function" then
        H.log.warn("%s: LogEvent unavailable: %s", LOG_TAG, tostring(mod))
        return
    end
    local r = mod.record(opts)
    if type(r) == "table" and r.ok == false then
        H.log.warn("%s: LogEvent: %s", LOG_TAG, tostring(r.code or r.message))
    end
end


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
    if v == nil and b then v = row[b] end
    return v
end

local function err(code, message)
    return { ok = false, code = code, message = message or code }
end

local function parse_items(json)
    local items = {}
    if type(json) ~= "string" or json == "" then return items end
    for obj in json:gmatch("%b{}") do
        local course_id = tonumber(obj:match('"courseId"%s*:%s*(%d+)'))
        if course_id then
            local canvas = tonumber(obj:match('"canvasCourseId"%s*:%s*(%d+)'))
            local line = obj:match('"lineType"%s*:%s*"([^"]*)"') or "purchase"
            items[#items + 1] = {
                courseId = course_id,
                canvasCourseId = canvas,
                lineType = line,
            }
        end
    end
    return items
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
    local res, herr = H.http.get_sync(url, auth_headers(), { timeout = 10 })
    if herr then return nil, herr end
    return res, nil
end

local function http_post_json(url, body)
    local headers = auth_headers()
    headers["Content-Type"] = "application/json"
    local res, herr = H.http.post_sync(url, body, headers, {
        timeout = 10,
        content_type = "application/json",
    })
    if herr then return nil, herr end
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
    local _qr, qerr = H.query_sync([[
        SELECT link_id, account_id, canvas_user_id, canvas_email, last_seen_at
        FROM ${SCHEMA}account_canvas_links
        WHERE account_id = :ACCOUNTID
    ]], { ACCOUNTID = account_id })
    if qerr then return nil, qerr end
    local rows = qrows(_qr)
    if rows and rows[1] then return rows[1], nil end
    return nil, nil
end

local function link_insert(account_id, canvas_user_id, canvas_email)
    local _, qerr = H.query_sync([[
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
    if qerr then return nil, qerr end
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
        H.log.info("PaidCourse: wait JIT account_id=%s reason=%s",
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
        return nil
    end
    for _, obj in ipairs(each_object(res.body or "")) do
        local id = object_id(obj)
        if id then
            return id
        end
    end
    return nil
end

local function stamp_canvas_enrollment(enrollment_id, canvas_enrollment_id)
    if not enrollment_id or not canvas_enrollment_id then
        return
    end
    local _, serr = H.query_sync([[
        UPDATE ${SCHEMA}user_enrollments
           SET canvas_enrollment_id = :CEID,
               updated_at = NOW(),
               updated_id = 0
         WHERE enrollment_id = :EID
           AND canvas_enrollment_id IS NULL
    ]], { CEID = canvas_enrollment_id, EID = enrollment_id })
    if serr then
        H.log.warn("PaidCourse: stamp canvas_enrollment_id err enrollment_id=%s: %s",
            tostring(enrollment_id), tostring(serr))
    end
end

local function enroll_student(canvas_user_id, course_id)
    local body = string.format(
        '{"enrollment":{"user_id":%d,"type":"StudentEnrollment","enrollment_state":"active","notify":false}}',
        canvas_user_id
    )
    local url = BASE .. "/api/v1/courses/" .. tostring(course_id) .. "/enrollments"
    local delays = { 200, 800, 2000 }
    local last = "canvas_enroll_failed"
    for attempt = 1, 3 do
        local res, herr = http_post_json(url, body)
        if not res then
            last = "canvas_enroll_failed:" .. tostring(herr)
        elseif res.status >= 200 and res.status < 300 then
            return true, object_id(res.body or ""), nil
        else
            local snippet = string.lower(string.sub(tostring(res.body or ""), 1, 400))
            if string.find(snippet, "already", 1, true) then
                return true, nil, "already"
            end
            last = "canvas_enroll_http_" .. tostring(res.status)
            H.log.warn("PaidCourse: enroll HTTP %s attempt=%s body=%s",
                tostring(res.status), tostring(attempt),
                string.sub(tostring(res.body or ""), 1, 300))
            if res.status < 500 and res.status ~= 429 then
                return false, nil, last
            end
        end
        if attempt < 3 then
            H.sleep(delays[attempt])
        end
    end
    return false, nil, last
end

local function project_canvas_seat(canvas_user_id, canvas_course, enrollment_id)
    if not canvas_user_id or not canvas_course then
        return false
    end
    if not TOKEN or TOKEN == "" then
        H.log.error("PaidCourse: CANVAS_API_KEY not set; defer LMS enroll enrollment_id=%s",
            tostring(enrollment_id))
        return false
    end
    local existing_eid = already_enrolled(canvas_user_id, canvas_course)
    if existing_eid then
        stamp_canvas_enrollment(enrollment_id, existing_eid)
        return true
    end
    local ok, ceid, eerr = enroll_student(canvas_user_id, canvas_course)
    if ok then
        if not ceid then
            ceid = already_enrolled(canvas_user_id, canvas_course)
        end
        stamp_canvas_enrollment(enrollment_id, ceid)
        return true
    end
    H.log.warn("PaidCourse: canvas project deferred enrollment_id=%s err=%s",
        tostring(enrollment_id), tostring(eerr))
    return false
end

local function load_order(order_id, stripe_intent_id)
    local sql
    local bind
    if stripe_intent_id and stripe_intent_id ~= "" then
        sql = [[
            SELECT order_id, account_id, stripe_intent_id, status,
                   currency, total_cents, items_json
            FROM ${SCHEMA}orders
            WHERE stripe_intent_id = :INTENT
        ]]
        bind = { INTENT = stripe_intent_id }
    else
        sql = [[
            SELECT order_id, account_id, stripe_intent_id, status,
                   currency, total_cents, items_json
            FROM ${SCHEMA}orders
            WHERE order_id = :ORDERID
        ]]
        bind = { ORDERID = order_id }
    end
    local _or, qerr = H.query_sync(sql, bind)
    if qerr then return nil, qerr end
    local rows = qrows(_or)
    if not rows or not rows[1] then return nil, nil end
    return rows[1], nil
end

local function entitlement_for_order_course(order_id, course_id)
    local _er, qerr = H.query_sync([[
        SELECT enrollment_id
        FROM ${SCHEMA}user_enrollments
        WHERE order_id = :ORDERID
          AND course_id = :COURSEID
        ORDER BY enrollment_id DESC
    ]], { ORDERID = order_id, COURSEID = course_id })
    if qerr then return nil, qerr end
    local rows = qrows(_er)
    if rows and rows[1] then
        return tonumber(pick(rows[1], "enrollment_id", "ENROLLMENT_ID")), nil
    end
    return nil, nil
end

local function next_enrollment_id()
    local _nr = H.query_sync([[
        SELECT COALESCE(MAX(enrollment_id), 0) + 1 AS next_id
        FROM ${SCHEMA}user_enrollments
    ]], {})
    local rows = qrows(_nr)
    return tonumber(pick(rows and rows[1], "next_id", "NEXT_ID")) or 1
end

local function grant_line(account_id, order_id, line, canvas_user_id, amount_cents, currency)
    local course_id = line.courseId
    local _cr, cerr = H.query_sync([[
        SELECT course_id, pricing_type, published, canvas_course_id, retired
        FROM ${SCHEMA}courses
        WHERE course_id = :COURSEID
          AND published = 1
    ]], { COURSEID = course_id })
    if cerr then
        return nil, "catalog_error"
    end
    local rows = qrows(_cr)
    if not rows or not rows[1] then
        return nil, "course_not_found"
    end
    local course = rows[1]
    local retired = tonumber(pick(course, "retired", "RETIRED")) or 0
    if retired ~= 0 then
        return nil, "not_offered"
    end
    local pricing = string.lower(tostring(pick(course, "pricing_type", "PRICING_TYPE") or ""))
    if pricing ~= "paid" then
        return nil, "not_paid"
    end
    local canvas_course = line.canvasCourseId
        or tonumber(pick(course, "canvas_course_id", "CANVAS_COURSE_ID"))
    if not canvas_course then
        return nil, "no_canvas_course"
    end

    local existing, eerr = entitlement_for_order_course(order_id, course_id)
    if eerr then
        return nil, "entitlement_lookup_failed"
    end
    if existing then
        project_canvas_seat(canvas_user_id, canvas_course, existing)
        return existing, nil
    end

    local source = "paid"
    if line.lineType == "renew" then
        source = "renew"
    end

    local _, serr = H.query_sync([[
        UPDATE ${SCHEMA}user_enrollments
           SET status = 'superseded',
               updated_at = NOW(),
               updated_id = 0
         WHERE account_id = :ACCOUNTID
           AND course_id = :COURSEID
           AND status IN ('active', 'completed', 'pending')
    ]], { ACCOUNTID = account_id, COURSEID = course_id })
    if serr then
        H.log.warn("PaidCourse: supersede err: %s", tostring(serr))
        return nil, "entitlement_error"
    end

    local eid = next_enrollment_id()
    local _, ierr = H.query_sync([[
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
            'active', NOW(), NOW() + INTERVAL '90 days',
            NULL, 0, NULL,
            NULL, 'paid_renew', :SOURCE, :ORDERID,
            NULL, NULL, 0, NOW(), 0, NOW()
        )
    ]], {
        EID = eid,
        ACCOUNTID = account_id,
        COURSEID = course_id,
        CANVASCOURSE = canvas_course,
        SOURCE = source,
        ORDERID = order_id,
    })
    if ierr then
        local again = select(1, entitlement_for_order_course(order_id, course_id))
        if again then
            project_canvas_seat(canvas_user_id, canvas_course, again)
            return again, nil
        end
        H.log.warn("PaidCourse: entitlement insert err: %s", tostring(ierr))
        return nil, "entitlement_error"
    end
    local etype = "purchased"
    if source == "renew" then
        etype = "renewed"
    end
    try_log_event({
        account_id = account_id,
        course_id = course_id,
        canvas_user_id = canvas_user_id,
        canvas_course_id = canvas_course,
        enrollment_id = eid,
        event_type = etype,
        actor = "system",
        amount_cents = amount_cents,
        currency = currency,
        order_id = order_id,
    })
    project_canvas_seat(canvas_user_id, canvas_course, eid)
    return eid, nil
end

local function complete_order(order_id)
    local _, uerr = H.query_sync([[
        UPDATE ${SCHEMA}orders
           SET status = 'completed',
               updated_at = NOW(),
               updated_id = 0
         WHERE order_id = :ORDERID
           AND status = 'pending'
    ]], { ORDERID = order_id })
    return uerr
end

function M.fulfill(opts)
    if type(opts) ~= "table" then
        return err("validation", "Missing fulfill opts")
    end
    local stripe_intent_id = opts.stripe_intent_id
    local order_id = tonumber(opts.order_id)
    if (not stripe_intent_id or stripe_intent_id == "") and not order_id then
        return err("validation", "Missing payment intent")
    end

    local order, oerr = load_order(order_id, stripe_intent_id)
    if oerr then
        H.log.warn("PaidCourse: order lookup err: %s", tostring(oerr))
        return err("order_error", "Could not load order")
    end
    if not order then
        return err("order_not_found", "No matching order")
    end

    order_id = tonumber(pick(order, "order_id", "ORDER_ID"))
    local status = tostring(pick(order, "status", "STATUS") or "")
    if status == "completed" then
        return {
            ok = true,
            already = true,
            order_id = order_id,
            stripe_intent_id = pick(order, "stripe_intent_id", "STRIPE_INTENT_ID"),
        }
    end
    if status ~= "pending" then
        return err("order_not_pending", "Order is not pending")
    end

    local want_cents = tonumber(opts.amount_cents)
    local want_cur = tostring(opts.currency or ""):lower()
    local got_cents = tonumber(pick(order, "total_cents", "TOTAL_CENTS"))
    local got_cur = tostring(pick(order, "currency", "CURRENCY") or ""):lower()
    if want_cents and got_cents and want_cents ~= got_cents then
        H.log.warn("PaidCourse: amount mismatch order_id=%s", tostring(order_id))
        return err("amount_mismatch", "Payment amount does not match order")
    end
    if want_cur ~= "" and got_cur ~= "" and want_cur ~= got_cur then
        H.log.warn("PaidCourse: currency mismatch order_id=%s", tostring(order_id))
        return err("amount_mismatch", "Payment currency does not match order")
    end

    local account_id = tonumber(pick(order, "account_id", "ACCOUNT_ID"))
    if not account_id then
        return err("order_error", "Order has no account")
    end

    local _ar, aerr = H.query_sync([[
        SELECT account_id, name
        FROM ${SCHEMA}accounts
        WHERE account_id = :ACCOUNTID
    ]], { ACCOUNTID = account_id })
    if aerr or not qrows(_ar) or not qrows(_ar)[1] then
        return err("account_not_found", "Account not found")
    end
    local acct = qrows(_ar)[1]
    local _or, ierr = H.query_sync([[
        SELECT email
        FROM ${SCHEMA}account_oidc_identities
        WHERE account_id = :ACCOUNTID
        ORDER BY last_seen_at DESC
    ]], { ACCOUNTID = account_id })
    if ierr then
        H.log.warn("PaidCourse: oidc lookup err: %s", tostring(ierr))
    end
    local oidc = qrows(_or) and qrows(_or)[1]
    local email = pick(oidc, "email", "EMAIL")
    local display = pick(acct, "name", "NAME") or email

    local canvas_user_id, lerr = ensure_link(account_id, email, display)
    if not canvas_user_id then
        H.log.info("PaidCourse: canvas pending account_id=%s order_id=%s reason=%s",
            tostring(account_id), tostring(order_id), tostring(lerr or "not_provisioned"))
    end

    local items = parse_items(pick(order, "items_json", "ITEMS_JSON"))
    if #items == 0 then
        return err("order_error", "Order has no lines")
    end

    local granted = {}
    for i = 1, #items do
        local eid, gerr = grant_line(account_id, order_id, items[i], canvas_user_id, got_cents, got_cur)
        if not eid then
            return err(gerr or "fulfill_failed", "Could not grant paid enrollment")
        end
        granted[#granted + 1] = eid
    end

    local uerr = complete_order(order_id)
    if uerr then
        H.log.warn("PaidCourse: complete order err: %s", tostring(uerr))
        return err("order_error", "Could not complete order")
    end

    return {
        ok = true,
        already = false,
        order_id = order_id,
        stripe_intent_id = pick(order, "stripe_intent_id", "STRIPE_INTENT_ID"),
        enrollment_ids = granted,
        canvas_pending = canvas_user_id == nil,
    }
end

return M
                ]==]
            WHERE group_name = 'Enroll'
              AND script_name = 'PaidCourse';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'PaidCourse Helium-first find-dont-create' AS name,
        [=[
            # Forward Migration ${MIGRATION}: PaidCourse Helium-first find-dont-create (do not restamp 1352)
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
-- Enroll.PaidCourse (STRIPE_PLAN Phase 7/8)
-- Module. require("Enroll.PaidCourse").fulfill(opts)
-- Not JWT-invokable. Webhook C HMAC is the lock.
-- Never call Enroll.FreeCourse / Enroll.RenewFree.

local M = {}

local LOG_TAG = "PaidCourse"
local function try_log_event(opts)
    local ok, mod = pcall(require, "Enroll.LogEvent")
    if not ok or type(mod) ~= "table" or type(mod.record) ~= "function" then
        H.log.warn("%s: LogEvent unavailable: %s", LOG_TAG, tostring(mod))
        return
    end
    local r = mod.record(opts)
    if type(r) == "table" and r.ok == false then
        H.log.warn("%s: LogEvent: %s", LOG_TAG, tostring(r.code or r.message))
    end
end


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
    if v == nil and b then v = row[b] end
    return v
end

local function err(code, message)
    return { ok = false, code = code, message = message or code }
end

local function parse_items(json)
    local items = {}
    if type(json) ~= "string" or json == "" then return items end
    for obj in json:gmatch("%b{}") do
        local course_id = tonumber(obj:match('"courseId"%s*:%s*(%d+)'))
        if course_id then
            local canvas = tonumber(obj:match('"canvasCourseId"%s*:%s*(%d+)'))
            local line = obj:match('"lineType"%s*:%s*"([^"]*)"') or "purchase"
            items[#items + 1] = {
                courseId = course_id,
                canvasCourseId = canvas,
                lineType = line,
            }
        end
    end
    return items
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
    local res, herr = H.http.get_sync(url, auth_headers(), { timeout = 10 })
    if herr then return nil, herr end
    return res, nil
end

local function http_post_json(url, body)
    local headers = auth_headers()
    headers["Content-Type"] = "application/json"
    local res, herr = H.http.post_sync(url, body, headers, {
        timeout = 10,
        content_type = "application/json",
    })
    if herr then return nil, herr end
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
    local res, herr = http_get(url)
    if not res then
        return nil, "canvas_search_failed:" .. tostring(herr)
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
    local res, herr = http_post_json(url, body)
    if not res then
        return nil, "canvas_create_failed:" .. tostring(herr)
    end
    if res.status < 200 or res.status >= 300 then
        H.log.warn("PaidCourse: create HTTP %s body=%s",
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
    local _qr, qerr = H.query_sync([[
        SELECT link_id, account_id, canvas_user_id, canvas_email, last_seen_at
        FROM ${SCHEMA}account_canvas_links
        WHERE account_id = :ACCOUNTID
    ]], { ACCOUNTID = account_id })
    if qerr then return nil, qerr end
    local rows = qrows(_qr)
    if rows and rows[1] then return rows[1], nil end
    return nil, nil
end

local function link_insert(account_id, canvas_user_id, canvas_email)
    local _, qerr = H.query_sync([[
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
    if qerr then return nil, qerr end
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
        H.log.info("PaidCourse: created canvas_user_id=%s for account_id=%s",
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
    local delays = { 200, 800, 2000 }
    local last = "canvas_enroll_failed"
    for attempt = 1, 3 do
        local res, herr = http_post_json(url, body)
        if not res then
            last = "canvas_enroll_failed:" .. tostring(herr)
        elseif res.status >= 200 and res.status < 300 then
            return true, object_id(res.body or ""), nil
        else
            local snippet = string.lower(string.sub(tostring(res.body or ""), 1, 400))
            if string.find(snippet, "already", 1, true) then
                return true, nil, "already"
            end
            last = "canvas_enroll_http_" .. tostring(res.status)
            H.log.warn("PaidCourse: enroll HTTP %s attempt=%s body=%s",
                tostring(res.status), tostring(attempt),
                string.sub(tostring(res.body or ""), 1, 300))
            if res.status < 500 and res.status ~= 429 then
                return false, nil, last
            end
        end
        if attempt < 3 then
            H.sleep(delays[attempt])
        end
    end
    return false, nil, last
end

local function load_order(order_id, stripe_intent_id)
    local sql
    local bind
    if stripe_intent_id and stripe_intent_id ~= "" then
        sql = [[
            SELECT order_id, account_id, stripe_intent_id, status,
                   currency, total_cents, items_json
            FROM ${SCHEMA}orders
            WHERE stripe_intent_id = :INTENT
        ]]
        bind = { INTENT = stripe_intent_id }
    else
        sql = [[
            SELECT order_id, account_id, stripe_intent_id, status,
                   currency, total_cents, items_json
            FROM ${SCHEMA}orders
            WHERE order_id = :ORDERID
        ]]
        bind = { ORDERID = order_id }
    end
    local _or, qerr = H.query_sync(sql, bind)
    if qerr then return nil, qerr end
    local rows = qrows(_or)
    if not rows or not rows[1] then return nil, nil end
    return rows[1], nil
end

local function entitlement_for_order_course(order_id, course_id)
    local _er, qerr = H.query_sync([[
        SELECT enrollment_id
        FROM ${SCHEMA}user_enrollments
        WHERE order_id = :ORDERID
          AND course_id = :COURSEID
        ORDER BY enrollment_id DESC
    ]], { ORDERID = order_id, COURSEID = course_id })
    if qerr then return nil, qerr end
    local rows = qrows(_er)
    if rows and rows[1] then
        return tonumber(pick(rows[1], "enrollment_id", "ENROLLMENT_ID")), nil
    end
    return nil, nil
end

local function next_enrollment_id()
    local _nr = H.query_sync([[
        SELECT COALESCE(MAX(enrollment_id), 0) + 1 AS next_id
        FROM ${SCHEMA}user_enrollments
    ]], {})
    local rows = qrows(_nr)
    return tonumber(pick(rows and rows[1], "next_id", "NEXT_ID")) or 1
end

local function grant_line(account_id, order_id, line, canvas_user_id, amount_cents, currency)
    local course_id = line.courseId
    local _cr, cerr = H.query_sync([[
        SELECT course_id, pricing_type, published, canvas_course_id, retired
        FROM ${SCHEMA}courses
        WHERE course_id = :COURSEID
          AND published = 1
    ]], { COURSEID = course_id })
    if cerr then
        return nil, "catalog_error"
    end
    local rows = qrows(_cr)
    if not rows or not rows[1] then
        return nil, "course_not_found"
    end
    local course = rows[1]
    local retired = tonumber(pick(course, "retired", "RETIRED")) or 0
    if retired ~= 0 then
        return nil, "not_offered"
    end
    local pricing = string.lower(tostring(pick(course, "pricing_type", "PRICING_TYPE") or ""))
    if pricing ~= "paid" then
        return nil, "not_paid"
    end
    local canvas_course = line.canvasCourseId
        or tonumber(pick(course, "canvas_course_id", "CANVAS_COURSE_ID"))
    if not canvas_course then
        return nil, "no_canvas_course"
    end

    if TOKEN and TOKEN ~= "" then
        if not already_enrolled(canvas_user_id, canvas_course) then
            local ok, _, eerr = enroll_student(canvas_user_id, canvas_course)
            if not ok then
                return nil, eerr or "canvas_enroll_failed"
            end
        end
    else
        H.log.error("PaidCourse: CANVAS_API_KEY not set; skipping LMS enroll")
        return nil, "canvas_unconfigured"
    end

    local existing, eerr = entitlement_for_order_course(order_id, course_id)
    if eerr then
        return nil, "entitlement_lookup_failed"
    end
    if existing then
        return existing, nil
    end

    local source = "paid"
    if line.lineType == "renew" then
        source = "renew"
    end

    local _, serr = H.query_sync([[
        UPDATE ${SCHEMA}user_enrollments
           SET status = 'superseded',
               updated_at = NOW(),
               updated_id = 0
         WHERE account_id = :ACCOUNTID
           AND course_id = :COURSEID
           AND status IN ('active', 'completed', 'pending')
    ]], { ACCOUNTID = account_id, COURSEID = course_id })
    if serr then
        H.log.warn("PaidCourse: supersede err: %s", tostring(serr))
        return nil, "entitlement_error"
    end

    local eid = next_enrollment_id()
    local _, ierr = H.query_sync([[
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
            'active', NOW(), NOW() + INTERVAL '90 days',
            NULL, 0, NULL,
            NULL, 'paid_renew', :SOURCE, :ORDERID,
            NULL, NULL, 0, NOW(), 0, NOW()
        )
    ]], {
        EID = eid,
        ACCOUNTID = account_id,
        COURSEID = course_id,
        CANVASCOURSE = canvas_course,
        SOURCE = source,
        ORDERID = order_id,
    })
    if ierr then
        local again = select(1, entitlement_for_order_course(order_id, course_id))
        if again then
            return again, nil
        end
        H.log.warn("PaidCourse: entitlement insert err: %s", tostring(ierr))
        return nil, "entitlement_error"
    end
    local etype = "purchased"
    if source == "renew" then
        etype = "renewed"
    end
    try_log_event({
        account_id = account_id,
        course_id = course_id,
        canvas_user_id = canvas_user_id,
        canvas_course_id = canvas_course,
        enrollment_id = eid,
        event_type = etype,
        actor = "system",
        amount_cents = amount_cents,
        currency = currency,
        order_id = order_id,
    })
    return eid, nil
end

local function complete_order(order_id)
    local _, uerr = H.query_sync([[
        UPDATE ${SCHEMA}orders
           SET status = 'completed',
               updated_at = NOW(),
               updated_id = 0
         WHERE order_id = :ORDERID
           AND status = 'pending'
    ]], { ORDERID = order_id })
    return uerr
end

function M.fulfill(opts)
    if type(opts) ~= "table" then
        return err("validation", "Missing fulfill opts")
    end
    local stripe_intent_id = opts.stripe_intent_id
    local order_id = tonumber(opts.order_id)
    if (not stripe_intent_id or stripe_intent_id == "") and not order_id then
        return err("validation", "Missing payment intent")
    end

    local order, oerr = load_order(order_id, stripe_intent_id)
    if oerr then
        H.log.warn("PaidCourse: order lookup err: %s", tostring(oerr))
        return err("order_error", "Could not load order")
    end
    if not order then
        return err("order_not_found", "No matching order")
    end

    order_id = tonumber(pick(order, "order_id", "ORDER_ID"))
    local status = tostring(pick(order, "status", "STATUS") or "")
    if status == "completed" then
        return {
            ok = true,
            already = true,
            order_id = order_id,
            stripe_intent_id = pick(order, "stripe_intent_id", "STRIPE_INTENT_ID"),
        }
    end
    if status ~= "pending" then
        return err("order_not_pending", "Order is not pending")
    end

    local want_cents = tonumber(opts.amount_cents)
    local want_cur = tostring(opts.currency or ""):lower()
    local got_cents = tonumber(pick(order, "total_cents", "TOTAL_CENTS"))
    local got_cur = tostring(pick(order, "currency", "CURRENCY") or ""):lower()
    if want_cents and got_cents and want_cents ~= got_cents then
        H.log.warn("PaidCourse: amount mismatch order_id=%s", tostring(order_id))
        return err("amount_mismatch", "Payment amount does not match order")
    end
    if want_cur ~= "" and got_cur ~= "" and want_cur ~= got_cur then
        H.log.warn("PaidCourse: currency mismatch order_id=%s", tostring(order_id))
        return err("amount_mismatch", "Payment currency does not match order")
    end

    local account_id = tonumber(pick(order, "account_id", "ACCOUNT_ID"))
    if not account_id then
        return err("order_error", "Order has no account")
    end

    local _ar, aerr = H.query_sync([[
        SELECT account_id, name
        FROM ${SCHEMA}accounts
        WHERE account_id = :ACCOUNTID
    ]], { ACCOUNTID = account_id })
    if aerr or not qrows(_ar) or not qrows(_ar)[1] then
        return err("account_not_found", "Account not found")
    end
    local acct = qrows(_ar)[1]
    local _or, ierr = H.query_sync([[
        SELECT email
        FROM ${SCHEMA}account_oidc_identities
        WHERE account_id = :ACCOUNTID
        ORDER BY last_seen_at DESC
    ]], { ACCOUNTID = account_id })
    if ierr then
        H.log.warn("PaidCourse: oidc lookup err: %s", tostring(ierr))
    end
    local oidc = qrows(_or) and qrows(_or)[1]
    local email = pick(oidc, "email", "EMAIL")
    local display = pick(acct, "name", "NAME") or email

    local canvas_user_id, lerr = ensure_link(account_id, email, display)
    if not canvas_user_id then
        return err(lerr or "unlinked", "Canvas account is not ready yet")
    end

    local items = parse_items(pick(order, "items_json", "ITEMS_JSON"))
    if #items == 0 then
        return err("order_error", "Order has no lines")
    end

    local granted = {}
    for i = 1, #items do
        local eid, gerr = grant_line(account_id, order_id, items[i], canvas_user_id, got_cents, got_cur)
        if not eid then
            return err(gerr or "fulfill_failed", "Could not grant paid enrollment")
        end
        granted[#granted + 1] = eid
    end

    local uerr = complete_order(order_id)
    if uerr then
        H.log.warn("PaidCourse: complete order err: %s", tostring(uerr))
        return err("order_error", "Could not complete order")
    end

    return {
        ok = true,
        already = false,
        order_id = order_id,
        stripe_intent_id = pick(order, "stripe_intent_id", "STRIPE_INTENT_ID"),
        enrollment_ids = granted,
    }
end

return M
                ]==]
            WHERE group_name = 'Enroll'
              AND script_name = 'PaidCourse';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Restore Enroll.PaidCourse create-on-miss' AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Restore PaidCourse 1352 body
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
