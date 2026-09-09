-- schemahelper_invoke.lua
-- SchemaTool invocation, progress parsing, connect probe text, and the
-- post-run result-line builder.
--
-- CHANGELOG
-- 0.6.4 - 2026-09-08 - Dark-grey progress bar background
-- 0.6.3 - 2026-09-08 - Eighths bar, compare/catalog parse, issue scroll
-- 0.6.1 - 2026-09-08 - Instance rows are label + value + two attrs
-- 0.6.0 - 2026-09-08 - Instance block replaces session header
-- 0.5.8 - 2026-08-25 - Pass --work-dir to SchemaTool; log/exit files use work_dir

local C = require("schemahelper_const")
local W = require("schemahelper_wrappers")
local connect = require("schemahelper_connect")
local UI = require("schemahelper_ui")
local Mouse = require("schemahelper_mouse")

local ATTR = C.ATTR

local EIGHTHS = { "▏", "▎", "▍", "▌", "▋", "▊", "▉", "█" }
local CLASS_RANK = { drift = 1, catalog = 2, missing = 3, anomaly = 4 }
local CLASS_ATTR = {
    drift = ATTR.SECTION,
    catalog = ATTR.RUNTIME,
    missing = ATTR.DATE,
    anomaly = ATTR.ERR,
}

local function bar_attr(base)
    return {
        fg = base and base.fg,
        brightness = base and base.brightness,
        bg = C.ATTR.BARBG,
    }
end

local function read_tail_bytes(path, nbytes)
    local f = io.open(path, "rb")
    if not f then
        return ""
    end
    local size = f:seek("end")
    if not size then
        f:close()
        return ""
    end
    local start = math.max(0, size - nbytes)
    f:seek("set", start)
    local data = f:read("*a") or ""
    f:close()
    return data
end

local function ui_issue_class(raw)
    if raw == "ok" or not raw or raw == "" then
        return nil
    end
    if raw == "anomaly" then
        return "anomaly"
    end
    if raw == "drift" then
        return "drift"
    end
    if raw == "missing_load" or raw == "missing_apply" or raw == "orphan" then
        return "missing"
    end
    if raw == "catalog" or raw == "missing_table" or raw == "missing_column"
        or raw == "nullability" or raw == "extra_table" or raw == "extra_column" then
        return "catalog"
    end
    return "drift"
end

local function tint_cell(prog, ref, ui)
    if not ui or not ref then
        return
    end
    local idx = prog.ref_at[ref]
    if not idx then
        return
    end
    local cell = math.floor((idx - 1) / 8) + 1
    local prev = prog.cell_issue[cell]
    if not prev or (CLASS_RANK[ui] or 0) > (CLASS_RANK[prev] or 0) then
        prog.cell_issue[cell] = ui
    end
end

local function parse_schematool_progress(log, fallback_total)
    local prog = {
        phase = "starting",
        current = 0,
        total = fallback_total or 0,
        ref = nil,
        name = nil,
        issues = {},
        cell_issue = {},
        ref_at = {},
        compare_current = 0,
    }
    local text = read_tail_bytes(log, 262144)
    for line in (text .. "\n"):gmatch("([^\n]*)\n") do
        local phase = line:match("^phase: (%S+)")
        if phase then
            prog.phase = phase
        end
        local cur, tot, ref, name = line:match("^expect (%d+)/(%d+) ref (%d+) name=(.*)")
        if not cur then
            cur, tot, ref = line:match("^expect (%d+)/(%d+) ref (%d+)")
        end
        if cur then
            prog.phase = "expect"
            prog.current = tonumber(cur) or 0
            prog.total = tonumber(tot) or prog.total
            prog.ref = tonumber(ref)
            if name then
                prog.name = name
            end
            if prog.ref then
                prog.ref_at[prog.ref] = prog.current
            end
        end
        local ccur, ctot, cref, cclass = line:match("^compare (%d+)/(%d+) ref (%d+) (%S+)")
        if ccur then
            prog.phase = "compare"
            if prog.total > 0 then
                prog.current = prog.total
            end
            prog.compare_current = tonumber(ccur) or 0
            local ntot = tonumber(ctot)
            if ntot and ntot > 0 and (not fallback_total or fallback_total < 1) then
                prog.total = ntot
            end
            prog.ref = tonumber(cref)
            local ui = ui_issue_class(cclass)
            if ui then
                prog.issues[#prog.issues + 1] = {
                    ref = prog.ref,
                    class = ui,
                    raw = cclass,
                }
                tint_cell(prog, prog.ref, ui)
            end
        end
        local catref, catclass = line:match("^catalog ref (%d+) (%S+)")
        if not catref then
            catref, catclass = line:match("^catalog %d+/%d+ ref (%d+) (%S+)")
        end
        if catref then
            local ui = ui_issue_class(catclass) or "catalog"
            local r = tonumber(catref)
            prog.issues[#prog.issues + 1] = {
                ref = r,
                class = ui,
                raw = catclass,
            }
            tint_cell(prog, r, ui)
        end
        if line:match("^Compare:") then
            prog.phase = "compare"
            if prog.total > 0 then
                prog.current = prog.total
            end
        end
        if line:match("^SQL:") then
            prog.phase = "remediate"
        end
    end
    return prog
