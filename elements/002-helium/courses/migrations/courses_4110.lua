-- Migration: courses_4110.lua
-- Seed Build.ReceiveSuggested (invokable)
--
-- One receipt attempt for each suggestion that is still Suggested
-- and has no receipt_mail value. The row becomes received either
-- way. sent or failed is stored on receipt_mail. A later call
-- skips a row whose receipt_mail is already set, and skips Intake.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-23 - Seed Build.ReceiveSuggested

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "4110"
cfg.GROUP_NAME = "Build"
cfg.SCRIPT_NAME = "ReceiveSuggested"
if engine == "mysql" or engine == "mariadb" then
    cfg.CAST_INTEGER = "signed"
else
    cfg.CAST_INTEGER = cfg.INTEGER
end
if engine == "db2" then
    cfg.FROM_ONE = "FROM SYSIBM.SYSDUMMY1"
elseif engine == "firebird" then
    cfg.FROM_ONE = "FROM RDB$DATABASE"
else
    cfg.FROM_ONE = ""
end
-- ----------------------------------------------------------------------------
-- Forward: seed Build.ReceiveSuggested (invokable)
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
-- Build.ReceiveSuggested
-- One receipt attempt per Suggested row with a null receipt_mail.
-- Failure still stores received. Intake is not selected.

local TEMPLATE = "course.suggestion_received"

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

local function fail(code, message)
    H.set_result_json({ ok = false, code = code, message = message or code })
    return 0
end

local function positive(value)
    local n = tonumber(value)
    if not n or n < 1 or n ~= math.floor(n) then return nil end
    return n
end

local function text(value)
    if value == nil then return "" end
    return tostring(value)
end

local function nzi(value)
    local n = tonumber(value)
    if not n or n == 0 then return 0 end
    return n
end

local function staff_gate()
    local loaded, gate = pcall(require, "Enroll.StaffGate")
    if not loaded or type(gate) ~= "table" or type(gate.allow) ~= "function" then
        return fail("not_staff", "Staff check is unavailable")
    end
    local allowed, why = gate.allow(params)
    if allowed then return nil end
    if why == "lookup_error" then
        return fail("lookup_error", "Could not resolve staff access")
    end
    if why == "missing_identity" then
        return fail("missing_identity", "Sign in required")
    end
    return fail("not_staff", "Staff access is required")
end

local function claim_update(id, user_id)
    local res, err = H.query_sync([[
        UPDATE ${SCHEMA}course_build_runs
           SET state = 'received',
               receipt_mail = 'failed',
               updated_id = CAST(:USERID AS ${CAST_INTEGER}),
               updated_at = NOW()
         WHERE suggestion_id = CAST(:SUGGESTION_ID AS ${CAST_INTEGER})
           AND state = 'suggested'
           AND receipt_mail IS NULL
    ]], { SUGGESTION_ID = id, USERID = user_id })
    if err or type(res) ~= "table" then return false end
    return res.affected_rows == 1
end

local function claim_insert(id, user_id)
    local res, err = H.query_sync([[
        INSERT INTO ${SCHEMA}course_build_runs (
            run_id, suggestion_id, working_title, state, receipt_mail, flagged,
            annotation, valid_after, valid_until,
            created_id, created_at, updated_id, updated_at
        )
        SELECT
            COALESCE((SELECT MAX(run_id) FROM ${SCHEMA}course_build_runs), 0) + 1,
            s.suggestion_id,
            s.course_name,
            'received',
            'failed',
            0,
            NULL,
            NULL,
            NULL,
            src.user_id,
            NOW(),
            src.user_id,
            NOW()
        FROM (
            SELECT CAST(:SUGGESTION_ID AS ${CAST_INTEGER}) AS suggestion_id,
                   CAST(:USERID AS ${CAST_INTEGER}) AS user_id
            ${FROM_ONE}
        ) src
        JOIN ${SCHEMA}course_suggestions s
          ON s.suggestion_id = src.suggestion_id
        WHERE NOT EXISTS (
            SELECT 1
              FROM ${SCHEMA}course_build_runs existing
             WHERE existing.suggestion_id = src.suggestion_id
        )
    ]], { SUGGESTION_ID = id, USERID = user_id })
    if err or type(res) ~= "table" then return false end
    return res.affected_rows == 1
end

local function mark_sent(id)
    H.query_sync([[
        UPDATE ${SCHEMA}course_build_runs
           SET receipt_mail = 'sent',
               updated_at = NOW()
         WHERE suggestion_id = CAST(:SUGGESTION_ID AS ${CAST_INTEGER})
           AND state = 'received'
           AND receipt_mail = 'failed'
    ]], { SUGGESTION_ID = id })
