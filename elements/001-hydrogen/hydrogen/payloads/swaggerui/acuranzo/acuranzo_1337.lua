-- Migration: acuranzo_1337.lua
-- PRIORITIZE 2.29: seed Catalog.SyncFromCanvas (not invokable)
--
-- Pulls Canvas name / public_description / workflow_state into
-- linked Lithium courses rows. Never INSERT. invokable=0 until
-- Course Manager + 2.23. Operator/DQM runs it; Part 5 is the button.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-08-20 - Seed Catalog.SyncFromCanvas (PRIORITIZE 2.29)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1337"
cfg.GROUP_NAME = "Catalog"
cfg.SCRIPT_NAME = "SyncFromCanvas"
-- ----------------------------------------------------------------------------
-- Forward: seed Catalog.SyncFromCanvas (not invokable)
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
-- Catalog.SyncFromCanvas (PRIORITIZE 2.29)
-- invokable=0. Operator/DQM (later Course Manager + 2.23).
-- GET Canvas course by canvas_course_id. Map name / parsed
-- public_description / workflow_state → Lithium. Never INSERT.
-- Skip empty Canvas copy. Image/slug/code/featured/sort/prices
-- are Lithium-owned and never overwritten.
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

local function trim(s)
    s = tostring(s or "")
    return (s:gsub("^%s+", ""):gsub("%s+$", ""))
end

local function truncate(s, n)
    s = tostring(s or "")
    if #s <= n then return s end
    return s:sub(1, n)
end

local function fail(code, message)
    H.set_result_json({ ok = false, code = code, message = message or code })
    return 0
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

