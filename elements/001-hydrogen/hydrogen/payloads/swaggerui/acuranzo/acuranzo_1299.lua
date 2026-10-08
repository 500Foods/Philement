-- Migration: acuranzo_1299.lua
-- Fix Orchestrator scripts schema + tick/shutdown markers (follow-up to mig 1289)
--
-- Migration 1289 embedded hardcoded lithium.scripts in the Orchestrator body.
-- On DB2 (and any non-lithium schema) that yields SQL0204N LITHIUM.SCRIPTS and
-- blocks the EnsureCanvasUser poll. Quiet tick logging (every 60s) and a
-- shutdown line without "shutdown requested" also broke blackbox test_43.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-08-09 - SCHEMA-qualified scripts + per-tick log + shutdown wording

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1299"
cfg.GROUP_NAME = "Orchestrators"
cfg.SCRIPT_NAME = "Orchestrator"
-- ----------------------------------------------------------------------------
-- Forward: replace Orchestrator body (idempotent UPDATE)
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
            ]==],
                updated_at = ${NOW},
                summary = '500 Courses Orchestrator: EnsureCanvasUser every 60s (schema-safe scripts)'
            WHERE group_name = 'Orchestrators'
              AND script_name = 'Orchestrator';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Fix Orchestrator scripts schema + tick markers'                    AS name,
        [=[
            # Forward Migration ${MIGRATION}: Orchestrator 1289 follow-up

            Replaces `Orchestrators.Orchestrator` body so EnsureCanvasUser
            source is loaded from `${SCHEMA}scripts` (not hardcoded lithium),
            ticks every iteration (test_43 lifecycle), and logs
            "shutdown requested" on exit.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Reverse: type flip only (body owned by forward; same pattern as 1290)
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
            -- No schema/script row of its own; body owned by 1289/1299 forward.
            -- Still MUST flip applied→forward so TestMigration APPLY watermark drops.
            SELECT 1 ${DUMMY_TABLE};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Reverse Orchestrator 1299 marker only'                             AS name,
        [=[
            # Reverse Migration ${MIGRATION}: type flip only

            1299 only replaced the Orchestrator script body. Reverse does not
            restore the pre-1299 body (re-APPLY 1289 for a full remove). It only
            marks this migration un-applied so the chain can reverse past 1299.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
