-- Migration: acuranzo_1343.lua
-- PRIORITIZE 2.1: replace Orchestrators.Orchestrator
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-08-21 - Orchestrator EnsureCanvasUser every 15s

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1343"
cfg.GROUP_NAME = "Orchestrators"
cfg.SCRIPT_NAME = "Orchestrator"
-- ----------------------------------------------------------------------------
-- Forward: replace Orchestrators.Orchestrator body
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
-- Orchestrators.Orchestrator (500 Courses — Phase 30 driver, 1343 15s poll)
-- Periodic Provision.EnsureCanvasUser submit; per-iteration tick for lifecycle probes.

local function qrows(res)
    if type(res) ~= "table" then return nil end
    if type(res.rows) == "table" then return res.rows end
    return res
end

local TICK_MS = 1000
local ENSURE_EVERY = 15
local ensure_countdown = 0

H.log.info("Orchestrator: started (500courses provision driver)")

local function job_active(jobs, name)
    for _, j in ipairs(jobs or {}) do
        local sn = j.script_name or j.SCRIPT_NAME or ""
        local st = string.lower(tostring(j.status or j.STATUS or ""))
        if sn == name and (st == "pending" or st == "running") then
            return true
        end
    end
    return false
end

local function submit_ensure()
    if job_active(H.scoreboard.list(), "Provision.EnsureCanvasUser") then
        H.log.info("Orchestrator: EnsureCanvasUser already active; skip")
        return
    end
    local _qr, err = H.query_sync([[
        SELECT code FROM ${SCHEMA}scripts
        WHERE group_name = 'Provision' AND script_name = 'EnsureCanvasUser'
        LIMIT 1
    ]])
    local rows = qrows(_qr)
    if err or not rows or not rows[1] then
        H.log.warn("Orchestrator: EnsureCanvasUser source missing: %s",
            tostring(err or "no row"))
        return
    end
    local src = rows[1].code or rows[1].CODE
    if not src or src == "" then
        H.log.warn("Orchestrator: EnsureCanvasUser empty code")
        return
    end
    local id = H.scoreboard.submit({
        script_name = "Provision.EnsureCanvasUser",
        source = src,
    })
    if id then
        H.log.info("Orchestrator: submitted EnsureCanvasUser job %s", tostring(id))
    else
        H.log.warn("Orchestrator: EnsureCanvasUser submit failed")
    end
end

local iterations = 0
-- Run once shortly after boot so first-login lag is bounded.
ensure_countdown = 5

while not H.shutdown_requested() do
    iterations = iterations + 1
    ensure_countdown = ensure_countdown - 1
    if ensure_countdown <= 0 then
        ensure_countdown = ENSURE_EVERY
        local ok, err = pcall(submit_ensure)
        if not ok then
            H.log.warn("Orchestrator: ensure pcall err: %s", tostring(err))
        end
    end
    local jobs = H.scoreboard.list()
    H.log.info("Orchestrator: tick %d, %d job(s)", iterations, #jobs)
    H.sleep(TICK_MS)
end

H.log.info("Orchestrator: shutdown requested, exiting after %d iteration(s)", iterations)
            ]==]
            WHERE group_name = 'Orchestrators'
              AND script_name = 'Orchestrator';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Orchestrator EnsureCanvasUser every 15s'                                                       AS name,
        [=[
            # Forward Migration ${MIGRATION}: Orchestrator EnsureCanvasUser every 15s

            PRIORITIZE 2.1. ENSURE_EVERY 60 -> 15. Reverse restores 1299 (60s).
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
-- Orchestrators.Orchestrator (500 Courses — Phase 30 driver, 1299 schema fix)
-- Periodic Provision.EnsureCanvasUser submit; per-iteration tick for lifecycle probes.

local function qrows(res)
    if type(res) ~= "table" then return nil end
    if type(res.rows) == "table" then return res.rows end
    return res
end

local TICK_MS = 1000
local ENSURE_EVERY = 60
local ensure_countdown = 0

H.log.info("Orchestrator: started (500courses provision driver)")

local function job_active(jobs, name)
    for _, j in ipairs(jobs or {}) do
        local sn = j.script_name or j.SCRIPT_NAME or ""
        local st = string.lower(tostring(j.status or j.STATUS or ""))
        if sn == name and (st == "pending" or st == "running") then
            return true
        end
    end
    return false
end

local function submit_ensure()
    if job_active(H.scoreboard.list(), "Provision.EnsureCanvasUser") then
        H.log.info("Orchestrator: EnsureCanvasUser already active; skip")
        return
    end
    local _qr, err = H.query_sync([[
        SELECT code FROM ${SCHEMA}scripts
        WHERE group_name = 'Provision' AND script_name = 'EnsureCanvasUser'
        LIMIT 1
    ]])
    local rows = qrows(_qr)
    if err or not rows or not rows[1] then
        H.log.warn("Orchestrator: EnsureCanvasUser source missing: %s",
            tostring(err or "no row"))
        return
    end
    local src = rows[1].code or rows[1].CODE
    if not src or src == "" then
        H.log.warn("Orchestrator: EnsureCanvasUser empty code")
        return
    end
    local id = H.scoreboard.submit({
        script_name = "Provision.EnsureCanvasUser",
        source = src,
    })
    if id then
        H.log.info("Orchestrator: submitted EnsureCanvasUser job %s", tostring(id))
    else
        H.log.warn("Orchestrator: EnsureCanvasUser submit failed")
    end
end

local iterations = 0
-- Run once shortly after boot so first-login lag is bounded.
ensure_countdown = 5

while not H.shutdown_requested() do
    iterations = iterations + 1
    ensure_countdown = ensure_countdown - 1
    if ensure_countdown <= 0 then
        ensure_countdown = ENSURE_EVERY
        local ok, err = pcall(submit_ensure)
        if not ok then
            H.log.warn("Orchestrator: ensure pcall err: %s", tostring(err))
        end
    end
    local jobs = H.scoreboard.list()
    H.log.info("Orchestrator: tick %d, %d job(s)", iterations, #jobs)
    H.sleep(TICK_MS)
end

H.log.info("Orchestrator: shutdown requested, exiting after %d iteration(s)", iterations)
            ]==]
            WHERE group_name = 'Orchestrators'
              AND script_name = 'Orchestrator';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Restore prior Orchestrators.Orchestrator body'                               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: restore prior Orchestrator

            Restores the pre-2.1 Orchestrators.Orchestrator body.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
