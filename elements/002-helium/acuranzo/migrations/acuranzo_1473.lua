-- Migration: acuranzo_1473.lua
-- Reports.Render.Zone (Reporting)
-- System datetime and item timezones through luatz from the payload. Loaded
-- on the first datetime item that needs it.
--
-- One module of the split Reports.Render. Reports.Render (acuranzo_1479.lua)
-- reads this row with H.altquery_sync and runs it with load() in its
-- per-request environment. The row is not invokable and has no MCP access.
-- Reverse deletes the row. QueryRef 175 stays unused. No lookup.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Split from the acuranzo_1479.lua Reports.Render body

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1473"
cfg.GROUP_NAME = "Reports"
cfg.SCRIPT_NAME = "Render.Zone"

local sql_new = [==[
-- Reports.Render.Zone (Reporting). Lazy: system datetime and timezone items. luatz loads from the Hydrogen payload.
-- Loaded by Reports.Render into its per-request environment.
-- Names defined here are shared with Reports.Render and its other modules.

def_zone = type(def.defaultZone) == "string" and def.defaultZone ~= "" and def.defaultZone or nil
gen_now = nil
function now()
    if gen_now == nil then
        local n = tonumber(P.now)
        if n == nil and H and H.system and H.system.now then n = tonumber(H.system.now()) end
        gen_now = math.floor(n or os.time())
    end
    return gen_now
end
zone_logged = {}
function zone_log(level, message)
    if zone_logged[message] then return end
    zone_logged[message] = true
    add_log(level, message)
end
-- Phase 22a: IANA zones resolve through luatz (group luatz) on host tzdata.
-- Config hooks (env, defaults hardcoded): REPORTS_TZ_ZONEINFO_PATH (reports.tz.zoneinfo_path),
-- REPORTS_TZ_DEFAULT_ZONE (reports.tz.default_zone, used when the report has no defaultZone).
function cfg_env(name)
    local ok, v = pcall(function() return os.getenv and os.getenv(name) end)
    return ok and type(v) == "string" and v ~= "" and v or nil
end
TZ_PATH = cfg_env("REPORTS_TZ_ZONEINFO_PATH") or "/usr/share/zoneinfo"
def_zone = def_zone or cfg_env("REPORTS_TZ_DEFAULT_ZONE")
tzc = nil
function iana_offset(zone, instant)
    if not zone:match("^[%w_+%-/]+$") or zone:find("..", 1, true) or zone:sub(1, 1) == "/" then
        zone_log("warn", "zone " .. zone .. " is not an IANA name; printed UTC")
        return nil
    end
    if tzc == nil then
        local ok, m = pcall(require, "luatz.tzcache")
        tzc = ok and type(m) == "table" and m or false
        if not tzc then
            zone_log("warn", "luatz is not available (" .. tostring(m) .. "); printed UTC")
        elseif tzc.set_base_path then
            tzc.set_base_path(TZ_PATH)
        end
    end
    if not tzc then return nil end
    if io == nil then
        zone_log("warn", "zone " .. zone .. " needs read access to " .. TZ_PATH .. " (io is sandboxed); printed UTC")
        return nil
    end
    local ok, tz = pcall(tzc.get_tz, zone)
    if not ok or type(tz) ~= "table" then
        zone_log("warn", "zone " .. zone .. " is unknown or unreadable under " .. TZ_PATH .. "; printed UTC")
        return nil
    end
    local ok2, info = pcall(tz.find_current, tz, instant)
    if not ok2 or type(info) ~= "table" then
        zone_log("warn", "zone " .. zone .. " has no offset at " .. tostring(instant) .. "; printed UTC")
        return nil
    end
    return info.gmtoff
end
function zone_offset(item, instant)
    local mode = item.timezone or "User"
    if mode == "UTC" then return 0 end
    local zone
    if mode == "Explicit" then
        zone = type(item.zone) == "string" and item.zone ~= "" and item.zone or nil
        if not zone then
            zone_log("info", "item " .. tostring(item.id) .. " has Explicit timezone with no zone; printed UTC")
            return 0
        end
    else
        -- The caller's own claim keeps its login offset (tzoffset drift accepted).
        local off = tonumber(HY.tzoffset)
        if type(HY.tz) == "string" and HY.tz ~= "" and off then return math.floor(off * 60) end
        zone = type(HY.tz) == "string" and HY.tz ~= "" and HY.tz or def_zone
        if not zone then
            zone_log("info", "no user timezone or report default zone; printed UTC")
            return 0
        end
    end
    return iana_offset(zone, instant) or 0
end
]==]

-- ----------------------------------------------------------------------------
-- Forward: insert Reports.Render.Zone
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
                mcp_access,
                mcp_schema,
                mcp_annotations,
                ${COMMON_FIELDS}
            )
            VALUES (
                '${GROUP_NAME}',
                '${SCRIPT_NAME}',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
]====] .. sql_new .. [====[
                ]==],
                'Reports.Render.Zone module (Reporting)',
                0,
                0,
                NULL,
                NULL,
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Insert Reports.Render.Zone'                                        AS name,
        [=[
            # Forward Migration ${MIGRATION}: Reports.Render.Zone

            Inserts group Reports, script Render.Zone. System datetime and item timezones through luatz from the payload. Loaded on the first datetime item that needs it.
            Reports.Render loads it. It is not invokable.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Reverse: delete Reports.Render.Zone
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
            WHERE group_name = '${GROUP_NAME}'
              AND script_name = '${SCRIPT_NAME}';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Delete Reports.Render.Zone'                                        AS name,
        [=[
            # Reverse Migration ${MIGRATION}: delete Reports.Render.Zone

            Deletes the group Reports script Render.Zone row.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