end

local function write_ledger(id, user_id)
    H.query_sync([[
        INSERT INTO ${SCHEMA}course_build_actions (
            action_id, run_id, actor_account_id, action_name,
            human_cost_cents, token_cost_cents, elapsed_ms, prompt_count,
            prompt, result_ref, valid_after, valid_until,
            created_id, created_at, updated_id, updated_at
        )
        SELECT
            COALESCE((SELECT MAX(action_id) FROM ${SCHEMA}course_build_actions), 0) + 1,
            run.run_id,
            src.user_id,
            'received',
            NULL, NULL, NULL, NULL, NULL, NULL,
            NULL, NULL,
            src.user_id, NOW(), src.user_id, NOW()
        FROM (
            SELECT CAST(:SUGGESTION_ID AS ${CAST_INTEGER}) AS suggestion_id,
                   CAST(:USERID AS ${CAST_INTEGER}) AS user_id
            ${FROM_ONE}
        ) src
        JOIN ${SCHEMA}course_build_runs run
          ON run.suggestion_id = src.suggestion_id
    ]], { SUGGESTION_ID = id, USERID = user_id })
end

local function send_receipt(row, id)
    local email = text(pick(row, "submitter_email", "SUBMITTER_EMAIL"))
    if not email:find("@", 1, true) then return false end
    if type(H) ~= "table" or type(H.mail) ~= "table" or type(H.mail.send_sync) ~= "function" then
        return false
    end
    local res, err = H.mail.send_sync({
        template = TEMPLATE,
        to = email,
        params = {
            FIRST_NAME = text(pick(row, "submitter_name", "SUBMITTER_NAME")),
            COURSE_NAME = text(pick(row, "course_name", "COURSE_NAME")),
        },
        idempotency_key = "suggestion-receipt-" .. tostring(id),
    })
    if err or type(res) ~= "table" then return false end
    return res.status == "queued" or res.message_id ~= nil
end

local function receive_one(row, user_id)
    local id = positive(pick(row, "suggestion_id", "SUGGESTION_ID"))
    if not id then return nil end
    local claimed
    if nzi(pick(row, "run_id", "RUN_ID")) > 0 then
        claimed = claim_update(id, user_id)
    else
        claimed = claim_insert(id, user_id)
    end
    if not claimed then return nil end
    local sent = send_receipt(row, id)
    if sent then mark_sent(id) end
    write_ledger(id, user_id)
    return sent
end

if type(params) ~= "table" then
    return fail("validation", "Missing params")
end

local denied = staff_gate()
if denied then return denied end

local actor = tonumber(params.user_id)
if not actor or actor < 0 or actor ~= math.floor(actor) then actor = 0 end

local rows, qerr = H.query_sync([[
    SELECT s.suggestion_id, s.course_name, s.submitter_name, s.submitter_email,
           r.run_id
    FROM ${SCHEMA}course_suggestions s
    LEFT JOIN ${SCHEMA}course_build_runs r
      ON r.suggestion_id = s.suggestion_id
    WHERE r.run_id IS NULL
       OR (r.state = 'suggested' AND r.receipt_mail IS NULL)
]])
if qerr then
    return fail("lookup_error", "Could not list suggestions")
end

local received, sent, failed = 0, 0, 0
local list = qrows(rows) or {}
for _, row in ipairs(list) do
    local outcome = receive_one(row, actor)
    if outcome ~= nil then
        received = received + 1
        if outcome then sent = sent + 1 else failed = failed + 1 end
    end
end

H.set_result_json({
    ok = true,
    received = received,
    mail_sent = sent,
    mail_failed = failed,
})
return 0
                ]==],
                'Marks Suggested rows received and attempts one receipt email',
                1,
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Build.ReceiveSuggested invokable script'                      AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Build.ReceiveSuggested

            Inserts `Build.ReceiveSuggested` with invokable = 1.
            Staff only. A null receipt_mail is the not-attempted
            state. The attempt writes sent or failed and sets state
            to received. Intake rows are not selected.
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
            WHERE group_name = '${GROUP_NAME}'
              AND script_name = '${SCRIPT_NAME}';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Build.ReceiveSuggested'                                     AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Build.ReceiveSuggested

            Deletes that script row. Draft rows stay.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})
-- ----------------------------------------------------------------------------
return queries end
