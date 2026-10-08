-- Migration: acuranzo_1357.lua
-- PRIORITIZE 2.18: seed Enroll.CanvasSeat (module, not invokable)
--
-- require("Enroll.CanvasSeat").unenroll / enroll / find_active
-- Hydrogen H.http is GET/POST only; Canvas DELETE uses
-- X-HTTP-Method-Override: DELETE. task=delete so Unarchive can
-- POST a fresh StudentEnrollment.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-08-21 - Seed Enroll.CanvasSeat module (PRIORITIZE 2.18)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1357"
cfg.GROUP_NAME = "Enroll"
cfg.SCRIPT_NAME = "CanvasSeat"
-- ----------------------------------------------------------------------------
-- Forward: seed Enroll.CanvasSeat (not invokable)
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
-- Enroll.CanvasSeat (PRIORITIZE 2.18)
-- Module. require("Enroll.CanvasSeat")
-- Not JWT-invokable. Archive / Sync / Refund / Deactivate call it.
-- Secrets: os.getenv("CANVAS_API_KEY"), os.getenv("CANVAS_BASE_URL").

local M = {}

local function getenv(k, default)
    if type(os) == "table" and type(os.getenv) == "function" then
        local v = os.getenv(k)
        if v and v ~= "" then return v end
    end
    return default
end

local function nzi(v)
    local n = tonumber(v)
    if not n or n == 0 then return 0 end
    return n
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

local BASE = (getenv("CANVAS_BASE_URL", "https://canvas.500courses.com")):gsub("/+$", "")
local TOKEN = getenv("CANVAS_API_KEY", nil)

local function auth_headers()
    return {
        Authorization = "Bearer " .. TOKEN,
        Accept = "application/json",
    }
end

local function http_get(url)
    local res, err = H.http.get_sync(url, auth_headers(), { timeout = 15 })
    if err then return nil, err end
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
    local res, err = H.http.post_sync(url, body or "", headers, {
        timeout = 15,
        content_type = "application/json",
    })
    if err then return nil, err end
    return res, nil
end

function M.configured()
    return TOKEN ~= nil and TOKEN ~= ""
end

function M.find_active(canvas_user_id, canvas_course_id)
    local uid = nzi(canvas_user_id)
    local cid = nzi(canvas_course_id)
    if uid == 0 or cid == 0 then
        return nil
    end
    local url = BASE .. "/api/v1/courses/" .. tostring(cid)
        .. "/enrollments?user_id=" .. tostring(uid)
        .. "&type[]=StudentEnrollment&state[]=active&state[]=invited&per_page=5"
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

function M.enroll(opts)
    opts = opts or {}
    if not M.configured() then
        return { ok = false, code = "canvas_unconfigured" }
    end
    local uid = nzi(opts.canvas_user_id)
    local cid = nzi(opts.canvas_course_id)
    if uid == 0 or cid == 0 then
        return { ok = false, code = "validation" }
    end
    local existing = M.find_active(uid, cid)
    if existing then
        return { ok = true, already = true, canvas_enrollment_id = existing }
    end
    local body = string.format(
        '{"enrollment":{"user_id":%d,"type":"StudentEnrollment","enrollment_state":"active","notify":false}}',
        uid
    )
    local url = BASE .. "/api/v1/courses/" .. tostring(cid) .. "/enrollments"
    local res, err = http_post_json(url, body)
    if not res then
        return { ok = false, code = "canvas_enroll_failed" }
    end
    if res.status >= 200 and res.status < 300 then
        local eid = object_id(res.body or "") or M.find_active(uid, cid)
        return { ok = true, already = false, canvas_enrollment_id = eid }
    end
    local snippet = string.lower(string.sub(tostring(res.body or ""), 1, 400))
    if string.find(snippet, "already", 1, true) then
        return {
            ok = true,
            already = true,
            canvas_enrollment_id = M.find_active(uid, cid),
        }
    end
    H.log.warn("CanvasSeat.enroll HTTP %s body=%s",
        tostring(res.status), string.sub(tostring(res.body or ""), 1, 300))
    return { ok = false, code = "canvas_enroll_http_" .. tostring(res.status) }
end

local function delete_enrollment(canvas_course_id, canvas_enrollment_id)
    local cid = nzi(canvas_course_id)
    local eid = nzi(canvas_enrollment_id)
    if cid == 0 or eid == 0 then
        return { ok = false, code = "validation" }
    end
    local url = BASE .. "/api/v1/courses/" .. tostring(cid)
        .. "/enrollments/" .. tostring(eid) .. "?task=delete"
    local res, err = http_post_json(url, "{}", {
        ["X-HTTP-Method-Override"] = "DELETE",
    })
    if not res then
        return { ok = false, code = "canvas_unenroll_failed" }
    end
    if res.status >= 200 and res.status < 300 then
        return { ok = true, already = false }
    end
    if res.status == 404 then
        return { ok = true, already = true }
    end
    local snippet = string.lower(string.sub(tostring(res.body or ""), 1, 400))
    if string.find(snippet, "not found", 1, true)
        or string.find(snippet, "does not exist", 1, true) then
        return { ok = true, already = true }
    end
    H.log.warn("CanvasSeat.unenroll HTTP %s body=%s",
        tostring(res.status), string.sub(tostring(res.body or ""), 1, 300))
    return { ok = false, code = "canvas_unenroll_http_" .. tostring(res.status) }
end

function M.unenroll(opts)
    opts = opts or {}
    if not M.configured() then
        return { ok = false, code = "canvas_unconfigured" }
    end
    local uid = nzi(opts.canvas_user_id)
    local cid = nzi(opts.canvas_course_id)
    local eid = nzi(opts.canvas_enrollment_id)
    if cid == 0 then
        return { ok = true, already = true }
    end
    if eid == 0 and uid ~= 0 then
        eid = nzi(M.find_active(uid, cid))
    end
    if eid == 0 then
        return { ok = true, already = true }
    end
    return delete_enrollment(cid, eid)
end

return M
                ]==],
                'PRIORITIZE 2.18: Canvas enroll/unenroll module',
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
        'Seed Enroll.CanvasSeat module'                                     AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Enroll.CanvasSeat

            Module `require("Enroll.CanvasSeat")` (`invokable=0`).

            - `enroll` — POST StudentEnrollment; already-enrolled is ok.
            - `unenroll` — DELETE `task=delete` via method override.
              Missing / 404 is ok (already gone).
            - `find_active` — GET enrollments for user+course.

            Secrets stay in env (`CANVAS_*`). No diagram.
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
              AND script_name = 'CanvasSeat';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Enroll.CanvasSeat script'                                   AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Enroll.CanvasSeat
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
