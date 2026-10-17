-- schemahelper_invoke.lua
-- SchemaTool invocation, progress parsing, connect probe text, and the
-- post-run result-line builder.
--
-- CHANGELOG
-- 0.6.11 - 2026-10-17 - Step labels share one width; counts are four digits
-- 0.6.9 - 2026-10-16 - Loading screen keeps one line per finished step
-- 0.6.8 - 2026-10-16 - Result counts use the dashboard column
-- 0.6.7 - 2026-10-16 - Progress log stays separate; phase labels name the work
-- 0.6.6 - 2026-10-07 - Progress tint treats type and dropped as catalog
-- 0.6.5 - 2026-10-07 - Expect progress may include engine=<name>
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
        or raw == "nullability" or raw == "type" or raw == "dropped"
        or raw == "extra_table" or raw == "extra_column" then
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

local function touch_step(prog, key, label, cur, tot, detail)
    local steps = prog.steps
    local step
    for i = 1, #steps do
        if steps[i].key == key then
            step = steps[i]
        end
    end
    if not step then
        for i = 1, #steps do
            steps[i].done = true
        end
        step = {
            key = key,
            label = label,
            cur = 0,
            tot = 0,
            detail = "",
            done = false,
        }
        steps[#steps + 1] = step
    end
    step.label = label
    if cur and cur > 0 then
        step.cur = cur
    end
    if tot and tot > 0 then
        step.tot = tot
    end
    if detail and detail ~= "" then
        step.detail = detail:gsub("^%s+", ""):gsub("%s+$", "")
    end
    if step.tot > 0 and step.cur >= step.tot then
        step.done = true
    end
end

local function note_phase(prog, line)
    local phase, detail = line:match("^phase: (%S+)%s*(.*)")
    if not phase then
        return
    end
    detail = detail or ""
    prog.phase = phase
    prog.detail = detail
    if phase == "discover" then
        local files = detail:match("^files=(%d+)")
        if files then
            prog.discover_total = tonumber(files)
            touch_step(prog, "discover", "Finding migration files",
                prog.discover_total, prog.discover_total, "")
        else
            touch_step(prog, "discover", "Finding migration files", 0, 0, "")
        end
    elseif phase == "expect" then
        touch_step(prog, "expect", "Reading migration", 0, 0, "")
    elseif phase == "dump" then
        touch_step(prog, "dump", "Reading the database", 0, 0, "")
    elseif phase == "compare" then
        touch_step(prog, "compare", "Comparing queries", 0, 0, "")
    elseif phase == "remediate" then
        touch_step(prog, "remediate", "Writing the report", 0, 0, "")
    elseif phase == "catalog" then
        local fc, ft = detail:match("^fold%s+(%d+)/(%d+)")
        if not fc then
            fc, ft = detail:match("^table%s+(%d+)/(%d+)")
        end
        if fc then
            prog.cat_current = tonumber(fc)
            prog.cat_total = tonumber(ft)
        end
        local tname = detail:match("^table%s+%d+/%d+%s+(%S+)")
        if tname then
            prog.name = tname
        else
            prog.name = nil
        end
        local pn = detail:match("^probe%s+(%d+)")
        if pn then
            prog.cat_total = tonumber(pn)
            prog.cat_current = 0
            prog.name = nil
        end
        if detail:find("^fold", 1) then
            touch_step(prog, "fold", "Folding migration DDL",
                prog.cat_current, prog.cat_total, "")
        elseif detail:find("^table", 1) then
            touch_step(prog, "tables", "Finding tables",
                prog.cat_current, prog.cat_total, prog.name or "")
        elseif detail:find("^probe", 1) then
            touch_step(prog, "shapes", "Reading table shapes", 0, 0, "")
        end
    elseif phase == "rows" then
        local rc, rt, rname = detail:match("^probe%s+(%d+)/(%d+)%s*(.*)")
        if rc then
            prog.row_current = tonumber(rc)
            prog.row_total = tonumber(rt)
            if rname and rname ~= "" then
                prog.name = rname
            end
            touch_step(prog, "rows", "Checking default rows",
                prog.row_current, prog.row_total, prog.name or "")
        elseif detail:find("^skipped", 1) then
            touch_step(prog, "rows", "Checking default rows", 0, 0, "")
            prog.steps[#prog.steps].done = true
        end
    end
end

local function parse_schematool_progress(log, fallback_total)
    local prog = {
        phase = "starting",
        detail = "",
        current = 0,
        total = fallback_total or 0,
        ref = nil,
        name = nil,
        issues = {},
        cell_issue = {},
        ref_at = {},
        compare_current = 0,
        discover_total = 0,
        cat_current = 0,
        cat_total = 0,
        row_current = 0,
        row_total = 0,
        steps = {},
        ref_name = {},
    }
    local text = read_tail_bytes(log, 262144)
    for line in (text .. "\n"):gmatch("([^\n]*)\n") do
        note_phase(prog, line)
        local cur, tot, ref, rest = line:match("^expect (%d+)/(%d+) ref (%d+)%s*(.*)")
        local name
        if rest and rest ~= "" then
            name = rest:match("name=(.*)")
        end
        if cur then
            prog.phase = "expect"
            prog.current = tonumber(cur) or 0
            prog.total = tonumber(tot) or prog.total
            prog.ref = tonumber(ref)
            if name and name ~= "" then
                prog.name = name:gsub("^%s+", ""):gsub("%s+$", "")
            else
                prog.name = nil
            end
            if prog.ref then
                prog.ref_at[prog.ref] = prog.current
                if prog.name and prog.name ~= "" then
                    prog.ref_name[prog.ref] = prog.name
                end
            end
            local detail = prog.ref and string.format("%04d", prog.ref) or ""
            if prog.name and prog.name ~= "" then
                detail = detail .. " " .. prog.name
            end
            touch_step(prog, "expect", "Reading migration",
                prog.current, prog.total, detail)
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
            local title = prog.ref_name[prog.ref] or ""
            local detail = prog.ref and string.format("%04d", prog.ref) or ""
            if title ~= "" then
                detail = detail .. " " .. title
            end
            touch_step(prog, "compare", "Comparing queries",
                prog.compare_current, tonumber(ctot) or prog.total, detail)
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

local PHASE_LABEL = {
    starting = "Starting",
    discover = "Finding migration files",
    expect = "Reading migration",
    dump = "Reading the database",
    compare = "Comparing queries",
    catalog = "Reading table shapes",
    rows = "Checking default rows",
    remediate = "Writing the report",
    done = "Finished",
}

local ACTIVITY_LABELS = {
    "Finding migration files",
    "Reading migration",
    "Reading the database",
    "Comparing queries",
    "Folding migration DDL",
    "Reading table shapes",
    "Finding tables",
    "Checking default rows",
    "Writing the report",
}

local ACTIVITY_WIDTH = 0
for i = 1, #ACTIVITY_LABELS do
    if #ACTIVITY_LABELS[i] > ACTIVITY_WIDTH then
        ACTIVITY_WIDTH = #ACTIVITY_LABELS[i]
    end
end

local COUNT_WIDTH = 9

local function step_line(step)
    local label = string.format("%-" .. ACTIVITY_WIDTH .. "s", step.label or "")
    local cur = step.cur or 0
    local tot = step.tot or 0
    local detail = step.detail or ""
    if step.done then
        detail = "Done"
    elseif tot < 1 and detail == "" then
        detail = "…"
    end
    local count
    if tot > 0 then
        count = string.format("%04d/%04d", cur, tot)
    else
        count = string.rep(" ", COUNT_WIDTH)
    end
    if detail ~= "" then
        return string.format("%s %s: %s", label, count, detail)
    end
    return string.format("%s %s", label, count)
end

local function phase_text(prog)
    prog = prog or {}
    local phase = prog.phase or "starting"
    local detail = prog.detail or ""
    local label = PHASE_LABEL[phase] or phase
    if phase == "catalog" and detail:find("^fold", 1) then
        label = "Folding migration DDL"
    elseif phase == "rows" and detail:find("^probe", 1) then
        label = "Checking default rows"
    end
    local extra = ""
    if phase == "expect" and (prog.total or 0) > 0 then
        extra = string.format("%d / %d", prog.current or 0, prog.total)
        local desc = prog.name
        if not desc or desc == "" then
            desc = prog.ref and ("ref " .. tostring(prog.ref)) or ""
        end
        if desc ~= "" then
            extra = extra .. "   " .. desc
        end
    elseif phase == "compare" then
        extra = string.format("%d / %d",
            prog.compare_current or prog.current or 0, prog.total or 0)
    elseif phase == "discover" and (prog.discover_total or 0) > 0 then
        extra = string.format("%d files", prog.discover_total)
    elseif phase == "catalog" and (prog.cat_total or 0) > 0 then
        extra = string.format("%d / %d", prog.cat_current or 0, prog.cat_total)
        if prog.name and prog.name ~= "" and detail:find("^table", 1) then
            extra = extra .. "   " .. prog.name
        end
    elseif phase == "rows" and (prog.row_total or 0) > 0 then
        extra = string.format("%d / %d", prog.row_current or 0, prog.row_total)
        if prog.name and prog.name ~= "" then
            extra = extra .. "   " .. prog.name
        end
    end
    if extra ~= "" then
        return "  " .. label .. "   " .. extra
    end
    return "  " .. label
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
    local progress = opts.work_dir .. "/" .. C.INSTANCE_PROGRESS
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
    local wipe_p = io.open(progress, "w")
    if wipe_p then
        wipe_p:close()
    end
    os.remove(exitf)
    local total = W.count_disk_refs(opts.migrations, opts.design)
    app.progress = {
        phase = "starting",
        detail = "",
        current = 0,
        total = total,
        ref = nil,
        issues = {},
        cell_issue = {},
        issue_scroll = 0,
        issue_vis = 8,
    }
    local cmd = "(" .. table.concat(parts, " ") .. " > " .. W.sh_quote(log)
        .. " 2> " .. W.sh_quote(progress)
        .. "; echo $? > " .. W.sh_quote(exitf) .. ") &"
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
        app.progress = parse_schematool_progress(progress, total)
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
            local steps = app.progress.steps or {}
            for i = 1, #steps do
                steps[i].done = true
            end
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
            local tail_path = opts.work_dir .. "/" .. C.INSTANCE_PROGRESS
            local tail = log_tail(tail_path, 8)
            if #tail == 0 then
                tail = log_tail(opts.work_dir .. "/" .. C.INSTANCE_LOG, 8)
            end
            for i = 1, #tail do
                add(tail[i], ATTR.PATH)
            end
        end
    end
    if built and not err then
        local tot = built.totals
        local rows = {
            { label = "Total migrations found", value = tot.total },
            { label = "Perfect migrations", value = tot.perfect },
            { label = "Accepted variations", value = tot.accepted },
            { label = "Findings for review", value = tot.subject },
            { label = "Live extras (not applicable)", value = tot.info or 0 },
            { label = "Unkeyed DML (AutoMigration)", value = tot.unkeyed or 0 },
        }
        if tot.applied > 0 or tot.packet > 0 then
            rows[#rows + 1] = {
                label = "Applied / packets",
                value = string.format("%d / %d", tot.applied, tot.packet),
            }
        end
        local class_rows = {}
        for i = 1, #built.classes do
            local c = built.classes[i]
            class_rows[#class_rows + 1] = {
                label = c.name,
                value = c.count,
                indent = 2,
            }
        end
        local aligned_in = {}
        for i = 1, #rows do
            aligned_in[#aligned_in + 1] = rows[i]
        end
        for i = 1, #class_rows do
            aligned_in[#aligned_in + 1] = class_rows[i]
        end
        local aligned = Q.align_count_lines(aligned_in)
        for i = 1, #rows do
            add(aligned[i], ATTR.PROMPT)
        end
        add("", ATTR.PATH)
        add("Variance classes (findings for review)", ATTR.SECTION)
        if #class_rows == 0 then
            add("  (none)", ATTR.PATH)
        else
            for i = 1, #class_rows do
                add(aligned[#rows + i], ATTR.PATH)
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
    phase_text = phase_text,
    step_line = step_line,
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
