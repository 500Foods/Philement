-- schemahelper_actions.lua
-- Finding actions and queue lifecycle: rebuild/ingest/reaudit, plus the
-- apply / generate-packet / promote operations invoked from the review loop.
-- Depends on queue, packet, apply, connect, wrappers, invoke. The few
-- helpers owned by the orchestrator (show_mode, read_key, the two inline
-- input loops) are injected via init() to avoid a circular require.
--
-- CHANGELOG
-- 0.5.8 - 2026-08-25 - Use work_dir for JSON/detail reads; out_dir for .mig/state

local W = require("schemahelper_wrappers")
local Q = require("schemahelper_queue")
local packet = require("schemahelper_packet")
local apply = require("schemahelper_apply")
local connect = require("schemahelper_connect")
local I = require("schemahelper_invoke")
local C = require("schemahelper_const")

local M = {}

local ctx = {}

function M.init(host)
    -- host provides: show_mode(screen, app, mode), read_key(screen, app),
    -- run_apply_confirm(screen, app), run_note(screen, app, opts)
    ctx.show_mode = host.show_mode
    ctx.read_key = host.read_key
    ctx.run_apply_confirm = host.run_apply_confirm
    ctx.run_note = host.run_note
end

local function rebuild_queue(app, opts)
    app.state = Q.load_state(opts.state_file)
    app.built = Q.build({
        out_dir = opts.work_dir,
        track = opts.track,
        state = app.state,
    })
    local n = app.built.totals.subject
    if n < 1 then
        app.review_index = 1
    elseif app.review_index > n then
        app.review_index = n
    end
end

local function ingest_audit(app, opts, ran, exit_code)
    local state_fh = io.open(opts.state_file, "r")
    if state_fh then
        state_fh:close()
    else
        Q.create_state(opts.state_file, opts.design, opts.engine, opts.schema)
    end
    rebuild_queue(app, opts)
    app.catalog_degraded = false
    local ok_exit = (exit_code == 0 or exit_code == 2 or exit_code == 3)
    local have_meta = Q.artifacts_present(opts.work_dir, "metadata")
    local have_cat = Q.artifacts_present(opts.work_dir, "catalog")
    if ran and not ok_exit then
        if opts.track == "catalog" and not have_cat then
            return "SchemaTool failed; see log"
        elseif have_meta then
            app.catalog_degraded = true
            app.show_mode_msg = "Catalog track failed; metadata findings kept"
            return nil
        end
        return "SchemaTool failed; see log"
    end
    if ran and ok_exit and (opts.track == "both" or opts.track == "catalog")
        and not have_cat and have_meta then
        app.catalog_degraded = true
        app.show_mode_msg = "Catalog track failed; metadata findings kept"
    end
    return nil
end

local function reaudit(screen, app, opts)
    app.show_mode_msg = ""
    app.status_note = "Connecting…"
    ctx.show_mode(screen, app, "running")
    app.conn = I.probe_connect(opts)
    ctx.show_mode(screen, app, "running")
    if not app.conn.ok then
        app.show_mode_msg = "Re-audit skipped — no database connection"
        rebuild_queue(app, opts)
        return false
    end
    app.status_note = "Running SchemaTool…"
    ctx.show_mode(screen, app, "running")
    local exit_code, log = I.invoke_schematool(opts, screen, app, ctx.show_mode)
    app.log = log
    local err = ingest_audit(app, opts, true, exit_code)
    if err then
        app.show_mode_msg = err
        return false
    end
    if not app.catalog_degraded then
        app.show_mode_msg = "re-audited"
    end
    return true
end

local function apply_finding(screen, app, opts)
    if not app.built or not app.built.subject then
        app.show_mode_msg = "error: no finding selected"
        return nil
    end
    local f = app.built.subject[app.review_index]
    if not f then
        app.show_mode_msg = "error: no finding selected"
        return nil
    end
    local why = apply.refuse_reason(f, opts.allow_write)
    if why then
        app.show_mode_msg = "update disabled — " .. why
        return nil
    end
    local token = apply.confirm_token(f)
    local conn = connect.resolve(opts.wrapper)
    local sql, sql_err = apply.build_sql(f, conn, opts.work_dir)
    if not sql then
        app.show_mode_msg = "error: " .. tostring(sql_err)
        return nil
    end
    app.apply_finding = f
    app.apply_token = token
    app.apply_sql = sql
    app.apply_buf = ""
    local typed = ctx.run_apply_confirm(screen, app)
    app.apply_finding = nil
    app.apply_token = nil
    app.apply_sql = nil
    if typed == nil then
        app.show_mode_msg = "update cancelled"
        return nil
    end
    if typed ~= token then
        app.show_mode_msg = "update aborted — type " .. token
        return nil
    end
    local log_path, log_err = apply.write_log(opts.work_dir, f, sql)
    if not log_path then
        app.show_mode_msg = "error: " .. tostring(log_err)
        return nil
    end
    local ok, exec_err = connect.exec_sql(opts.wrapper, log_path)
    if not ok then
        app.show_mode_msg = "update failed: " .. tostring(exec_err)
        return nil
    end
    local saved, save_err = Q.save_decision(
        opts.state_file, f.id, "applied", {
            note = token,
        })
    if not saved then
        app.show_mode_msg = "updated but sidecar: " .. tostring(save_err)
        return nil
    end
    rebuild_queue(app, opts)
    if f.kind == "orphan" then
        app.show_mode_msg = "deleted orphan ref " .. token
            .. " from queries (review .mig for migration capture)"
    elseif f.class and f.class:find("^catalog") then
        app.show_mode_msg = "applied DDL to " .. token
            .. " — catalog shape changed"
    else
        app.show_mode_msg = "updated " .. token
            .. " — metadata only, does not replay DDL"
    end
    if app.built.totals.subject < 1 then
        return "dashboard"
    end
    return nil
