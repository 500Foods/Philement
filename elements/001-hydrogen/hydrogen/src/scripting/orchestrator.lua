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
--   2. Every TICK_MS, lists active scripts whose next_run is due
--      (in real deployments this would be QueryRef #088; in this
--      reference it just lists the local scoreboard and submits
--      a sample "tick" job when the queue is idle, to exercise
--      the worker pool end-to-end).
--   3. Exits cleanly when H.shutdown_requested() is true.

local TICK_MS = 1000

H.log.info("Orchestrator: started")

local iterations = 0
while not H.shutdown_requested() do
    iterations = iterations + 1
    local jobs = H.scoreboard.list()
    H.log.info("Orchestrator: tick %d, %d job(s) in scoreboard",
                iterations, #jobs)

    if #jobs == 0 then
        local id = H.scoreboard.submit({
            script_name = "orchestrator.tick",
            source = "H.set_current_state('orchestrator tick ' .. tostring(os.time()))\n"
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
