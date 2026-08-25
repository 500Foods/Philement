-- schemahelper_invoke.lua
-- SchemaTool invocation, progress parsing, connect probe text, and the
-- post-run result-line builder.
--
-- CHANGELOG
-- 0.5.8 - 2026-08-25 - Extracted from schemahelper.lua (invoke cluster)

local C = require("schemahelper_const")
local W = require("schemahelper_wrappers")
local connect = require("schemahelper_connect")

local ATTR = C.ATTR

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

local function parse_schematool_progress(log, fallback_total)
    local prog = {
        phase = "starting",
        current = 0,
        total = fallback_total or 0,
        ref = nil,
    }
    local text = read_tail_bytes(log, 32768)
    for line in (text .. "\n"):gmatch("([^\n]*)\n") do
        local phase = line:match("^phase: (%S+)")
        if phase then
            prog.phase = phase
        end
        local cur, tot, ref = line:match("^expect (%d+)/(%d+) ref (%d+)")
        if cur then
            prog.phase = "expect"
            prog.current = tonumber(cur) or 0
            prog.total = tonumber(tot) or prog.total
            prog.ref = tonumber(ref)
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

local function progress_bar(width, current, total)
    if width < 3 then
        return ""
    end
    local inner = width - 2
    local filled = 0
    if total and total > 0 then
        filled = math.floor(inner * current / total + 0.5)
        if filled > inner then
            filled = inner
        end
    end
    return "[" .. string.rep("█", filled) .. string.rep("─", inner - filled) .. "]"
end

local function invoke_schematool(opts, screen, app, show_mode_fn)
    local log = opts.out_dir .. "/schemahelper_schematool.log"
    local exitf = opts.out_dir .. "/schemahelper_schematool.exit"
    local parts = {
        W.sh_quote(opts.wrapper),
        "--format",
        "json",
        "--out-dir",
        W.sh_quote(opts.out_dir),
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
    }
    local cmd = "(" .. table.concat(parts, " ") .. " > " .. W.sh_quote(log)
        .. " 2>&1; echo $? > " .. W.sh_quote(exitf) .. ") &"
    os.execute(cmd)
    while true do
        local raw = C.t.input.readansi(0.2)
        if raw == nil then
            screen:check_resize(true)
        end
        app.progress = parse_schematool_progress(log, total)
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

local function connect_text(conn)
    if not conn then
        return "checking…", ATTR.DATE
    end
    local target
    if conn.family == "file" or (conn.host == "" and conn.database ~= "") then
        target = conn.database
    else
        local who = conn.user
        if who ~= "" then
            who = who .. "@"
        end
        target = who .. conn.host
        if conn.port ~= "" then
            target = target .. ":" .. conn.port
        end
        if conn.database ~= "" then
            target = target .. "/" .. conn.database
        end
        if conn.schema ~= "" then
            target = target .. "  schema=" .. conn.schema
        end
    end
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

local function session_header(opts, app, _)
    local lines = {}
    local function add(text, attr)
        lines[#lines + 1] = { text, attr }
    end
    add("wrapper  " .. (opts.wrapper ~= "" and opts.wrapper or "(none)"), ATTR.PATH)
    add("out-dir  " .. (opts.out_dir ~= "" and opts.out_dir or "(none)"), ATTR.PATH)
    add("state    " .. (opts.state_file ~= "" and opts.state_file or "(none)"), ATTR.PATH)
    add("track    " .. opts.track, ATTR.PATH)
    add("log      " .. (app.log or "(none)"), ATTR.PATH)
    local ctext, cattr = connect_text(app.conn)
    add("connect  " .. ctext, cattr)
    return lines
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
            local tail = log_tail(opts.out_dir .. "/schemahelper_schematool.log", 6)
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
    if built and Q.artifacts_present(opts.out_dir, opts.track) then
        add("[Enter] review existing artifacts", ATTR.PROMPT)
    end
    return lines
end

return {
    read_tail_bytes = read_tail_bytes,
    parse_schematool_progress = parse_schematool_progress,
    progress_bar = progress_bar,
    invoke_schematool = invoke_schematool,
    probe_connect = probe_connect,
    connect_text = connect_text,
    session_header = session_header,
    log_tail = log_tail,
    build_result_lines = build_result_lines,
}
