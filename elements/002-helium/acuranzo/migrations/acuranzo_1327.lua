-- Migration: acuranzo_1327.lua
-- PRIORITIZE 2.9: seed Enroll.LogEvent (module, not invokable)
--
-- require("Enroll.LogEvent").record(opts)
-- INSERT enrollment_events then best-effort Canvas wiki PUT.
-- Hydrogen H.http has POST only; page update uses
-- X-HTTP-Method-Override: PUT.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-08-20 - Seed Enroll.LogEvent module (PRIORITIZE 2.9)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1327"
cfg.GROUP_NAME = "Enroll"
cfg.SCRIPT_NAME = "LogEvent"
-- ----------------------------------------------------------------------------
-- Forward: seed Enroll.LogEvent (not invokable)
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
-- Enroll.LogEvent (PRIORITIZE 2.9)
-- Module. require("Enroll.LogEvent").record(opts)
-- Not JWT-invokable. Writers call it after a successful enroll action.
-- INSERT is required; Canvas wiki PUT is best-effort.

local M = {}

local MODULE_NAME = "Course Information"
local PAGE_TITLE = "Enrolment History"
local PAGE_URL = "enrolment-history"
local QUERY_REF = 150

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

local function nzi(v)
    local n = tonumber(v)
    if not n or n == 0 then return 0 end
    return n
end

local function str_or_empty(v)
    if v == nil then return "" end
    local s = tostring(v)
    if s == "null" or s == "NULL" then return "" end
    return s
end

local function json_escape(s)
    s = tostring(s or "")
    s = s:gsub("\\", "\\\\")
    s = s:gsub('"', '\\"')
    s = s:gsub("\n", "\\n")
    s = s:gsub("\r", "\\r")
    s = s:gsub("\t", "\\t")
    return s
end

local function html_escape(s)
    s = tostring(s or "")
    s = s:gsub("&", "&" .. "amp;")
    s = s:gsub("<", "&" .. "lt;")
    s = s:gsub(">", "&" .. "gt;")
    return s
end

local function object_id(obj)
    return tonumber(obj:match('"id"%s*:%s*(%d+)'))
end

local function object_field(obj, key)
    return obj:match('"' .. key .. '"%s*:%s*"([^"]*)"')
end

