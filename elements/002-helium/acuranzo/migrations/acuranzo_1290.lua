-- Migration: acuranzo_1290.lua
-- Phase 31: intro course enrollment on first Canvas provision
--
-- Extends the existing Provision.EnsureCanvasUser script (seeded by 1289)
-- with a best-effort, idempotent enroll of the freshly linked Canvas user
-- into the tenant intro course. Keeps Hydrogen Canvas-agnostic: outbound REST
-- via H.http + env token, same as Phase 30. No new Canvas-named C module.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.4 - 2026-08-08 - Reverse must flip type 1003→forward (TestMigration loop)
-- 1.0.3 - 2026-08-08 - Default CANVAS_INTRO_COURSE_ID 801→1001 (5C-001-W5C-EN)
-- 1.0.2 - 2026-08-08 - DB2: UPDATE-only (drop SELECT/WHERE NOT EXISTS INSERT; avoids SQL1585N)
-- 1.0.1 - 2026-08-08 - DB2: ${DUMMY_TABLE} before WHERE NOT EXISTS on script seed
-- 1.0.0 - 2026-08-08 - Extend EnsureCanvasUser with enroll_intro_course step

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1290"
cfg.GROUP_NAME = "Provision"
cfg.SCRIPT_NAME = "EnsureCanvasUser"
-- ----------------------------------------------------------------------------
-- Forward: upsert Provision.EnsureCanvasUser with intro-course enroll step
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
            -- Phase 31: replace EnsureCanvasUser body (row seeded by 1289).
            UPDATE ${SCHEMA}scripts
             SET code = [==[
 -- Provision.EnsureCanvasUser (Phase 30 + Phase 31 intro enroll)
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

 local function link_lookup(account_id)
     local _qr, err = H.query_sync([[
         SELECT link_id, account_id, canvas_user_id, canvas_email, last_seen_at
         FROM lithium.account_canvas_links
         WHERE account_id = :ACCOUNTID
     ]], { ACCOUNTID = account_id })
     if err then return nil, err end
     local rows = qrows(_qr)
     if rows and rows[1] then return rows[1], nil end
     return nil, nil
 end

 local function link_insert(account_id, canvas_user_id, canvas_email)
     local _qr, err = H.query_sync([[
         INSERT INTO lithium.account_canvas_links (
             link_id, account_id, canvas_user_id, canvas_email, last_seen_at,
             valid_after, valid_until, created_id, created_at, updated_id, updated_at
         )
         SELECT
             (SELECT COALESCE(MAX(link_id), 0) + 1 FROM lithium.account_canvas_links),
             a.account_id,
             :CANVASUSERID,
             :CANVASEMAIL,
             NOW(),
             '2025-01-01 00:00:00',
             '2035-01-01 00:00:00',
             0, NOW(), 0, NOW()
         FROM lithium.accounts a
         WHERE a.account_id = :ACCOUNTID
           AND NOT EXISTS (
               SELECT 1 FROM lithium.account_canvas_links acl
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
         UPDATE lithium.account_canvas_links
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
     FROM lithium.account_oidc_identities oi
     JOIN lithium.accounts a ON a.account_id = oi.account_id
     WHERE oi.email IS NOT NULL
       AND oi.email <> ''
       AND NOT EXISTS (
           SELECT 1 FROM lithium.account_canvas_links acl
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
                 ]==],
                 updated_at = ${NOW},
                 summary = 'Phase 30/31: ensure Canvas user + link + intro course enroll'
             WHERE group_name = '${GROUP_NAME}'
               AND script_name = '${SCRIPT_NAME}';

             ${SUBQUERY_DELIMITER}

             UPDATE ${SCHEMA}${QUERIES}
               SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
             WHERE query_ref = ${MIGRATION}
               and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
         ]=]
                                                                             AS code,
         'Extend Provision.EnsureCanvasUser with intro-course enroll (Phase 31)' AS name,
         [=[
             # Forward Migration ${MIGRATION}: Phase 31 intro course enrollment

             Upserts the `Provision.EnsureCanvasUser` worker script (seeded by
             1289) so that after a Canvas user is created/linked for a new OIDC
             account, it is also enrolled (StudentEnrollment, active) into the
             tenant intro course.

              Intro course id comes from env CANVAS_INTRO_COURSE_ID (default 1001
              = 5C-001-W5C-EN Welcome to 500 Courses). Enroll is
             best-effort: a non-2xx (usually "already enrolled") is logged and
             ignored so re-provisioning never spams the Canvas API. Idempotency
             at the account level holds because the Orchestrator only submits
             EnsureCanvasUser for accounts without an account_canvas_links row.

             Requires Scripting.Enabled, CANVAS_API_KEY / CANVAS_BASE_URL /
             CANVAS_INTRO_COURSE_ID env, and the 1283-1289 schema. Hydrogen must
             be rolled so the Orchestrator picks up the new script source.
         ]=]
                                                                             AS summary,
         '{}'                                                                AS collection,
         ${COMMON_INSERT}
     FROM next_query_id;

 ]====]})

-- ----------------------------------------------------------------------------
-- Reverse
-- ----------------------------------------------------------------------------
-- Phase 31 has no own schema; the script body is owned/removed by 1289's
-- reverse. Reverse here is a no-op marker so the migration record stays
-- consistent (operators re-apply 1289 to fully remove the script).
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
            -- Phase 31 reverse: no schema/script row of its own (body owned by
            -- 1289). Still MUST flip applied→forward so TestMigration's APPLY
            -- watermark drops; a pure SELECT left APPLY stuck at 1290 and
            -- tripped "Reverse migration applied but APPLY value unchanged".
            SELECT 1 ${DUMMY_TABLE};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                             AS code,
        'Phase 31 reverse marker (type flip only)'                           AS name,
        [=[
            # Reverse Migration ${MIGRATION}: type flip only

            Phase 31 introduced no new schema or script row; it extended the
            script owned by 1289. Reverse does not restore the pre-31 script
            body (re-APPLY 1289 for a full remove). It only marks this
            migration un-applied so the chain can reverse past 1290.
        ]=]
                                                                             AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

 ]====]})

return queries end