local function extract_json_string(body, key)
    if type(body) ~= "string" or body == "" then return nil end
    local pat = '"' .. key .. '"%s*:%s*'
    local _, e = body:find(pat)
    if not e then return nil end
    local rest = body:sub(e + 1)
    if rest:match("^null") then return nil end
    if rest:sub(1, 1) ~= '"' then
        local lit = rest:match("^([^,}%s]+)")
        return lit
    end
    local i = 2
    local out = {}
    while i <= #rest do
        local c = rest:sub(i, i)
        if c == "\\" then
            local n = rest:sub(i + 1, i + 1)
            if n == "n" then out[#out + 1] = "\n"
            elseif n == "r" then out[#out + 1] = "\r"
            elseif n == "t" then out[#out + 1] = "\t"
            elseif n == '"' then out[#out + 1] = '"'
            elseif n == "\\" then out[#out + 1] = "\\"
            elseif n == "/" then out[#out + 1] = "/"
            elseif n == "u" then
                local hex = rest:sub(i + 2, i + 5)
                out[#out + 1] = "\\u" .. hex
                i = i + 4
            else
                out[#out + 1] = n
            end
            i = i + 2
        elseif c == '"' then
            return table.concat(out)
        else
            out[#out + 1] = c
            i = i + 1
        end
    end
    return nil
end

local function read_json_string(s, i)
    if s:sub(i, i) ~= '"' then return nil, i end
    local out = {}
    i = i + 1
    while i <= #s do
        local c = s:sub(i, i)
        if c == "\\" then
            local n = s:sub(i + 1, i + 1)
            if n == "n" then out[#out + 1] = "\n"
            elseif n == "r" then out[#out + 1] = "\r"
            elseif n == "t" then out[#out + 1] = "\t"
            elseif n == '"' then out[#out + 1] = '"'
            elseif n == "\\" then out[#out + 1] = "\\"
            elseif n == "/" then out[#out + 1] = "/"
            else out[#out + 1] = n end
            i = i + 2
        elseif c == '"' then
            return table.concat(out), i + 1
        else
            out[#out + 1] = c
            i = i + 1
        end
    end
    return table.concat(out), i
end

local function parse_json_object(s)
    local t = {}
    if type(s) == "table" then
        for k, v in pairs(s) do
            if type(k) == "string" then t[k] = str_or_empty(v) end
        end
        return t
    end
    if type(s) ~= "string" or s == "" then return t end
    local i = s:find("{", 1, true) or 1
    i = i + 1
    while i <= #s do
        while i <= #s and s:sub(i, i):match("%s") do i = i + 1 end
        if i > #s or s:sub(i, i) == "}" then break end
        if s:sub(i, i) ~= '"' then break end
        local key
        key, i = read_json_string(s, i)
        while i <= #s and s:sub(i, i):match("%s") do i = i + 1 end
        if s:sub(i, i) == ":" then i = i + 1 end
        while i <= #s and s:sub(i, i):match("%s") do i = i + 1 end
        local val
        if s:sub(i, i) == '"' then
            val, i = read_json_string(s, i)
        else
            local lit = s:sub(i):match("^([^,}%s]+)") or ""
            val = lit
            i = i + #lit
        end
        if key and key ~= "" then t[key] = str_or_empty(val) end
        while i <= #s and s:sub(i, i) ~= "," and s:sub(i, i) ~= "}" do
            i = i + 1
        end
        if s:sub(i, i) == "," then i = i + 1 end
    end
    return t
end

local function encode_json_object(map)
    local keys = {}
    for k in pairs(map or {}) do
        keys[#keys + 1] = k
    end
    table.sort(keys)
    local parts = {}
    for _, k in ipairs(keys) do
        parts[#parts + 1] = '"' .. json_escape(k) .. '":"'
            .. json_escape(tostring(map[k] or "")) .. '"'
    end
    return "{" .. table.concat(parts, ",") .. "}"
end

local function parse_footer(footer)
    if type(footer) ~= "string" or trim(footer) == "" then return nil end
    local tags = {}
    local n = 0
    for line in footer:gmatch("[^\n]+") do
        local tline = trim(line)
        if tline ~= "" and tline ~= "---" then
            local colon = tline:find(":", 1, true)
            if colon and colon > 1 then
                local k = trim(tline:sub(1, colon - 1))
                local v = trim(tline:sub(colon + 1))
                if k ~= "" then
                    tags[k] = v
                    n = n + 1
                end
            end
        end
    end
    if n == 0 then return nil end
    return tags
end

local function split_brochure(blob)
    if type(blob) ~= "string" or trim(blob) == "" then
        return nil, nil, nil
    end
    local parts = {}
    local buf = {}
    local function flush()
        parts[#parts + 1] = trim(table.concat(buf, "\n"))
        buf = {}
    end
    for line in (blob .. "\n"):gmatch("(.-)\n") do
        if trim(line) == "---" then
            flush()
        else
            buf[#buf + 1] = line
        end
    end
    flush()
    if #parts <= 1 then
        local only = parts[1]
        if only == "" then return nil, nil, nil end
        return nil, only, nil
    end
    local summary = parts[1]
    if summary == "" then summary = nil end
    local description = parts[2]
    if description == "" then description = nil end
    local footer = nil
    if #parts >= 3 then
        footer = table.concat(parts, "\n", 3)
    end
    return summary, description, footer
end

local function truthy_flag(v)
    local s = trim(str_or_empty(v)):lower()
    if s == "true" or s == "1" or s == "yes" then return 1 end
    if s == "false" or s == "0" or s == "no" then return 0 end
    if s ~= "" then return 1 end
    return 0
end

local function try_log_event(opts)
    local ok, mod = pcall(require, "Catalog.LogEvent")
    if not ok or type(mod) ~= "table" or type(mod.record) ~= "function" then
        H.log.warn("SyncFromCanvas: LogEvent unavailable: %s", tostring(mod))
        return
    end
    local r = mod.record(opts)
    if type(r) == "table" and r.ok == false then
        H.log.warn("SyncFromCanvas: LogEvent: %s", tostring(r.code or r.message))
    end
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
    local res, herr = H.http.get_sync(url, auth_headers(), { timeout = 30 })
    if herr then return nil, herr end
    return res, nil
end

local function load_targets(course_id, canvas_course_id, all_linked)
    local sql
    local binds = {}
    if all_linked then
        sql = [[
            SELECT course_id, canvas_course_id, title, summary, description,
                   published, tags, license, level, delivery_style, source,
                   has_ai, has_quizzes, has_badge, has_certificate,
                   canvas_updated_at
            FROM ${SCHEMA}courses
            WHERE canvas_course_id IS NOT NULL
            ORDER BY course_id
        ]]
    elseif course_id > 0 then
        sql = [[
            SELECT course_id, canvas_course_id, title, summary, description,
                   published, tags, license, level, delivery_style, source,
                   has_ai, has_quizzes, has_badge, has_certificate,
                   canvas_updated_at
            FROM ${SCHEMA}courses
            WHERE course_id = :COURSEID
        ]]
        binds.COURSEID = course_id
    else
        sql = [[
            SELECT course_id, canvas_course_id, title, summary, description,
                   published, tags, license, level, delivery_style, source,
                   has_ai, has_quizzes, has_badge, has_certificate,
                   canvas_updated_at
            FROM ${SCHEMA}courses
            WHERE canvas_course_id = :CANVASCOURSEID
        ]]
        binds.CANVASCOURSEID = canvas_course_id
    end
    local res, err = H.query_sync(sql, binds)
    if err then return nil, err end
    return qrows(res) or {}, nil
end

local function int_flag(v)
    local n = tonumber(v)
    if n and n ~= 0 then return 1 end
    return 0
end

local function apply_one(row)
    local course_id = nzi(pick(row, "course_id", "COURSE_ID"))
    local canvas_id = nzi(pick(row, "canvas_course_id", "CANVAS_COURSE_ID"))
    if course_id == 0 then return "skip", "no_course" end
    if canvas_id == 0 then return "skip", "no_canvas" end
    local url = BASE .. "/api/v1/courses/" .. tostring(canvas_id)
    local res, herr = http_get(url)
    if herr then return "error", "canvas_get:" .. tostring(herr) end
    if not res or res.status < 200 or res.status >= 300 then
        local st = res and res.status or "nil"
        return "error", "canvas_http_" .. tostring(st)
    end
    local body = res.body or ""
    local name = trim(extract_json_string(body, "name") or "")
    local public_desc = extract_json_string(body, "public_description")
    local workflow = trim(extract_json_string(body, "workflow_state") or "")
    local canvas_updated = trim(extract_json_string(body, "updated_at") or "")

    local new_title = str_or_empty(pick(row, "title", "TITLE"))
    if name ~= "" then new_title = truncate(name, 500) end

    local cur_summary = str_or_empty(pick(row, "summary", "SUMMARY"))
    local cur_desc = str_or_empty(pick(row, "description", "DESCRIPTION"))
    local new_summary = cur_summary
    local new_desc = cur_desc
    local footer_tags = nil
    if type(public_desc) == "string" and trim(public_desc) ~= "" then
        local s, d, f = split_brochure(public_desc)
        if s ~= nil and s ~= "" then new_summary = truncate(s, 500) end
        if d ~= nil and d ~= "" then new_desc = d end
        footer_tags = parse_footer(f)
    end

    local new_published = int_flag(pick(row, "published", "PUBLISHED"))
    if workflow ~= "" then
        if workflow:lower() == "available" then
            new_published = 1
        else
            new_published = 0
        end
    end

    local cur_tags = parse_json_object(pick(row, "tags", "TAGS"))
    local merged = {}
    for k, v in pairs(cur_tags) do merged[k] = v end
    local upserted = {}
    if footer_tags then
        for k, v in pairs(footer_tags) do
            merged[k] = v
            upserted[#upserted + 1] = k
        end
        table.sort(upserted)
    end
    local tags_json = encode_json_object(merged)

    local new_license = str_or_empty(pick(row, "license", "LICENSE"))
    local new_level = str_or_empty(pick(row, "level", "LEVEL"))
    local new_delivery = str_or_empty(pick(row, "delivery_style", "DELIVERY_STYLE"))
    local new_source = str_or_empty(pick(row, "source", "SOURCE"))
    local new_ai = int_flag(pick(row, "has_ai", "HAS_AI"))
    local new_quizzes = int_flag(pick(row, "has_quizzes", "HAS_QUIZZES"))
    local new_badge = int_flag(pick(row, "has_badge", "HAS_BADGE"))
    local new_cert = int_flag(pick(row, "has_certificate", "HAS_CERTIFICATE"))
    if footer_tags then
        if footer_tags.License ~= nil then new_license = footer_tags.License end
        if footer_tags.Level ~= nil then new_level = footer_tags.Level end
        if footer_tags.Source ~= nil then new_source = footer_tags.Source end
        if footer_tags.Delivery ~= nil then new_delivery = footer_tags.Delivery end
        if footer_tags.DeliveryStyle ~= nil then new_delivery = footer_tags.DeliveryStyle end
        if footer_tags.Style ~= nil then new_delivery = footer_tags.Style end
        if footer_tags.AI ~= nil then new_ai = truthy_flag(footer_tags.AI) end
        if footer_tags.Quizzes ~= nil then new_quizzes = truthy_flag(footer_tags.Quizzes) end
        if footer_tags.Badge ~= nil then new_badge = truthy_flag(footer_tags.Badge) end
        if footer_tags.Certificate ~= nil then new_cert = truthy_flag(footer_tags.Certificate) end
    end

    local old_title = str_or_empty(pick(row, "title", "TITLE"))
    local old_published = int_flag(pick(row, "published", "PUBLISHED"))
    local old_tags = encode_json_object(cur_tags)
    local old_license = str_or_empty(pick(row, "license", "LICENSE"))
    local old_level = str_or_empty(pick(row, "level", "LEVEL"))
    local old_delivery = str_or_empty(pick(row, "delivery_style", "DELIVERY_STYLE"))
    local old_source = str_or_empty(pick(row, "source", "SOURCE"))
    local old_ai = int_flag(pick(row, "has_ai", "HAS_AI"))
    local old_quizzes = int_flag(pick(row, "has_quizzes", "HAS_QUIZZES"))
    local old_badge = int_flag(pick(row, "has_badge", "HAS_BADGE"))
    local old_cert = int_flag(pick(row, "has_certificate", "HAS_CERTIFICATE"))

    local changed = {}
    if new_title ~= old_title then changed.title = new_title end
    if new_summary ~= cur_summary then changed.summary = new_summary end
    if new_desc ~= cur_desc then changed.description = true end
    if new_published ~= old_published then
        changed.published = (new_published == 1) and "available" or "hidden"
    end
    if tags_json ~= old_tags then
        changed.tags = table.concat(upserted, ",")
    end
    local typed_changed = new_license ~= old_license
        or new_level ~= old_level
        or new_delivery ~= old_delivery
        or new_source ~= old_source
        or new_ai ~= old_ai
        or new_quizzes ~= old_quizzes
        or new_badge ~= old_badge
        or new_cert ~= old_cert
    if not next(changed) and not typed_changed and canvas_updated == "" then
        return "skip", "unchanged"
    end
    if not next(changed) and not typed_changed then
        local _, werr = H.query_sync([[
            UPDATE ${SCHEMA}courses
               SET canvas_updated_at = CAST(NULLIF(:CANVASUPDATEDAT, '') AS ${TIMESTAMP_TZ}),
                   updated_at = NOW()
             WHERE course_id = :COURSEID
        ]], {
            CANVASUPDATEDAT = canvas_updated,
            COURSEID = course_id,
        })
        if werr then return "error", "write:" .. tostring(werr) end
        return "skip", "watermark"
    end

    local _, werr = H.query_sync([[
        UPDATE ${SCHEMA}courses
           SET title = :TITLE,
               summary = NULLIF(:SUMMARY, ''),
               description = NULLIF(:DESCRIPTION, ''),
               published = :PUBLISHED,
               tags = CAST(:TAGS AS ${JSON}),
               license = NULLIF(:LICENSE, ''),
               level = NULLIF(:LEVEL, ''),
               delivery_style = NULLIF(:DELIVERY, ''),
               source = NULLIF(:SOURCE, ''),
               has_ai = :HASAI,
               has_quizzes = :HASQUIZZES,
               has_badge = :HASBADGE,
               has_certificate = :HASCERT,
               canvas_updated_at = CAST(NULLIF(:CANVASUPDATEDAT, '') AS ${TIMESTAMP_TZ}),
               updated_at = NOW(),
               updated_id = 0
         WHERE course_id = :COURSEID
    ]], {
        TITLE = new_title,
        SUMMARY = new_summary,
        DESCRIPTION = new_desc,
        PUBLISHED = new_published,
        TAGS = tags_json,
        LICENSE = new_license,
        LEVEL = new_level,
        DELIVERY = new_delivery,
        SOURCE = new_source,
        HASAI = new_ai,
        HASQUIZZES = new_quizzes,
        HASBADGE = new_badge,
        HASCERT = new_cert,
        CANVASUPDATEDAT = canvas_updated,
        COURSEID = course_id,
    })
    if werr then return "error", "write:" .. tostring(werr) end

    if changed.title then
        try_log_event({
            course_id = course_id,
            canvas_course_id = canvas_id,
            event_type = "title",
            actor = "system",
            detail = truncate(new_title, 200),
        })
    end
    if changed.summary then
        try_log_event({
            course_id = course_id,
            canvas_course_id = canvas_id,
            event_type = "summary",
            actor = "system",
            detail = truncate(new_summary, 200),
        })
    end
    if changed.description then
        try_log_event({
            course_id = course_id,
            canvas_course_id = canvas_id,
            event_type = "description",
            actor = "system",
            detail = "",
        })
    end
    if changed.published then
        try_log_event({
            course_id = course_id,
            canvas_course_id = canvas_id,
            event_type = "published",
            actor = "system",
            detail = changed.published,
        })
    end
    if changed.tags or typed_changed then
        try_log_event({
            course_id = course_id,
            canvas_course_id = canvas_id,
            event_type = "tags",
            actor = "system",
            detail = changed.tags or "",
        })
    end
    return "synced", nil
end

if not TOKEN or TOKEN == "" then
    return fail("canvas_unconfigured", "CANVAS_API_KEY is not set")
end

params = params or {}
local action = trim(str_or_empty(params.action)):lower()
local course_id = nzi(params.course_id)
local canvas_course_id = nzi(params.canvas_course_id)
local all_linked = (action == "all" or action == "linked")
if not all_linked and course_id == 0 and canvas_course_id == 0 then
    return fail("validation", "course_id, canvas_course_id, or action=all required")
end

local rows, lerr = load_targets(course_id, canvas_course_id, all_linked)
if lerr then
    return fail("read_error", "Could not load courses")
end
if #rows == 0 then
    H.set_result_json({ ok = true, synced = 0, skipped = 0, errors = 0, scanned = 0 })
    return 0
end

local synced, skipped, errors = 0, 0, 0
for i = 1, #rows do
    local status, reason = apply_one(rows[i])
    if status == "synced" then
        synced = synced + 1
    elseif status == "skip" then
        skipped = skipped + 1
    else
        errors = errors + 1
        H.log.warn("SyncFromCanvas: course=%s %s",
            tostring(pick(rows[i], "course_id", "COURSE_ID")),
            tostring(reason))
    end
end

H.set_current_state("done")
H.set_result_json({
    ok = true,
    synced = synced,
    skipped = skipped,
    errors = errors,
    scanned = #rows,
})
return 0
                ]==],
                'PRIORITIZE 2.29: Canvas→Lithium brochure sync (invokable=0)',
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
        'Seed Catalog.SyncFromCanvas script'                                AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Catalog.SyncFromCanvas

            Inserts `Catalog.SyncFromCanvas` with `invokable = 0`.
            Operator/DQM invoke until Course Manager + 2.23.

            Params: `course_id` or `canvas_course_id` or `action=all`.

            For each linked row: GET Canvas course; map `name` → title
            (skip empty); parse `public_description` (`---` grammar);
            `workflow_state=available` → `published=1`, else 0;
            footer keys upsert `tags` JSON and dual-write known typed
            columns. Never INSERT. Never overwrite image/slug/code/
            featured/sort/prices. Real field changes append
            `catalog_events` via `Catalog.LogEvent` (wiki best-effort).

            CAST canvas_updated_at uses PostgreSQL `timestamptz`
            (lithium). No diagram.
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
            WHERE group_name = 'Catalog'
              AND script_name = 'SyncFromCanvas';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Catalog.SyncFromCanvas script'                              AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Catalog.SyncFromCanvas
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
