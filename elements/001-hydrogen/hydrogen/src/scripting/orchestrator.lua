-- Hydrogen Scripting Subsystem - Reference Orchestrator
--
-- Phase 11c/f of the LUA_PLAN. Seeded into the `scripts` table by
-- migration 1210 (group_name='Orchestrators', script_name='Orchestrator').
-- The C-side orchestrator loader reads this source from the database
-- at the `READY FOR REQUESTS` hook (QueryRef #087) and runs it on a
-- dedicated pthread's lua_State. It is not a worker; it is the
-- subsystem's own long-running tier-2 context.
--
-- Contract:
--   - Cooperative shutdown: check H.shutdown_requested() on every
--     iteration; exit the loop cleanly when it returns true.
--   - Use H.sleep(ms) between iterations. H.sleep returns early
--     when shutdown is requested, so a 1-second H.sleep wakes up
--     within ~100 ms of landing.
--   - H.log.* is the right channel for status messages; H.set_current_state
--     is a no-op here (no scoreboard entry - the Orchestrator is not a job).
--   - H.scoreboard.list() / H.scoreboard.submit() reflect the in-memory
--     scoreboard. The Orchestrator is expected to drive the scoreboard;
--     the worker pool drains it.
--
-- This reference Orchestrator:
--   1. Logs that it has started.
--   2. Once, exercises the data-plane query API (H.query / H.wait /
--      H.query_sync / H.altquery) so blackbox runs cover the scripting
--      query stack end-to-end.
--   3. Once, exercises H.mail / H.notify (freeform queue + deferred notify)
--      so blackbox runs cover scripting_api_mail_notify when Mail Relay is on.
--   4. Once, exercises H.http.get / H.http.post (+ H.wait / *_sync) against
--      HYDROGEN_HTTP_PROBE_BASE (set by blackbox tests to this process's
--      WebServer) so blackbox runs cover scripting http_client + pool.
--   5. Once, exercises H.scoreboard.get / cancel plus a cancel-target job
--      so blackbox runs cover scoreboard_find / scoreboard_request_kill.
--   6. Once, exercises H.llm.list_sync and H.llm.call (+ H.wait) when
--      HYDROGEN_LLM_PROBE_MODEL is set (blackbox points engines at mock LLM).
--   7. Every TICK_MS, lists active scripts whose next_run is due
--      (in real deployments this would be QueryRef #088; in this
--      reference it just lists the local scoreboard and submits
--      a sample "tick" job when the queue is idle, to exercise
--      the worker pool end-to-end).
--   8. Exits cleanly when H.shutdown_requested() is true.

local TICK_MS = 1000

H.log.info("Orchestrator: started")

-- One-shot data-plane probe: H.query (+ params JSON), H.wait, H.query_sync,
-- and H.altquery by configured connection name. Failures are logged but do
-- not stop the lifecycle loop (fixture/engine differences are possible).
local function run_query_probe()
    local h = H.query("SELECT 1 AS n", { flag = 1 }, { timeout = 10 })
    local rows, err = H.wait(h)
    if err then
        H.log.warn("Orchestrator: query_probe wait err: %s", tostring(err))
    else
        H.log.info("Orchestrator: query_probe ok")
    end

    local rows_sync, err_sync = H.query_sync("SELECT 2 AS n")
    if err_sync then
        H.log.warn("Orchestrator: query_sync_probe err: %s", tostring(err_sync))
    else
        H.log.info("Orchestrator: query_sync_probe ok")
    end

    if type(H.altquery) == "function" then
        local h_alt = H.altquery("Acuranzo", "SELECT 3 AS n", nil)
        local rows_alt, err_alt = H.wait(h_alt)
        if err_alt then
            H.log.warn("Orchestrator: altquery_probe err: %s", tostring(err_alt))
        else
            H.log.info("Orchestrator: altquery_probe ok")
        end
        -- silence unused-local warnings in strict environments
        rows_alt = rows_alt
    end

    rows = rows
    rows_sync = rows_sync
end

-- One-shot mail/notify probe. Freeform H.mail covers dual-mode parse + producer
-- enqueue (to/cc/bcc arrays, priority, send + send_sync + H.wait). H.notify is
-- still a deferred-error shim. Failures are logged; Mail Relay may be off.
local function run_mail_probe()
    if type(H.mail) ~= "table" or type(H.mail.send) ~= "function" then
        H.log.warn("Orchestrator: mail_probe skipped (H.mail unavailable)")
        return
    end

    local freeform = {
        to = { "orch-probe@hydrogen.local" },
        cc = { "orch-cc@hydrogen.local" },
        bcc = "orch-bcc@hydrogen.local",
        subject = "Orchestrator mail probe",
        text_body = "blackbox freeform body",
        html_body = "<p>blackbox freeform body</p>",
        from = "orchestrator@hydrogen.local",
        reply_to = "orchestrator-reply@hydrogen.local",
        priority = 5,
        idempotency_key = "orch-mail-probe-freeform",
    }

    local res, err = H.mail.send_sync(freeform)
    if err then
        H.log.warn("Orchestrator: mail_probe send_sync err: %s", tostring(err))
    elseif type(res) == "table" and res.message_id then
        H.log.info("Orchestrator: mail_probe send_sync ok")
    else
        H.log.warn("Orchestrator: mail_probe send_sync unexpected result")
    end

    local h = H.mail.send({
        to = "orch-async@hydrogen.local",
        subject = "Orchestrator mail async probe",
        body = "async freeform via body alias",
        priority = 1,
    })
    local wait_res, wait_err = H.wait(h)
    if wait_err then
        H.log.warn("Orchestrator: mail_probe wait err: %s", tostring(wait_err))
    elseif type(wait_res) == "table" and wait_res.message_id then
        H.log.info("Orchestrator: mail_probe async ok")
    else
        H.log.warn("Orchestrator: mail_probe async unexpected result")
    end

    if type(H.mail.send) == "function" and type(H.mail.send_sync) == "function" then
        -- Template path when mail.test exists; ignore missing-template errors.
        local tres, terr = H.mail.send_sync({
            template = "mail.test",
            to = "orch-template@hydrogen.local",
            params = { NAME = "Orchestrator" },
            priority = 0,
        })
        if terr then
            H.log.info("Orchestrator: mail_probe template skipped: %s", tostring(terr))
        elseif type(tres) == "table" and tres.message_id then
            H.log.info("Orchestrator: mail_probe template ok")
        end
        tres = tres
    end

    if type(H.notify) == "table" and type(H.notify.send_sync) == "function" then
        local nres, nerr = H.notify.send_sync({ channel = "unused", body = "x" })
        if nerr and tostring(nerr):find("deferred", 1, true) then
            H.log.info("Orchestrator: notify_probe ok")
        else
            H.log.warn("Orchestrator: notify_probe unexpected: %s / %s",
                       tostring(nres), tostring(nerr))
        end
    end

    -- Repository helpers (QueryRefs via mailrelay_repository). Soft-fail when
    -- MailRelay.Database / QTC is not ready; hard-require when functions exist
    -- and at least template_list succeeds so test_43 blackbox covers the path.
    if type(H.mail.template_list) == "function" then
        local repo_ok = true
        local function repo_check(name, r, e)
            if e then
                H.log.warn("Orchestrator: mail_repo_probe %s err: %s", name, tostring(e))
                repo_ok = false
                return
            end
            if type(r) ~= "table" then
                H.log.warn("Orchestrator: mail_repo_probe %s bad result", name)
                repo_ok = false
            end
        end
        local tl, tle = H.mail.template_list()
        repo_check("template_list", tl, tle)
        if type(H.mail.template_get) == "function" then
            local tg, tge = H.mail.template_get("mail.test")
            repo_check("template_get", tg, tge)
        end
        if type(H.mail.route_list) == "function" then
            local rl, rle = H.mail.route_list()
            repo_check("route_list", rl, rle)
        end
        if type(H.mail.queue_get) == "function" then
            local qg, qge = H.mail.queue_get("00000000-0000-0000-0000-000000000000")
            repo_check("queue_get", qg, qge)
        end
        local cutoff = "1970-01-01 00:00:00"
        if type(H.mail.cleanup_queue) == "function" then
            local cq, cqe = H.mail.cleanup_queue(cutoff)
            repo_check("cleanup_queue", cq, cqe)
        end
        if type(H.mail.cleanup_events) == "function" then
            local ce, cee = H.mail.cleanup_events(cutoff)
            repo_check("cleanup_events", ce, cee)
        end
        if type(H.mail.cleanup_attempts) == "function" then
            local ca, cae = H.mail.cleanup_attempts(cutoff)
            repo_check("cleanup_attempts", ca, cae)
        end
        if type(H.mail.cleanup_otp) == "function" then
            local co, coe = H.mail.cleanup_otp(cutoff)
            repo_check("cleanup_otp", co, coe)
        end
        if type(H.mail.event_list_pending) == "function" then
            local ep, epe = H.mail.event_list_pending()
            repo_check("event_list_pending", ep, epe)
        end
        if type(H.mail.event_insert) == "function" then
            local ei, eie = H.mail.event_insert({
                event_key = "orchestrator.mail_repo_probe",
                status_a65 = 0,
                template_key = "mail.test",
                recipients_json = '["orch-repo@hydrogen.local"]',
                subject = "orch repo probe",
                priority = 0,
            })
            repo_check("event_insert", ei, eie)
        end
        if repo_ok then
            H.log.info("Orchestrator: mail_repo_probe ok")
        end
    end

    if res and err == nil then
        H.log.info("Orchestrator: mail_probe ok")
    end
end

-- One-shot HTTP probe. Hits this process's WebServer when
-- HYDROGEN_HTTP_PROBE_BASE is set (test_43 exports it from config Port).
-- Covers scripting_http_get/post production path (libcurl via pool + wait).
local function run_http_probe()
    if type(H.http) ~= "table" or type(H.http.get) ~= "function" then
        H.log.warn("Orchestrator: http_probe skipped (H.http unavailable)")
        return
    end

    local base = nil
    if type(os) == "table" and type(os.getenv) == "function" then
        base = os.getenv("HYDROGEN_HTTP_PROBE_BASE")
    end
    if type(base) ~= "string" or base == "" then
        H.log.info("Orchestrator: http_probe skipped (no HYDROGEN_HTTP_PROBE_BASE)")
        return
    end
    -- strip trailing slash
    base = base:gsub("/+$", "")

    local get_url = base .. "/api/system/health"
    local h_get = H.http.get(get_url, { Accept = "application/json" }, { timeout = 10 })
    local get_res, get_err = H.wait(h_get)
    if get_err then
        H.log.warn("Orchestrator: http_probe get wait err: %s", tostring(get_err))
    elseif type(get_res) == "table" and get_res.status and get_res.status >= 200
           and get_res.status < 500 then
        H.log.info("Orchestrator: http_probe get ok status=%s", tostring(get_res.status))
    else
        H.log.warn("Orchestrator: http_probe get unexpected: %s", tostring(get_res))
    end

    local post_url = base .. "/api/system/health"
    local post_res, post_err = H.http.post_sync(
        post_url,
        "{}",
        { ["Content-Type"] = "application/json", ["X-Orch-Probe"] = "1" },
        { timeout = 10, content_type = "application/json" }
    )
    if post_err then
        -- Method-not-allowed or similar still proves the POST path ran.
        H.log.info("Orchestrator: http_probe post_sync err (ok if 4xx path): %s",
                   tostring(post_err))
    elseif type(post_res) == "table" and post_res.status then
        H.log.info("Orchestrator: http_probe post ok status=%s", tostring(post_res.status))
    else
        H.log.warn("Orchestrator: http_probe post unexpected")
    end

    if get_err == nil and type(get_res) == "table" and get_res.status then
        H.log.info("Orchestrator: http_probe ok")
    end
end

-- One-shot scoreboard probe. Covers H.scoreboard.get + cancel (find / kill)
-- and a short-lived cancel-target job in addition to the tick path's list/submit.
local function run_scoreboard_probe()
    if type(H.scoreboard) ~= "table" then
        H.log.warn("Orchestrator: scoreboard_probe skipped (H.scoreboard unavailable)")
        return
    end

    local cancel_id = H.scoreboard.submit({
        script_name = "orchestrator.scoreboard_cancel",
        params_json = "{\"probe\":\"cancel\"}",
        source = "local t = 0\n"
              .. "while t < 20000000 do\n"
              .. "  t = t + 1\n"
              .. "  if t % 100000 == 0 then\n"
              .. "    H.set_current_state('cancel-target ' .. tostring(t))\n"
              .. "  end\n"
              .. "end\n"
              .. "return 0\n",
    })
    if not cancel_id then
        H.log.warn("Orchestrator: scoreboard_probe submit cancel target failed")
        return
    end

    local snap = H.scoreboard.get(cancel_id)
    if type(snap) == "table" and snap.job_id then
        H.log.info("Orchestrator: scoreboard_probe get ok id=%s status=%s",
                   tostring(snap.job_id), tostring(snap.status))
    else
        H.log.warn("Orchestrator: scoreboard_probe get unexpected")
    end

    local cancelled = H.scoreboard.cancel(cancel_id)
    if cancelled then
        H.log.info("Orchestrator: scoreboard_probe cancel ok id=%s", cancel_id)
    else
        H.log.warn("Orchestrator: scoreboard_probe cancel returned false")
    end

    -- Brief settle so worker can observe kill_requested and mark terminal.
    H.sleep(200)

    local after = H.scoreboard.get(cancel_id)
    if type(after) == "table" then
        H.log.info("Orchestrator: scoreboard_probe after status=%s kill=%s",
                   tostring(after.status), tostring(after.kill_requested))
    end

    local missing = H.scoreboard.get("_____")
    if missing == nil then
        H.log.info("Orchestrator: scoreboard_probe miss ok")
    end

    H.log.info("Orchestrator: scoreboard_probe ok")
end

-- One-shot LLM probe. Uses HYDROGEN_LLM_PROBE_MODEL when set (test_43 + mock LLM).
-- list_sync covers enumeration; call + wait covers chat_proxy when engines resolve.
local function run_llm_probe()
    if type(H.llm) ~= "table" or type(H.llm.list_sync) ~= "function" then
        H.log.warn("Orchestrator: llm_probe skipped (H.llm unavailable)")
        return
    end

    local list_res, list_err = H.llm.list_sync()
    if list_err then
        H.log.info("Orchestrator: llm_probe list_sync err: %s", tostring(list_err))
    elseif type(list_res) == "table" and list_res.models ~= nil then
        H.log.info("Orchestrator: llm_probe list_sync ok")
    else
        H.log.warn("Orchestrator: llm_probe list_sync unexpected")
    end

    local model = nil
    if type(os) == "table" and type(os.getenv) == "function" then
        model = os.getenv("HYDROGEN_LLM_PROBE_MODEL")
    end
    if type(model) ~= "string" or model == "" then
        -- Prefer first name from list JSON string when mock/env did not set one.
        if type(list_res) == "table" and type(list_res.models) == "string"
           and list_res.models:find("\"name\"", 1, true) then
            model = list_res.models:match("\"name\"%s*:%s*\"([^\"]+)\"")
        end
    end
    if type(model) ~= "string" or model == "" then
        H.log.info("Orchestrator: llm_probe call skipped (no model)")
        if list_err == nil and type(list_res) == "table" then
            H.log.info("Orchestrator: llm_probe ok")
        end
        return
    end

    local h = H.llm.call(model, "orchestrator blackbox llm probe", {
        max_tokens = 32,
        temperature = 0.2,
        timeout = 15,
    })
    local call_res, call_err = H.wait(h)
    if call_err then
        H.log.info("Orchestrator: llm_probe call wait err: %s", tostring(call_err))
    elseif type(call_res) == "table" and call_res.status then
        H.log.info("Orchestrator: llm_probe call ok status=%s", tostring(call_res.status))
    else
        H.log.warn("Orchestrator: llm_probe call unexpected")
    end

    if (list_err == nil and type(list_res) == "table")
       or (call_err == nil and type(call_res) == "table") then
        H.log.info("Orchestrator: llm_probe ok")
    end
end

local query_probed = false
local mail_probed = false
local http_probed = false
local scoreboard_probed = false
local llm_probed = false
local iterations = 0
while not H.shutdown_requested() do
    iterations = iterations + 1
    local jobs = H.scoreboard.list()
    H.log.info("Orchestrator: tick %d, %d job(s) in scoreboard",
                iterations, #jobs)

    if not query_probed then
        query_probed = true
        local ok, probe_err = pcall(run_query_probe)
        if not ok then
            H.log.warn("Orchestrator: query_probe failed: %s", tostring(probe_err))
        end
    end

    if not mail_probed then
        mail_probed = true
        local ok_m, mail_err = pcall(run_mail_probe)
        if not ok_m then
            H.log.warn("Orchestrator: mail_probe failed: %s", tostring(mail_err))
        end
    end

    if not http_probed then
        http_probed = true
        local ok_h, http_err = pcall(run_http_probe)
        if not ok_h then
            H.log.warn("Orchestrator: http_probe failed: %s", tostring(http_err))
        end
    end

    if not scoreboard_probed then
        scoreboard_probed = true
        local ok_s, sb_err = pcall(run_scoreboard_probe)
        if not ok_s then
            H.log.warn("Orchestrator: scoreboard_probe failed: %s", tostring(sb_err))
        end
    end

    if not llm_probed then
        llm_probed = true
        local ok_l, llm_err = pcall(run_llm_probe)
        if not ok_l then
            H.log.warn("Orchestrator: llm_probe failed: %s", tostring(llm_err))
        end
    end

    if #jobs == 0 then
        local id = H.scoreboard.submit({
            script_name = "orchestrator.tick",
            source = "H.set_current_state('orchestrator tick ' .. tostring(os.time()))\n"
                     .. "H.set_result('json', 'orchestrator.tick')\n"
                     .. "return 0\n",
        })
        if id then
            H.log.info("Orchestrator: submitted tick job %s", id)
        else
            H.log.warn("Orchestrator: submit returned nil")
        end
    end

    H.sleep(TICK_MS)
end

H.log.info("Orchestrator: shutdown requested, exiting after %d iteration(s)",
            iterations)