end

local function packet_next(opts)
    local ref, scan = packet.next_ref({
        migrations = opts.migrations,
        packet_dir = opts.packet_dir,
        design = opts.design,
        engine = opts.engine,
        ref = opts.ref,
    })
    local collide = packet.collision({
        migrations = opts.migrations,
        packet_dir = opts.packet_dir,
        design = opts.design,
        engine = opts.engine,
        ref = opts.ref,
    }, ref)
    if collide then
        return ref, collide
    end
    return ref, nil, scan
end

local function generate_packet(screen, app, opts)
    if not app.built or not app.built.subject then
        app.show_mode_msg = "error: no finding selected"
        return nil
    end
    local f = app.built.subject[app.review_index]
    if not f then
        app.show_mode_msg = "error: no finding selected"
        return nil
    end
    local next_ref, why = packet_next(opts)
    if why then
        app.show_mode_msg = why
        return nil
    end
    app.note_finding = f
    app.note_ref = next_ref
    app.note_buf = ""
    local note = ctx.run_note(screen, app, opts)
    app.note_finding = nil
    if note == nil then
        app.show_mode_msg = "packet cancelled"
        return nil
    end
    local tool_ver = select(1, W.read_tool_version(opts.schematool))
    local detail_lines = Q.load_detail_section(opts.work_dir, f)
    local detail_text
    if #detail_lines > 0 then
        detail_text = table.concat(detail_lines, "\n") .. "\n"
    end
    local written, err = packet.write({
        migrations = opts.migrations,
        packet_dir = opts.packet_dir,
        out_dir = opts.out_dir,
        design = opts.design,
        engine = opts.engine,
        schema = opts.schema,
        ref = opts.ref,
        schemahelper_version = C.VERSION,
        schematool_version = tool_ver or "",
    }, f, {
        note = note,
        detail_text = detail_text,
    })
    if not written then
        app.show_mode_msg = "error: " .. tostring(err)
        return nil
    end
    local ok, save_err = Q.save_decision(
        opts.state_file, f.id, "packet", {
            ref = written.ref,
            packet = written.name,
            note = note,
        })
    if not ok then
        app.show_mode_msg = "error: " .. tostring(save_err)
        return nil
    end
    rebuild_queue(app, opts)
    app.show_mode_msg = string.format("packet %d  %s", written.ref, written.name)
    if app.built.totals.subject < 1 then
        return "dashboard"
    end
    return nil
end

local function promote_finding(_screen, app, opts)
    if not app.built or not app.built.subject then
        app.show_mode_msg = "error: no finding selected"
        return nil
    end
    local f = app.built.subject[app.review_index]
    if not f then
        app.show_mode_msg = "error: no finding selected"
        return nil
    end
    local tool_ver = select(1, W.read_tool_version(opts.schematool))
    local dest_path, name, ref = packet.promote({
        migrations = opts.migrations,
        packet_dir = opts.packet_dir,
        design = opts.design,
        engine = opts.engine,
        state = app.state,
        schemahelper_version = C.VERSION,
        schematool_version = tool_ver or "",
    }, f)
    if not dest_path then
        app.show_mode_msg = "error: " .. tostring(name)
        return nil
    end
    app.show_mode_msg = string.format(
        "promoted packet %d → %s/%s", ref, opts.migrations, name)
    return nil
end

return {
    init = M.init,
    rebuild_queue = rebuild_queue,
    ingest_audit = ingest_audit,
    reaudit = reaudit,
    apply_finding = apply_finding,
    packet_next = packet_next,
    generate_packet = generate_packet,
    promote_finding = promote_finding,
}