local function each_object(body)
    local objs = {}
    if type(body) ~= "string" or body == "" then return objs end
    for obj in body:gmatch("%b{}") do
        objs[#objs + 1] = obj
    end
    return objs
end

local BASE = (getenv("CANVAS_BASE_URL", "https://canvas.500courses.com")):gsub("/+$", "")
local TOKEN = getenv("CANVAS_API_KEY", nil)

local function auth_headers()
    return {
        Authorization = "Bearer " .. TOKEN,
        Accept = "application/json",
    }
end

local function http_get(url)
    local res, herr = H.http.get_sync(url, auth_headers(), { timeout = 15 })
    if herr then return nil, herr end
    return res, nil
end

local function http_post_json(url, body, extra_headers)
    local headers = auth_headers()
    headers["Content-Type"] = "application/json"
    if type(extra_headers) == "table" then
        for k, v in pairs(extra_headers) do
            headers[k] = v
        end
    end
    local res, herr = H.http.post_sync(url, body, headers, {
        timeout = 15,
        content_type = "application/json",
    })
    if herr then return nil, herr end
    return res, nil
end

local function http_put_json(url, body)
    return http_post_json(url, body, { ["X-HTTP-Method-Override"] = "PUT" })
end

local function next_event_id()
    local _nr = H.query_sync([[
        SELECT COALESCE(MAX(event_id), 0) + 1 AS next_id
        FROM ${SCHEMA}enrollment_events
    ]], {})
    local rows = qrows(_nr)
    return tonumber(pick(rows and rows[1], "next_id", "NEXT_ID")) or 1
end

local function resolve_canvas_course(course_id, canvas_course_id)
    local cid = nzi(canvas_course_id)
    if cid > 0 then return cid end
    if nzi(course_id) == 0 then return 0 end
    local _cr = H.query_sync([[
        SELECT canvas_course_id
        FROM ${SCHEMA}courses
        WHERE course_id = :COURSEID
    ]], { COURSEID = nzi(course_id) })
    local rows = qrows(_cr)
    return nzi(pick(rows and rows[1], "canvas_course_id", "CANVAS_COURSE_ID"))
end

local function load_history_sql()
    local _qr, qerr = H.query_sync([[
        SELECT code
        FROM ${SCHEMA}queries
        WHERE query_ref = :QREF
        ORDER BY query_id DESC
    ]], { QREF = QUERY_REF })
    if qerr then return nil, qerr end
    local rows = qrows(_qr)
    local code = pick(rows and rows[1], "code", "CODE")
    if type(code) ~= "string" or code == "" then
        return nil, "missing_query"
    end
    return code, nil
end

local function history_pre(course_id)
    local sql, serr = load_history_sql()
    local lines = {}
    if sql then
        local _lr, lerr = H.query_sync(sql, { COURSEID = nzi(course_id) })
        if lerr then
            H.log.warn("LogEvent: history query err: %s", tostring(lerr))
        else
            local rows = qrows(_lr) or {}
            for i = 1, #rows do
                local line = pick(rows[i], "line", "LINE")
                if line and tostring(line) ~= "" then
                    lines[#lines + 1] = html_escape(tostring(line))
                end
            end
        end
    else
        H.log.warn("LogEvent: QueryRef %s load err: %s", tostring(QUERY_REF), tostring(serr))
    end
    if #lines == 0 then
        lines[1] = "(no enrolment events)"
    end
    return "<pre>\n" .. table.concat(lines, "\n") .. "\n</pre>"
end

local function wiki_payload(body)
    return '{"wiki_page":{"title":"' .. PAGE_TITLE
        .. '","body":"' .. json_escape(body)
        .. '","published":false,"editing_roles":"teachers","notify_of_update":false}}'
end

local function ensure_page(canvas_course_id, body)
    local base = BASE .. "/api/v1/courses/" .. tostring(canvas_course_id) .. "/pages"
    local get_url = base .. "/" .. PAGE_URL
    local existing, gerr = http_get(get_url)
    if gerr then
        return nil, "canvas_page_get:" .. tostring(gerr)
    end
    local payload = wiki_payload(body)
    if existing and existing.status >= 200 and existing.status < 300 then
        local res, perr = http_put_json(get_url, payload)
        if perr then return nil, "canvas_page_put:" .. tostring(perr) end
        if not res or res.status < 200 or res.status >= 300 then
            local st = res and res.status or "nil"
            return nil, "canvas_page_put_http_" .. tostring(st)
        end
        return true, nil
    end
    local res, cerr = http_post_json(base, payload)
    if cerr then return nil, "canvas_page_create:" .. tostring(cerr) end
    if not res or res.status < 200 or res.status >= 300 then
        local st = res and res.status or "nil"
        return nil, "canvas_page_create_http_" .. tostring(st)
    end
    return true, nil
end

local function find_module_id(canvas_course_id)
    local url = BASE .. "/api/v1/courses/" .. tostring(canvas_course_id)
        .. "/modules?per_page=100"
    local res, herr = http_get(url)
    if not res then return nil, herr end
    if res.status < 200 or res.status >= 300 then
        return nil, "canvas_modules_http_" .. tostring(res.status)
    end
    for _, obj in ipairs(each_object(res.body or "")) do
        local name = object_field(obj, "name") or ""
        if name == MODULE_NAME then
            return object_id(obj), nil
        end
    end
    return nil, nil
end

local function ensure_module(canvas_course_id)
    local mid, merr = find_module_id(canvas_course_id)
    if merr then return nil, merr end
    if mid then return mid, nil end
    local url = BASE .. "/api/v1/courses/" .. tostring(canvas_course_id) .. "/modules"
    local body = '{"module":{"name":"' .. MODULE_NAME .. '","published":false}}'
    local res, herr = http_post_json(url, body)
    if herr then return nil, herr end
    if not res or res.status < 200 or res.status >= 300 then
        local st = res and res.status or "nil"
        return nil, "canvas_module_http_" .. tostring(st)
    end
    local id = object_id(res.body or "")
    if not id then
        mid = select(1, find_module_id(canvas_course_id))
        return mid, nil
    end
    return id, nil
end

local function module_has_page(canvas_course_id, module_id)
    local url = BASE .. "/api/v1/courses/" .. tostring(canvas_course_id)
        .. "/modules/" .. tostring(module_id) .. "/items?per_page=100"
    local res, herr = http_get(url)
    if not res then return false, herr end
    if res.status < 200 or res.status >= 300 then
        return false, "canvas_items_http_" .. tostring(res.status)
    end
    for _, obj in ipairs(each_object(res.body or "")) do
        local page_url = object_field(obj, "page_url") or ""
        local title = object_field(obj, "title") or ""
        if page_url == PAGE_URL or title == PAGE_TITLE then
            return true, nil
        end
    end
    return false, nil
end

local function ensure_module_item(canvas_course_id, module_id)
    local has, herr = module_has_page(canvas_course_id, module_id)
    if herr then return nil, herr end
    if has then return true, nil end
    local url = BASE .. "/api/v1/courses/" .. tostring(canvas_course_id)
        .. "/modules/" .. tostring(module_id) .. "/items"
    local body = '{"module_item":{"type":"Page","page_url":"' .. PAGE_URL
        .. '","title":"' .. PAGE_TITLE .. '","published":false}}'
    local res, perr = http_post_json(url, body)
    if perr then return nil, perr end
    if not res or res.status < 200 or res.status >= 300 then
        local st = res and res.status or "nil"
        return nil, "canvas_item_http_" .. tostring(st)
    end
    return true, nil
end

local function project_canvas(course_id, canvas_course_id)
    if not TOKEN or TOKEN == "" then
        return nil, "canvas_unconfigured"
    end
    local cid = resolve_canvas_course(course_id, canvas_course_id)
    if cid == 0 then
        return nil, "no_canvas_course"
    end
    local body = history_pre(course_id)
    local ok, perr = ensure_page(cid, body)
    if not ok then return nil, perr end
    local mid, merr = ensure_module(cid)
    if not mid then
        return nil, merr or "no_module"
    end
    local iok, ierr = ensure_module_item(cid, mid)
    if not iok then return nil, ierr end
    return true, nil
end

function M.record(opts)
    if type(opts) ~= "table" then
        return { ok = false, code = "validation", message = "Missing LogEvent opts" }
    end
    local account_id = nzi(opts.account_id)
    local course_id = nzi(opts.course_id)
    local event_type = str_or_empty(opts.event_type)
    if account_id == 0 or course_id == 0 or event_type == "" then
        return { ok = false, code = "validation", message = "account_id, course_id, event_type required" }
    end
    local actor = str_or_empty(opts.actor)
    if actor == "" then actor = "system" end
    local canvas_user_id = nzi(opts.canvas_user_id)
    local canvas_course_id = nzi(opts.canvas_course_id)
    local enrollment_id = nzi(opts.enrollment_id)
    local amount_cents = nzi(opts.amount_cents)
    local currency = str_or_empty(opts.currency)
    local order_id = nzi(opts.order_id)
    local eid = next_event_id()
    local _, ierr = H.query_sync([[
        INSERT INTO ${SCHEMA}enrollment_events (
            event_id, account_id, course_id,
            canvas_user_id, canvas_course_id, enrollment_id,
            event_type, actor, amount_cents, currency, order_id,
            valid_after, valid_until, created_id, created_at, updated_id, updated_at
        ) VALUES (
            :EID, :ACCOUNTID, :COURSEID,
            NULLIF(:CANVASUSER, 0), NULLIF(:CANVASCOURSE, 0), NULLIF(:ENROLLMENTID, 0),
            :EVENTTYPE, :ACTOR, NULLIF(:AMOUNTCENTS, 0), NULLIF(:CURRENCY, ''), NULLIF(:ORDERID, 0),
            NULL, NULL, 0, NOW(), 0, NOW()
        )
    ]], {
        EID = eid,
        ACCOUNTID = account_id,
        COURSEID = course_id,
        CANVASUSER = canvas_user_id,
        CANVASCOURSE = canvas_course_id,
        ENROLLMENTID = enrollment_id,
        EVENTTYPE = event_type,
        ACTOR = actor,
        AMOUNTCENTS = amount_cents,
        CURRENCY = currency,
        ORDERID = order_id,
    })
    if ierr then
        H.log.warn("LogEvent: insert err: %s", tostring(ierr))
        return { ok = false, code = "write_error", message = "Could not write enrollment event" }
    end
    local pok, perr = project_canvas(course_id, canvas_course_id)
    if not pok then
        H.log.warn("LogEvent: canvas project course_id=%s: %s",
            tostring(course_id), tostring(perr))
    end
    return {
        ok = true,
        event_id = eid,
        canvas_ok = pok and true or false,
        canvas_error = perr,
    }
end

return M
                ]==],
                'PRIORITIZE 2.9: enrollment_events writer + Canvas Enrolment History PUT',
                0,
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Enroll.LogEvent module script'                                AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Enroll.LogEvent

            Inserts `Enroll.LogEvent` with `invokable = 0`.
            Writers `require("Enroll.LogEvent")` and call `record`.

            1. INSERT `enrollment_events` (never a missing named bind;
               optional ints bind 0 and store NULL via NULLIF).
            2. Best-effort Canvas projection: unpublished module
               "Course Information", unpublished wiki page
               "Enrolment History" (`enrolment-history`), body =
               `<pre>` + QueryRef #150 `line` rows. Page update uses
               POST + `X-HTTP-Method-Override: PUT` because Hydrogen
               `H.http` has no put_sync.
            3. Canvas HTTP failure is logged; `record` still returns
               `ok=true` with `canvas_ok=false`. Enroll paths must
               not fail closed on the wiki PUT.

            Teachers-only: `published=false`, `editing_roles=teachers`.
            No diagram.
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
              AND script_name = 'LogEvent';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Enroll.LogEvent script'                                     AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Enroll.LogEvent
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