end

local function eighths_width(total)
    total = tonumber(total) or 0
    if total < 1 then
        return 0
    end
    return math.ceil(total / 8)
end

local function progress_eighths(prog)
    prog = prog or {}
    local total = prog.total or 0
    local current = prog.current or 0
    local n = eighths_width(total)
    local cells = {}
    for i = 1, n do
        local start = (i - 1) * 8
        local cap = math.min(8, total - start)
        local filled = current - start
        if filled < 0 then
            filled = 0
        end
        if filled > cap then
            filled = cap
        end
        local ch = " "
        if filled > 0 then
            ch = EIGHTHS[filled]
        end
        local ui = prog.cell_issue and prog.cell_issue[i]
        local attr = ATTR.VERSION
        if ui then
            attr = CLASS_ATTR[ui] or ATTR.ERR
        elseif filled == 0 then
            attr = ATTR.PATH
        end
        cells[i] = { ch = ch, attr = bar_attr(attr), class = ui }
    end
    return cells
end

local function progress_bar_spans(prog)
    local spans = {}
    local cells = progress_eighths(prog)
    for i = 1, #cells do
        spans[#spans + 1] = { cells[i].ch, cells[i].attr }
    end
    local total = prog and prog.total or 0
    local current = prog and prog.current or 0
    if total > 0 then
        local pct = math.floor(100 * current / total)
        spans[#spans + 1] = { string.format("  %d%%", pct), ATTR.SUB }
    end
    return spans
end

local function progress_bar(_width, current, total)
    local spans = progress_bar_spans({
        current = current,
        total = total,
        cell_issue = {},
    })
    local buf = {}
    for i = 1, #spans do
        buf[#buf + 1] = spans[i][1]
    end
    return table.concat(buf)
end

local function apply_issue_scroll(app, delta)
    local prog = app and app.progress
    if not prog then
        return
    end
    local n = #(prog.issues or {})
    local vis = prog.issue_vis or 8
    local max_off = math.max(0, n - vis)
    local s = (prog.issue_scroll or 0) + (delta or 0)
    if s < 0 then
        s = 0
    end
    if s > max_off then
        s = max_off
    end
    prog.issue_scroll = s
end

local function handle_run_input(raw, app)
    if not raw then
        return
    end
    local d = Mouse.wheel_delta(raw)
    if d then
        apply_issue_scroll(app, d)
        return
    end
    if raw == "k" or raw == "K" then
        apply_issue_scroll(app, 1)
    elseif raw == "j" or raw == "J" then
        apply_issue_scroll(app, -1)
    end
end

local function invoke_schematool(opts, screen, app, show_mode_fn)
    local log = opts.work_dir .. "/" .. C.INSTANCE_LOG
    local exitf = opts.work_dir .. "/schemahelper_schematool.exit"
    local parts = {
        W.sh_quote(opts.wrapper),
        "--format",
        "json",
        "--out-dir",
        W.sh_quote(opts.out_dir),
        "--work-dir",
        W.sh_quote(opts.work_dir),
        "--keep-work-dir",
        "--no-detail",
    }
    if opts.track == "catalog" or opts.track == "both" then
        parts[#parts + 1] = "--catalog"
    end
    if opts.migrations ~= "" then
        parts[#parts + 1] = "--migrations"
        parts[#parts + 1] = W.sh_quote(opts.migrations)
    end
    local wipe = io.open(log, "w")
    if wipe then
        wipe:close()
    end
    os.remove(exitf)
    local total = W.count_disk_refs(opts.migrations, opts.design)
    app.progress = {
        phase = "starting",
        current = 0,
        total = total,
        ref = nil,
        issues = {},
        cell_issue = {},
        issue_scroll = 0,
        issue_vis = 8,
    }
    local cmd = "(" .. table.concat(parts, " ") .. " > " .. W.sh_quote(log)
        .. " 2>&1; echo $? > " .. W.sh_quote(exitf) .. ") &"
    os.execute(cmd)
    while true do
        local raw = C.t.input.readansi(0.2)
        if raw == nil then
            screen:check_resize(true)
        else
            handle_run_input(raw, app)
        end
        local scroll = app.progress and app.progress.issue_scroll or 0
        local vis = app.progress and app.progress.issue_vis or 8
        app.progress = parse_schematool_progress(log, total)
        app.progress.issue_scroll = scroll
        app.progress.issue_vis = vis
        apply_issue_scroll(app, 0)
        show_mode_fn(screen, app, "running")
        local ef = io.open(exitf, "r")
        if ef then
            local code = tonumber((ef:read("*l") or ""):match("%d+")) or 1
            ef:close()
            os.remove(exitf)
            if app.progress.total > 0 then
                app.progress.current = app.progress.total
            end
            app.progress.phase = "done"
            show_mode_fn(screen, app, "running")
            return code, log
        end
    end
end

local function probe_connect(opts)
    return connect.probe(opts.wrapper)
end

local function basename(path)
    if not path or path == "" or path == "(none)" then
        return "(none)"
    end
    return path:match("([^/]+)$") or path
end

local function connect_target(conn)
    if conn.family == "file" or (conn.host == "" and conn.database ~= "") then
        return basename(conn.database)
    end
    local who = conn.user
    if who ~= "" then
        who = who .. "@"
    end
    local target = who .. conn.host
    if conn.port ~= "" then
        target = target .. ":" .. conn.port
    end
    if conn.database ~= "" then
        target = target .. "/" .. conn.database
    end
    if conn.schema ~= "" then
        target = target .. "  schema=" .. conn.schema
    end
    return target
end

local function connect_text(conn)
    if not conn then
        return "checking…", ATTR.DATE
    end
    local target = connect_target(conn)
    local family = conn.family
    if family ~= "" then
        family = family .. "  "
    end
    if conn.ok then
        return "ok    " .. family .. target, ATTR.VERSION
    end
    local detail = conn.detail or "failed"
    if #detail > 48 then
        detail = detail:sub(1, 45) .. "…"
    end
    return "fail  " .. family .. target .. "  (" .. detail .. ")", ATTR.ERR
end

local function picker_wrapper_path(app, opts)
    local list = app.picker and app.picker.list or {}
    local _, my = UI.mouse_hot_get()
    if my then
        for _, h in ipairs(UI.hotspots_get()) do
            if h.key and h.row == my and type(h.key) == "string"
                and h.key:sub(1, 5) == "PICK:" then
                local idx = tonumber(h.key:sub(6))
                if idx and list[idx] then
                    return list[idx].path
                end
            end
        end
    end
    local item = list[app.picker and app.picker.selected or 1]
    if item and item.path then
        return item.path
    end
    return opts.wrapper or ""
end

local function instance_connect(conn, mode)
    if not conn or mode == "picker" or mode == "splash" then
        return "Not Connected", ATTR.DATE
    end
    local target = connect_target(conn)
    if conn.ok then
        return "ok  " .. target, ATTR.VERSION
    end
    local detail = conn.detail or "failed"
    if #detail > 48 then
        detail = detail:sub(1, 45) .. "…"
    end
    return "fail  " .. target .. "  (" .. detail .. ")", ATTR.ERR
end

local function instance_block(opts, app, _, override)
    override = override or {}
    local mode = app.mode or ""
    local wrap = override.wrapper
    if not wrap then
        if mode == "picker" then
            wrap = picker_wrapper_path(app, opts)
        else
            wrap = opts.wrapper or ""
        end
    end
    local working = opts.work_dir
    if working == "" then
        working = app.planned_work_dir or "(none)"
    end
    if working == "" then
        working = "(none)"
    end
    local logging = C.INSTANCE_LOG
    local ctext, cattr = instance_connect(app.conn, mode)
    local lattr = ATTR.COLHEAD
    return {
        { "Wrapper", basename(wrap), lattr, ATTR.PATH },
        { "Logging", logging, lattr, ATTR.PATH },
        { "Working", working, lattr, ATTR.PATH },
        { "Connect", ctext, lattr, cattr },
    }
end

local function session_header(opts, app, width)
    return instance_block(opts, app, width)
end

local function log_tail(path, max_lines)
    local lines = {}
    if not path or path == "" then
        return lines
    end
    local f = io.open(path, "r")
    if not f then
        return lines
    end
    for line in f:lines() do
        lines[#lines + 1] = line
        if #lines > max_lines then
            table.remove(lines, 1)
        end
    end
    f:close()
    return lines
end

local function build_result_lines(opts, ran, exit_code, built, err, Q, ATTR_OK)
    local lines = {}
    local function add(text, attr)
        lines[#lines + 1] = { text, attr }
    end
    if ran then
        add(string.format("SchemaTool exit %d  (0 clean / 2 drift / 3 anomaly)", exit_code),
            (exit_code == 0 or exit_code == 2 or exit_code == 3) and ATTR.VERSION or ATTR.ERR)
    elseif opts.reuse then
        add("Loaded existing artifacts (--reuse)", ATTR.VERSION)
    end
    if err then
        add(err, ATTR.ERR)
        if ran then
            local tail = log_tail(opts.work_dir .. "/" .. C.INSTANCE_LOG, 6)
            for i = 1, #tail do
                add(tail[i], ATTR.PATH)
            end
        end
    end
    if built and not err then
        local tot = built.totals
        add(string.format("Total migrations found     %d", tot.total), ATTR.PROMPT)
        add(string.format("Perfect migrations         %d", tot.perfect), ATTR.PROMPT)
        add(string.format("Accepted variations        %d", tot.accepted), ATTR.PROMPT)
        add(string.format("Findings for review        %d", tot.subject), ATTR.PROMPT)
        if tot.applied > 0 or tot.packet > 0 then
            add(string.format("Applied / packets          %d / %d", tot.applied, tot.packet),
                ATTR.PATH)
        end
        add("", ATTR.PATH)
        add("Variance classes (findings for review)", ATTR.SECTION)
        if #built.classes == 0 then
            add("  (none)", ATTR.PATH)
        else
            for i = 1, #built.classes do
                local c = built.classes[i]
                add(string.format("  %-28s %d", c.name, c.count), ATTR.PATH)
            end
        end
    end
    add("", ATTR.PATH)
    add("[W] pick another wrapper   [Q]uit", ATTR.PROMPT)
    if built and Q.artifacts_present(opts.work_dir, opts.track) then
        add("[Enter] review existing artifacts", ATTR.PROMPT)
    end
    return lines
end

return {
    read_tail_bytes = read_tail_bytes,
    parse_schematool_progress = parse_schematool_progress,
    progress_bar = progress_bar,
    progress_eighths = progress_eighths,
    progress_bar_spans = progress_bar_spans,
    eighths_width = eighths_width,
    ui_issue_class = ui_issue_class,
    apply_issue_scroll = apply_issue_scroll,
    handle_run_input = handle_run_input,
    issue_attr = function(ui)
        return CLASS_ATTR[ui] or ATTR.PATH
    end,
    invoke_schematool = invoke_schematool,
    probe_connect = probe_connect,
    connect_text = connect_text,
    instance_block = instance_block,
    session_header = session_header,
    log_tail = log_tail,
    build_result_lines = build_result_lines,
}
