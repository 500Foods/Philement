-- schemahelper.lua
-- SchemaHelper — Interactive SchemaTool front-end (Lua 5.5 + terminal.lua)
--
-- CHANGELOG
-- 0.4.11 - 2026-08-23 - Explore panes, red rules, highlight, brotli decode
-- 0.4.10 - 2026-08-23 - Explore: Migration vs Database; ignore 1000→1003
-- 0.4.9 - 2026-08-23 - Explore: chrome scroll + field diff, no screen.body
-- 0.4.8 - 2026-08-23 - Source wrapper to resolve computed connect flags
-- 0.4.7 - 2026-08-23 - Paint: strip tabs/ANSI so log lines cannot crash width
-- 0.4.6 - 2026-08-23 - Dashboard q/r: do not index keys.r (terminal.lua)
-- 0.4.5 - 2026-08-23 - Connect ping reads wrapper --engine and connect flags
-- 0.4.4 - 2026-08-23 - Catalog failure keeps metadata dashboard
-- 0.4.3 - 2026-08-23 - SchemaTool progress bar from expect log lines
-- 0.4.2 - 2026-08-23 - Show SchemaTool log tail on exit 1
-- 0.4.1 - 2026-08-23 - Failed connect returns to wrapper picker
-- 0.4.0 - 2026-08-23 - Phase 4: generate migration packet
-- 0.2.2 - 2026-08-23 - Session header + live connect probe
-- 0.2.1 - 2026-08-23 - Persistent chrome; wrapper picker inset at top
-- 0.2.0 - 2026-08-23 - Phase 1: picker, SchemaTool invoke, queue result screen
-- 0.1.0 - 2026-08-23 - Splash: real versions/dates, wrapper paths, Esc exits

-- luacheck: globals arg package

local VERSION = "0.4.11"
local RELEASED = "2026-08-23"

local LUA_RELEASES = {
    ["5.5.1"] = "2026-08-03",
    ["5.5.0"] = "2025-12-22",
    ["5.5"] = "2025-12-22",
}

local TERMINAL_RELEASES = {
    ["0.1.0"] = "2026-06-07",
}

local WRAPPER_ORDER = {
    "postgresql",
    "mysql",
    "mariadb",
    "sqlite",
    "db2",
    "cockroachdb",
    "yugabytedb",
}

local WRAPPER_BLURB = {
    postgresql = "ACURANZO_DB_* / schema demo",
    mysql = "CANVAS_DB_* / schema demo",
    mariadb = "CANVAS_DB_* / schema demomrdb",
    sqlite = "hydrodemo.sqlite",
    db2 = "HYDROTST_DB_*",
    cockroachdb = "ACURANZO_DB_* / schema democrdb",
    yugabytedb = "YUGABYTE_DB_*  (never ACURANZO)",
}

local t = require("terminal")
local Screen = require("terminal.ui.panel.screen")
local Panel = require("terminal.ui.panel")

local key_map = t.input.keymap.default_key_map
local keys = t.input.keymap.default_keys

local ATTR_TITLE = { fg = "yellow", brightness = "bright" }
local ATTR_SUB = { fg = "cyan", brightness = "bright" }
local ATTR_VERSION = { fg = "green", brightness = "bright" }
local ATTR_DATE = { fg = "cyan" }
local ATTR_RUNTIME = { fg = "magenta" }
local ATTR_SECTION = { fg = "yellow" }
local ATTR_PATH = { fg = "white", brightness = "dim" }
local ATTR_PROMPT = { fg = "white", brightness = "bright" }
local ATTR_ERR = { fg = "red", brightness = "bright" }
local ATTR_OK = { fg = "green", brightness = "bright" }
local ATTR_RULE = { fg = "red", brightness = "bright" }
local ATTR_HL = { bg = "red", fg = "white", brightness = "bright" }
local ATTR_COLHEAD = { fg = "yellow", brightness = "bright" }

local function script_dir()
    local src = arg[0] or ""
    local dir = src:match("^(.*)/[^/]+$")
    return dir or "."
end

package.path = script_dir() .. "/lua/?.lua;" .. package.path
local queue = require("schemahelper_queue")
local connect = require("schemahelper_connect")
local packet = require("schemahelper_packet")

local function parse_args()
    local opts = {
        wrapper = "",
        migrations = "",
        out_dir = "",
        state_file = "",
        packet_dir = "",
        track = "both",
        reuse = false,
        allow_write = false,
        ref = 0,
        schematool = script_dir() .. "/schematool.sh",
        lua_version = _VERSION:match("Lua%s+([%d.]+)") or _VERSION,
        version = false,
    }
    local i = 1
    while i <= #arg do
        local a = arg[i]
        if a == "--wrapper" then
            opts.wrapper = arg[i + 1] or ""
            i = i + 2
        elseif a == "--migrations" then
            opts.migrations = arg[i + 1] or ""
            i = i + 2
        elseif a == "--out-dir" then
            opts.out_dir = arg[i + 1] or ""
            i = i + 2
        elseif a == "--state-file" then
            opts.state_file = arg[i + 1] or ""
            i = i + 2
        elseif a == "--packet-dir" then
            opts.packet_dir = arg[i + 1] or ""
            i = i + 2
        elseif a == "--track" then
            opts.track = arg[i + 1] or "both"
            i = i + 2
        elseif a == "--reuse" then
            opts.reuse = true
            i = i + 1
        elseif a == "--allow-write" then
            opts.allow_write = true
            i = i + 1
        elseif a == "--ref" then
            opts.ref = tonumber(arg[i + 1]) or 0
            i = i + 2
        elseif a == "--version" or a == "-v" then
            opts.version = true
            i = i + 1
        elseif a == "--schematool" then
            opts.schematool = arg[i + 1] or opts.schematool
            i = i + 2
        elseif a == "--lua-version" then
            opts.lua_version = arg[i + 1] or opts.lua_version
            i = i + 2
        else
            i = i + 1
        end
    end
    return opts
end

local function read_tool_version(path)
    local f = io.open(path, "r")
    if not f then
        return nil, nil
    end
    local ver
    local date
    for line in f:lines() do
        local v = line:match('^VERSION="([^"]+)"')
        if v then
            ver = v
        end
        local dv, dd = line:match("^#%s+(%d+%.%d+%.%d+)%s+%-%s+(%d%d%d%d%-%d%d%-%d%d)%s+")
        if dv and (not ver or dv == ver) and not date then
            date = dd
        end
        if ver and date then
            break
        end
    end
    f:close()
    return ver, date
end

local function wrapper_engine(path)
    local base = path:match("([^/]+)$") or path
    return base:match("^schematool_(.+)%.sh$") or ""
end

local function wrapper_meta(path)
    local engine = wrapper_engine(path)
    local flags = connect.parse_wrapper(path)
    local design = flags.design or "acuranzo"
    local schema = flags.schema or ""
    if schema == "" then
        local resolved = connect.resolve(path)
        schema = resolved.schema or ""
    end
    return design, engine, schema
end

local function wrapper_dir(path)
    return path:match("^(.*)/[^/]+$") or "."
end

local function discover_wrappers(dir)
    local found = {}
    local cmd = 'ls -1 "' .. dir:gsub('"', '\\"') .. '"/schematool_*.sh 2>/dev/null'
    local h = io.popen(cmd)
    if h then
        for line in h:lines() do
            if line ~= "" then
                local eng = wrapper_engine(line)
                if eng ~= "" then
                    found[eng] = line
                end
            end
        end
        h:close()
    end
    local list = {}
    local seen = {}
    for _, eng in ipairs(WRAPPER_ORDER) do
        if found[eng] then
            list[#list + 1] = { engine = eng, path = found[eng] }
            seen[eng] = true
        end
    end
    local extras = {}
    for eng, path in pairs(found) do
        if not seen[eng] then
            extras[#extras + 1] = { engine = eng, path = path }
        end
    end
    table.sort(extras, function(a, b)
        return a.engine < b.engine
    end)
    for _, item in ipairs(extras) do
        list[#list + 1] = item
    end
    return list
end

local function wrapper_label(item)
    local base = item.path:match("([^/]+)$") or item.path
    local blurb = WRAPPER_BLURB[item.engine] or item.engine
    return string.format("%-28s %s", base, blurb)
end

local function sh_quote(s)
    return "'" .. tostring(s):gsub("'", "'\\''") .. "'"
end

local function ensure_dir(path)
    if path and path ~= "" then
        os.execute("mkdir -p " .. sh_quote(path))
    end
end

local function count_disk_refs(migrations, design)
    if not migrations or migrations == "" then
        return 0
    end
    local design_pat = (design or "acuranzo"):gsub("(%W)", "%%%1")
    local patterns = {
        "^" .. design_pat .. "_(%d+)%.lua$",
        "^design_(%d+)%.lua$",
    }
    local n = 0
    local h = io.popen("ls -1 " .. sh_quote(migrations) .. " 2>/dev/null")
    if not h then
        return 0
    end
    for name in h:lines() do
        for i = 1, #patterns do
            if name:match(patterns[i]) then
                n = n + 1
                break
            end
        end
    end
    h:close()
    return n
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

local show_mode

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

local function invoke_schematool(opts, screen, app)
    local log = opts.out_dir .. "/schemahelper_schematool.log"
    local exitf = opts.out_dir .. "/schemahelper_schematool.exit"
    local parts = {
        sh_quote(opts.wrapper),
        "--format",
        "json",
        "--out-dir",
        sh_quote(opts.out_dir),
        "--no-detail",
    }
    if opts.track == "catalog" or opts.track == "both" then
        parts[#parts + 1] = "--catalog"
    end
    if opts.migrations ~= "" then
        parts[#parts + 1] = "--migrations"
        parts[#parts + 1] = sh_quote(opts.migrations)
    end
    local wipe = io.open(log, "w")
    if wipe then
        wipe:close()
    end
    os.remove(exitf)
    local total = count_disk_refs(opts.migrations, opts.design)
    app.progress = {
        phase = "starting",
        current = 0,
        total = total,
        ref = nil,
    }
    local cmd = "(" .. table.concat(parts, " ") .. " > " .. sh_quote(log)
        .. " 2>&1; echo $? > " .. sh_quote(exitf) .. ") &"
    os.execute(cmd)
    while true do
        local raw = t.input.readansi(0.2)
        if raw == nil then
            screen:check_resize(true)
        end
        app.progress = parse_schematool_progress(log, total)
        show_mode(screen, app, "running")
        local ef = io.open(exitf, "r")
        if ef then
            local code = tonumber((ef:read("*l") or ""):match("%d+")) or 1
            ef:close()
            os.remove(exitf)
            if app.progress.total > 0 then
                app.progress.current = app.progress.total
            end
            app.progress.phase = "done"
            show_mode(screen, app, "running")
            return code, log
        end
    end
end

local function probe_connect(opts)
    return connect.probe(opts.wrapper)
end

local function connect_text(conn)
    if not conn then
        return "checking…", ATTR_DATE
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
        return "ok    " .. family .. target, ATTR_VERSION
    end
    local detail = conn.detail or "failed"
    if #detail > 48 then
        detail = detail:sub(1, 45) .. "…"
    end
    return "fail  " .. family .. target .. "  (" .. detail .. ")", ATTR_ERR
end

local function session_header(opts, app, _)
    local lines = {}
    local function add(text, attr)
        lines[#lines + 1] = { text, attr }
    end
    add("wrapper  " .. (opts.wrapper ~= "" and opts.wrapper or "(none)"), ATTR_PATH)
    add("out-dir  " .. (opts.out_dir ~= "" and opts.out_dir or "(none)"), ATTR_PATH)
    add("state    " .. (opts.state_file ~= "" and opts.state_file or "(none)"), ATTR_PATH)
    add("track    " .. opts.track, ATTR_PATH)
    add("log      " .. (app.log or "(none)"), ATTR_PATH)
    local ctext, cattr = connect_text(app.conn)
    add("connect  " .. ctext, cattr)
    return lines
end

local function display_text(text)
    text = tostring(text or "")
    text = text:gsub("\t", "    ")
    text = text:gsub("\27%[[%d;?]*[ -/]*[@-~]", "")
    text = text:gsub("[%z\1-\8\11-\31\127]", "")
    return text
end

local function swidth(text)
    text = display_text(text)
    local ok, w = pcall(t.text.width.utf8swidth, text)
    if ok and type(w) == "number" then
        return w
    end
    return #text
end

local function write_span(row, col, width, text, attr)
    width = tonumber(width) or 0
    if width < 1 then
        return
    end
    text = display_text(text)
    local ok, shown = pcall(t.text.width.truncate_ellipsis, width, text, "right")
    if not ok or type(shown) ~= "string" then
        shown = text:sub(1, width)
    end
    t.cursor.position.set(row, col)
    t.output.write(t.text.push_seq(attr), shown, t.text.pop_seq())
end

local function write_centered(row, col, width, text, attr, trunc)
    text = display_text(text)
    local ok, shown = pcall(t.text.width.truncate_ellipsis, width, text, trunc or "right")
    if not ok or type(shown) ~= "string" then
        shown = text
    end
    local w = swidth(shown)
    if w < 1 then
        return
    end
    write_span(row, col + math.max(0, math.floor((width - w) / 2)), w, shown, attr)
end

local function wait_enter_or_esc(screen)
    while true do
        local raw = t.input.readansi(0.2)
        if raw == nil then
            if screen then
                screen:check_resize(true)
            end
        else
            local name = key_map[raw]
            if name == keys.enter or name == keys.escape then
                return name
            end
        end
    end
end

local function splash_content(self)
    local row = self.inner_row
    local col = self.inner_col
    local height = self.inner_height
    local width = self.inner_width
    if height < 1 or width < 1 then
        return
    end

    local opts = self.opts
    local tool_ver, tool_date = read_tool_version(opts.schematool)
    local term_ver = t._VERSION or "0.1.0"
    local versions = {
        { "SchemaHelper", VERSION, RELEASED, ATTR_TITLE },
        { "SchemaTool", tool_ver or "?", tool_date or "", ATTR_TITLE },
        { "Lua", opts.lua_version, LUA_RELEASES[opts.lua_version] or "", ATTR_RUNTIME },
        { "terminal.lua", term_ver, TERMINAL_RELEASES[term_ver] or "", ATTR_RUNTIME },
    }

    local name_w = 0
    local ver_w = 0
    local date_w = 0
    for i = 1, #versions do
        name_w = math.max(name_w, swidth(versions[i][1]))
        ver_w = math.max(ver_w, swidth(versions[i][2]))
        date_w = math.max(date_w, swidth(versions[i][3]))
    end

    local gap = 3
    local table_w = name_w + gap + ver_w + gap + date_w
    if table_w > width then
        table_w = width
    end
    local table_col = col + math.max(0, math.floor((width - table_w) / 2))
    local ver_col = table_col + name_w + gap
    local date_col = ver_col + ver_w + gap

    local migrations = opts.migrations
    if migrations == "" then
        migrations = "(migrations not resolved)"
    end
    local wrapper = opts.wrapper
    if wrapper == "" then
        wrapper = "(pick a SchemaTool wrapper after Enter)"
    end

    local last = row + height - 1
    local lines = {
        { "text", "Welcome to SchemaHelper", ATTR_TITLE },
        { "text", "Frontend to Schema Tool", ATTR_SUB },
        { "blank" },
        { "version", 1 },
        { "version", 2 },
        { "version", 3 },
        { "version", 4 },
        { "blank" },
        { "text", "Migration Comparator", ATTR_SECTION },
        { "path", migrations },
        { "blank" },
        { "text", "Database Comparator", ATTR_SECTION },
        { "path", wrapper },
        { "blank" },
        { "text", "Press Enter to continue", ATTR_PROMPT },
        { "text", "Press ESC to exit", ATTR_PROMPT },
    }

    local start = row + math.max(0, math.floor((height - #lines) / 2))

    for i = 1, #lines do
        local line_row = start + i - 1
        if line_row > last then
            break
        end
        local spec = lines[i]
        if spec[1] == "text" then
            write_centered(line_row, col, width, spec[2], spec[3], "right")
        elseif spec[1] == "path" then
            write_centered(line_row, col, width, spec[2], ATTR_PATH, "left")
        elseif spec[1] == "version" then
            local item = versions[spec[2]]
            local name_pad = string.rep(" ", name_w - swidth(item[1]))
            write_span(line_row, table_col, name_w, name_pad .. item[1], item[4])
            if date_col <= col + width then
                write_span(line_row, ver_col, ver_w, item[2], ATTR_VERSION)
                if item[3] ~= "" and date_col + date_w <= col + width then
                    write_span(line_row, date_col, date_w, item[3], ATTR_DATE)
                end
            end
        end
    end
end

local function paint_hline_join(self, row)
    local col = self.inner_col
    local width = self.inner_width
    if not row or width < 1 or col < 2 then
        return
    end
    t.cursor.position.set(row, col - 1)
    t.output.write(
        t.text.push_seq(ATTR_RULE),
        "├" .. string.rep("─", width) .. "┤",
        t.text.pop_seq())
end

local function paint_vline_join(self, split_col, top, bot)
    if not split_col or not top or not bot or bot <= top then
        return
    end
    t.cursor.position.set(top, split_col)
    t.output.write(t.text.push_seq(ATTR_RULE), "┬", t.text.pop_seq())
    for r = top + 1, bot - 1 do
        t.cursor.position.set(r, split_col)
        t.output.write(t.text.push_seq(ATTR_RULE), "│", t.text.pop_seq())
    end
    t.cursor.position.set(bot, split_col)
    t.output.write(t.text.push_seq(ATTR_RULE), "┴", t.text.pop_seq())
end

local function paint_framed(self, header, body, footer)
    local row = self.inner_row
    local col = self.inner_col
    local height = self.inner_height
    local width = self.inner_width
    if height < 3 or width < 4 then
        return
    end
    local last = row + height - 1
    local footer_row = last
    local footer_rule = last - 1
    local y = row
    for i = 1, #header do
        if y >= footer_rule then
            break
        end
        write_span(y, col + 1, width - 2, header[i][1], header[i][2] or ATTR_PATH)
        y = y + 1
    end
    if y < footer_rule then
        paint_hline_join(self, y)
        y = y + 1
    end
    local vis = footer_rule - y
    if vis < 0 then
        vis = 0
    end
    for i = 1, math.min(#body, vis) do
        write_span(y + i - 1, col + 1, width - 2, body[i][1], body[i][2] or ATTR_PATH)
    end
    paint_hline_join(self, footer_rule)
    write_span(footer_row, col + 1, width - 2, footer or "", ATTR_PROMPT)
end

local function picker_content(self)
    local app = self.app
    local list = app.picker.list
    local selected = app.picker.selected
    local lines = {
        { "Select SchemaTool target", ATTR_TITLE },
        { "", ATTR_PATH },
    }
    for i, item in ipairs(list) do
        local mark = i == selected and "● " or "○ "
        local attr = i == selected and ATTR_TITLE or ATTR_PATH
        lines[#lines + 1] = { mark .. wrapper_label(item), attr }
    end
    paint_framed(self, {}, lines, "Press Enter to select   Press ESC to exit")
end

local function running_content(self)
    local app = self.app
    local lines = session_header(self.opts, app, self.inner_width)
    lines[#lines + 1] = { "", ATTR_PATH }
    lines[#lines + 1] = { app.status_note or "Working…", ATTR_TITLE }
    local prog = app.progress
    if prog then
        local label = "  phase    " .. (prog.phase or "starting")
        if prog.phase == "expect" and prog.total and prog.total > 0 then
            label = string.format("  expect   %d / %d", prog.current or 0, prog.total)
            if prog.ref then
                label = label .. "   ref " .. tostring(prog.ref)
            end
        elseif prog.total and prog.total > 0 and (prog.current or 0) > 0 then
            label = string.format("  %s     %d / %d",
                prog.phase or "work", prog.current, prog.total)
        end
        lines[#lines + 1] = { label, ATTR_VERSION }
        local bar_w = math.max(10, (self.inner_width or 40) - 8)
        local pct = ""
        if prog.total and prog.total > 0 then
            pct = string.format("  %d%%",
                math.floor(100 * (prog.current or 0) / prog.total))
        end
        lines[#lines + 1] = {
            "  " .. progress_bar(bar_w, prog.current or 0, prog.total or 0) .. pct,
            ATTR_SUB,
        }
    end
    local header = session_header(self.opts, app, self.inner_width)
    local body = {}
    for i = #header + 1, #lines do
        body[#body + 1] = lines[i]
    end
    paint_framed(self, header, body, "Running SchemaTool…")
end

local function result_content(self)
    local header = session_header(self.opts, self.app, self.inner_width)
    local raw = self.app.result_lines or {}
    local body = {}
    local footer = "[w] pick another wrapper   [q]uit"
    for i = 1, #raw do
        local text = raw[i][1] or ""
        if text:match("^%[w%]") or text:match("^%[Enter%]") then
            if text:match("%[Enter%]") then
                footer = footer .. "   [Enter] review artifacts"
            end
        else
            body[#body + 1] = raw[i]
        end
    end
    paint_framed(self, header, body, footer)
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

local function dashboard_content(self)
    local app = self.app
    local header = session_header(self.opts, app, self.inner_width)
    local reserved = packet.list_reserved(
        self.opts.packet_dir, self.opts.design, self.opts.engine)
    local dash_lines, built = queue.build_dashboard_lines({
        out_dir = self.opts.out_dir,
        track = self.opts.track,
        state = app.state,
        reserved = reserved,
    })
    app.built = built
    local body = {}
    for i = 1, #dash_lines do
        body[#body + 1] = { dash_lines[i], ATTR_PATH }
    end
    if app.warn_in_repo then
        body[#body + 1] = { "", ATTR_PATH }
        body[#body + 1] = {
            "Warning: packet path is inside the git tree",
            ATTR_ERR,
        }
    end
    if app.catalog_degraded then
        body[#body + 1] = { "", ATTR_PATH }
        body[#body + 1] = {
            "Catalog track failed; metadata findings kept",
            ATTR_ERR,
        }
    end
    local msg = app.show_mode_msg or ""
    if msg ~= "" and msg ~= "Catalog track failed; metadata findings kept" then
        body[#body + 1] = { msg, ATTR_OK }
    end
    paint_framed(self, header, body,
        "[Enter] begin review   [r]e-audit   [q]uit")
end

local function review_content(self)
    local app = self.app
    local header = session_header(self.opts, app, self.inner_width)
    if not app.built or not app.built.subject then
        paint_framed(self, header,
            { { "No findings to review", ATTR_TITLE } },
            "[q]uit to dashboard")
        return
    end
    local subj = app.built.subject
    if #subj == 0 then
        paint_framed(self, header,
            { { "All findings reviewed — none subject for review", ATTR_TITLE } },
            "[q]uit to dashboard")
        return
    end
    local idx = app.review_index
    if idx < 1 then
        idx = 1
        app.review_index = 1
    elseif idx > #subj then
        idx = #subj
        app.review_index = #subj
    end
    local finding = subj[idx]
    local next_ref, g_reason = packet_next(self.opts)
    local review_lines = queue.build_review_lines_detailed(
        finding, self.opts.out_dir, app.state, next_ref, g_reason)
    local body = {}
    for i = 1, #review_lines do
        local line = review_lines[i]
        if not line:match("^What would you like")
            and not line:match("^  %[[esaugn%]")
            and not line:match("^  %[[np%]") then
            body[#body + 1] = { line, ATTR_PATH }
        end
    end
    paint_framed(self, header, body,
        string.format("[%d of %d]  [e]xplore [s]kip [a]ccept [g]enerate  [n]/[p]  [q]uit",
            idx, #subj))
end

local function wrap_display_line(text, width)
    text = display_text(text)
    if width < 8 then
        width = 8
    end
    local out = {}
    if text == "" then
        out[1] = ""
        return out
    end
    while swidth(text) > width do
        local cut = width
        if cut > #text then
            cut = #text
        end
        while cut > 1 and swidth(text:sub(1, cut)) > width do
            cut = cut - 1
        end
        out[#out + 1] = text:sub(1, cut)
        text = text:sub(cut + 1)
    end
    out[#out + 1] = text
    return out
end

local function flatten_explore_rows(view, col_w)
    local out = {}
    local rows = (view and view.rows) or {}
    for i = 1, #rows do
        local r = rows[i]
        if r.kind == "label" then
            out[#out + 1] = { kind = "label", text = r.text or "" }
        else
            local wrap_w = math.max(4, col_w - 5)
            local L = wrap_display_line(r.left or "", wrap_w)
            local R = wrap_display_line(r.right or "", wrap_w)
            local nwrap = math.max(#L, #R)
            for w = 1, nwrap do
                out[#out + 1] = {
                    kind = "pair",
                    n = (w == 1) and r.n or nil,
                    left = L[w] or "",
                    right = R[w] or "",
                    same = r.same,
                }
            end
        end
    end
    return out
end

local function next_pair_index(rows, from, dir)
    local i = from + dir
    while i >= 1 and i <= #rows do
        if rows[i].kind == "pair" then
            return i
        end
        i = i + dir
    end
    return from
end

local function explore_content(self)
    local app = self.app
    local col = self.inner_col
    local row = self.inner_row
    local height = self.inner_height
    local width = self.inner_width
    if height < 5 or width < 16 then
        return
    end
    local view = app.explore_view or { facts = {}, rows = {} }
    local last = row + height - 1
    local footer_row = last
    local footer_rule = last - 1
    local y = row
    local facts = view.facts or {}
    for i = 1, #facts do
        if y >= footer_rule - 2 then
            break
        end
        write_span(y, col + 1, width - 2, facts[i], ATTR_PATH)
        y = y + 1
    end
    local top_rule = y
    if y < footer_rule then
        paint_hline_join(self, y)
        y = y + 1
    end
    local split = col + math.floor(width / 2)
    local left_w = math.max(8, split - col - 2)
    local right_w = math.max(8, (col + width) - split - 2)
    write_span(y, col + 1, left_w, "Migration", ATTR_COLHEAD)
    write_span(y, split + 1, right_w, "Database", ATTR_COLHEAD)
    y = y + 1
    local body_top = y
    local vis = footer_rule - body_top
    if vis < 1 then
        vis = 1
    end
    local flat = flatten_explore_rows(view, left_w)
    if not app.explore_cursor or app.explore_cursor < 1 then
        app.explore_cursor = next_pair_index(flat, 0, 1)
        if app.explore_cursor < 1 then
            app.explore_cursor = 1
        end
    end
    if app.explore_cursor > #flat then
        app.explore_cursor = #flat
    end
    local cursor = app.explore_cursor
    local scroll = app.explore_scroll or 0
    if cursor > 0 then
        if cursor <= scroll then
            scroll = math.max(0, cursor - 1)
        elseif cursor > scroll + vis then
            scroll = cursor - vis
        end
    end
    local max_scroll = math.max(0, #flat - vis)
    if scroll > max_scroll then
        scroll = max_scroll
    end
    if scroll < 0 then
        scroll = 0
    end
    app.explore_scroll = scroll
    app.explore_vis = vis
    app.explore_flat = flat
    app.explore_max_scroll = max_scroll
    for i = 1, vis do
        local idx = scroll + i
        local r = flat[idx]
        local rr = body_top + i - 1
        if not r then
            break
        end
        if r.kind == "label" then
            write_span(rr, col + 1, width - 2, r.text, ATTR_SECTION)
        else
            local attr = ATTR_PATH
            if idx == cursor then
                attr = ATTR_HL
                write_span(rr, col, width, string.rep(" ", width), attr)
            end
            local ltxt = r.left or ""
            local rtxt = r.right or ""
            if r.n then
                ltxt = string.format("%4d %s", r.n, ltxt)
            end
            write_span(rr, col + 1, left_w, ltxt, attr)
            write_span(rr, split + 1, right_w, rtxt, attr)
        end
    end
    paint_hline_join(self, footer_rule)
    paint_vline_join(self, split, top_rule, footer_rule)
    write_span(footer_row, col + 1, width - 2,
        "[q]/Esc back   j/k move highlight   PgUp/PgDn", ATTR_PROMPT)
end

local function note_content(self)
    local app = self.app
    local header = session_header(self.opts, app, self.inner_width)
    local dest = packet.packet_path(
        self.opts.packet_dir,
        self.opts.design,
        self.opts.engine,
        app.note_ref or 0)
    local lines = {
        { "Generate a migration packet", ATTR_TITLE },
        { "  next ref  " .. tostring(app.note_ref or "?"), ATTR_VERSION },
        { "  path      " .. dest, ATTR_PATH },
    }
    if app.note_finding then
        lines[#lines + 1] = { "  finding   " .. (app.note_finding.id or ""), ATTR_PATH }
        lines[#lines + 1] = { "  class     " .. (app.note_finding.class or ""), ATTR_PATH }
    end
    if app.warn_in_repo then
        lines[#lines + 1] = { "", ATTR_PATH }
        lines[#lines + 1] = {
            "Warning: packet path is inside the git tree",
            ATTR_ERR,
        }
    end
    lines[#lines + 1] = { "", ATTR_PATH }
    lines[#lines + 1] = { "Optional one-line note:", ATTR_SECTION }
    lines[#lines + 1] = { "  " .. (app.note_buf or ""), ATTR_PROMPT }
    paint_framed(self, header, lines,
        "Press Enter to write (empty note is OK)   ESC cancel")
end

local function chrome_content(self)
    local mode = self.app.mode
    if mode == "splash" then
        splash_content(self)
    elseif mode == "picker" then
        picker_content(self)
    elseif mode == "running" then
        running_content(self)
    elseif mode == "dashboard" then
        dashboard_content(self)
    elseif mode == "review" then
        review_content(self)
    elseif mode == "explore" then
        explore_content(self)
    elseif mode == "note" then
        note_content(self)
    else
        result_content(self)
    end
end

local function build_screen(opts, app)
    local body = Panel {
        name = "chrome",
        content = chrome_content,
        border = {
            format = t.draw.box_fmt.single,
            attr = { fg = "red", brightness = "bright" },
            title = " SchemaHelper ",
            title_attr = { fg = "yellow", brightness = "bright" },
        },
    }
    body.opts = opts
    body.app = app
    return Screen {
        body = body,
        name = "SchemaHelper",
    }
end

show_mode = function(screen, app, mode)
    app.mode = mode
    screen:calculate_layout()
    screen:render()
end

local function run_dashboard(screen, app, opts, state)
    app.state = state
    app.built = queue.build({
        out_dir = opts.out_dir,
        track = opts.track,
        state = state,
    })
    show_mode(screen, app, "dashboard")
    while true do
        local raw = t.input.readansi(0.2)
        if raw == nil then
            screen:check_resize(true)
        else
            local name = key_map[raw]
            if name == keys.enter then
                if app.built.totals.subject == 0 then
                    app.show_mode_msg = "Nothing to review"
                    show_mode(screen, app, "dashboard")
                else
                    app.review_index = 1
                    app.show_mode_msg = ""
                    return "review"
                end
            elseif raw == "r" then
                app.show_mode_msg = "Re-audit not implemented in this fixture path"
                show_mode(screen, app, "dashboard")
            elseif raw == "q" or name == keys.escape then
                return "quit"
            end
        end
    end
end

local function rebuild_queue(app, opts)
    app.state = queue.load_state(opts.state_file)
    app.built = queue.build({
        out_dir = opts.out_dir,
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

local function run_note(screen, app, opts)
    app.note_buf = app.note_buf or ""
    show_mode(screen, app, "note")
    while true do
        local raw = t.input.readansi(0.2)
        if raw == nil then
            screen:check_resize(true)
        else
            local name = key_map[raw]
            if name == keys.enter then
                return app.note_buf
            elseif name == keys.escape then
                return nil
            elseif name == keys.backspace or raw == "\127" or raw == "\8" then
                app.note_buf = app.note_buf:sub(1, -2)
                show_mode(screen, app, "note")
            elseif raw and #raw == 1 and raw:match("[%g ]") and raw ~= "\27" then
                if #app.note_buf < 200 then
                    app.note_buf = app.note_buf .. raw
                end
                show_mode(screen, app, "note")
            end
        end
    end
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
    local note = run_note(screen, app, opts)
    app.note_finding = nil
    if note == nil then
        app.show_mode_msg = "packet cancelled"
        return nil
    end
    local tool_ver = select(1, read_tool_version(opts.schematool))
    local detail_lines = queue.load_detail_section(opts.out_dir, f)
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
        schemahelper_version = VERSION,
        schematool_version = tool_ver or "",
    }, f, {
        note = note,
        detail_text = detail_text,
    })
    if not written then
        app.show_mode_msg = "error: " .. tostring(err)
        return nil
    end
    local ok, save_err = queue.save_decision(
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

local function run_review(screen, app, opts)
    app.review_index = app.review_index < 1 and 1 or app.review_index
    show_mode(screen, app, "review")
    while true do
        local raw = t.input.readansi(0.2)
        if raw == nil then
            screen:check_resize(true)
        else
            local name = key_map[raw]
            if raw == "e" then
                local explore = { "No finding selected" }
                if app.built and app.built.subject and
                   app.review_index >= 1 and
                   app.review_index <= #app.built.subject then
                     local f = app.built.subject[app.review_index]
                     app.explore_view = queue.build_explore_view(f)
                     explore = queue.explore_lines(
                        opts.out_dir,
                        f.id,
                        app.built.subject,
                        app.state)
                end
                app.explore_lines = explore
                app.explore_scroll = 0
                app.explore_cursor = 0
                app.review_stack[#app.review_stack + 1] = "review"
                return "explore"
            elseif raw == "s" then
                if app.built and app.built.subject then
                    local f = app.built.subject[app.review_index]
                    if f then
                        local ok, why = queue.save_decision(
                            opts.state_file, f.id, "skipped", nil)
                        if ok then
                            app.show_mode_msg = "skipped: " .. f.id
                        else
                            app.show_mode_msg = "error: " .. tostring(why)
                        end
                    end
                end
                show_mode(screen, app, "review")
            elseif raw == "a" then
                if app.built and app.built.subject then
                    local f = app.built.subject[app.review_index]
                    if f then
                        local ok, why = queue.save_decision(
                            opts.state_file, f.id, "accepted", nil)
                        if ok then
                            app.show_mode_msg = "accepted: " .. f.id
                        else
                            app.show_mode_msg = "error: " .. tostring(why)
                        end
                    end
                end
                show_mode(screen, app, "review")
            elseif raw == "g" then
                local next_mode = generate_packet(screen, app, opts)
                if next_mode then
                    return next_mode
                end
                show_mode(screen, app, "review")
            elseif raw == "n" or name == keys.down then
                if app.built and app.built.subject then
                    app.review_index = math.min(
                        app.review_index + 1, #app.built.subject)
                end
                show_mode(screen, app, "review")
            elseif raw == "p" or name == keys.up then
                app.review_index = math.max(1, app.review_index - 1)
                show_mode(screen, app, "review")
            elseif raw == "r" then
                local state = queue.load_state(opts.state_file)
                app.built = queue.build({
                    out_dir = opts.out_dir,
                    track = opts.track,
                    state = state,
                })
                app.review_index = math.min(
                    app.review_index, app.built.totals.subject)
                if app.review_index < 1 then
                    app.review_index = 1
                end
                app.show_mode_msg = "re-audited"
                show_mode(screen, app, "dashboard")
                return "dashboard"
            elseif name == keys.escape or raw == "q" then
                return "dashboard"
            end
        end
    end
end

local function run_explore(screen, app, _)
    show_mode(screen, app, "explore")
    while true do
        local raw = t.input.readansi(0.2)
        if raw == nil then
            screen:check_resize(true)
        else
            local name = key_map[raw]
            local step = app.explore_vis or 10
            if name == keys.escape or raw == "q" then
                app.explore_lines = nil
                app.explore_view = nil
                app.explore_flat = nil
                app.explore_scroll = 0
                app.explore_cursor = 0
                local prev = app.review_stack[#app.review_stack]
                app.review_stack[#app.review_stack] = nil
                if prev == "review" then
                    return "review"
                end
                return "dashboard"
            elseif name == keys.up or raw == "k" then
                local flat = app.explore_flat or {}
                app.explore_cursor = next_pair_index(
                    flat, app.explore_cursor or 1, -1)
                show_mode(screen, app, "explore")
            elseif name == keys.down or raw == "j" then
                local flat = app.explore_flat or {}
                app.explore_cursor = next_pair_index(
                    flat, app.explore_cursor or 0, 1)
                show_mode(screen, app, "explore")
            elseif name == keys.page_up or raw == "b" then
                local flat = app.explore_flat or {}
                local cur = app.explore_cursor or 1
                for _ = 1, step do
                    cur = next_pair_index(flat, cur, -1)
                end
                app.explore_cursor = cur
                show_mode(screen, app, "explore")
            elseif name == keys.page_down or raw == "f"
                or name == keys.enter then
                local flat = app.explore_flat or {}
                local cur = app.explore_cursor or 0
                for _ = 1, step do
                    cur = next_pair_index(flat, cur, 1)
                end
                app.explore_cursor = cur
                show_mode(screen, app, "explore")
            end
        end
    end
end

local function pick_wrapper(screen, app, opts)
    local list = discover_wrappers(script_dir())
    if #list == 0 then
        return nil, "no schematool_*.sh wrappers found"
    end
    app.picker.list = list
    app.picker.selected = 1
    show_mode(screen, app, "picker")
    while true do
        local raw = t.input.readansi(0.2)
        if raw == nil then
            screen:check_resize(true)
        else
            local name = key_map[raw]
            if name == keys.up then
                app.picker.selected = math.max(1, app.picker.selected - 1)
                show_mode(screen, app, "picker")
            elseif name == keys.down then
                app.picker.selected = math.min(#list, app.picker.selected + 1)
                show_mode(screen, app, "picker")
            elseif name == keys.enter then
                opts.wrapper = list[app.picker.selected].path
                return true
            elseif name == keys.escape then
                return nil, "cancelled"
            end
        end
    end
end

local function finish_paths(opts)
    local design, engine, schema = wrapper_meta(opts.wrapper)
    opts.design = design
    opts.engine = engine
    opts.schema = schema
    if opts.out_dir == "" then
        opts.out_dir = wrapper_dir(opts.wrapper)
    end
    ensure_dir(opts.out_dir)
    if opts.packet_dir == "" then
        opts.packet_dir = opts.out_dir
    end
    ensure_dir(opts.packet_dir)
    if opts.state_file == "" then
        opts.state_file = queue.default_state_path(opts.out_dir, design, engine)
    end
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

local function build_result_lines(opts, ran, exit_code, built, err)
    local lines = {}
    local function add(text, attr)
        lines[#lines + 1] = { text, attr }
    end
    if ran then
        add(string.format("SchemaTool exit %d  (0 clean / 2 drift / 3 anomaly)", exit_code),
            (exit_code == 0 or exit_code == 2 or exit_code == 3) and ATTR_VERSION or ATTR_ERR)
    elseif opts.reuse then
        add("Loaded existing artifacts (--reuse)", ATTR_VERSION)
    end
    if err then
        add(err, ATTR_ERR)
        if ran then
            local tail = log_tail(opts.out_dir .. "/schemahelper_schematool.log", 6)
            for i = 1, #tail do
                add(tail[i], ATTR_PATH)
            end
        end
    end
    if built and not err then
        local tot = built.totals
        add(string.format("Total migrations found     %d", tot.total), ATTR_PROMPT)
        add(string.format("Perfect migrations         %d", tot.perfect), ATTR_PROMPT)
        add(string.format("Accepted variations        %d", tot.accepted), ATTR_PROMPT)
        add(string.format("Subject for review         %d", tot.subject), ATTR_PROMPT)
        if tot.applied > 0 or tot.packet > 0 then
            add(string.format("Applied / packets          %d / %d", tot.applied, tot.packet),
                ATTR_PATH)
        end
        add("", ATTR_PATH)
        add("Variance classes (subject for review)", ATTR_SECTION)
        if #built.classes == 0 then
            add("  (none)", ATTR_PATH)
        else
            for i = 1, #built.classes do
                local c = built.classes[i]
                add(string.format("  %-28s %d", c.name, c.count), ATTR_PATH)
            end
        end
    end
    add("", ATTR_PATH)
    add("[w] pick another wrapper   [q]uit", ATTR_PROMPT)
    if built and queue.artifacts_present(opts.out_dir, opts.track) then
        add("[Enter] review existing artifacts", ATTR_PROMPT)
    end
    return lines
end

local function wait_result_action(screen, can_reuse)
    while true do
        local raw = t.input.readansi(0.2)
        if raw == nil then
            screen:check_resize(true)
        else
            local name = key_map[raw]
            if raw == "w" then
                return "picker"
            elseif raw == "q" or name == keys.escape then
                return "quit"
            elseif name == keys.enter and can_reuse then
                return "reuse"
            end
        end
    end
end

local function main()
    local opts = parse_args()
    t.cursor.visible.set(false)

    local app = {
        mode = "splash",
        picker = { list = {}, selected = 1 },
        result_lines = {},
        conn = nil,
        log = "(none)",
        status_note = "",
        built = nil,
        review_index = 0,
        review_stack = {},
        explore_lines = nil,
        explore_view = nil,
        explore_scroll = 0,
        explore_cursor = 0,
        show_mode_msg = "",
        catalog_degraded = false,
    }
    local screen = build_screen(opts, app)
    show_mode(screen, app, "splash")
    if wait_enter_or_esc(screen) == keys.escape then
        return
    end

    local state
    local built
    while true do
        if opts.wrapper == "" then
            local ok, pick_err = pick_wrapper(screen, app, opts)
            if not ok then
                if pick_err ~= "cancelled" then
                    io.stderr:write("Error: " .. tostring(pick_err) .. "\n")
                end
                return
            end
        end

        finish_paths(opts)
        app.warn_in_repo = packet.in_git_tree(opts.packet_dir)
        app.catalog_degraded = false
        app.log = opts.out_dir .. "/schemahelper_schematool.log"

        app.status_note = "Connecting…"
        show_mode(screen, app, "running")
        app.conn = probe_connect(opts)
        show_mode(screen, app, "running")

        local ran = false
        local exit_code = 0
        local err
        local have_art = queue.artifacts_present(opts.out_dir, opts.track)
        local should_run = not opts.reuse or not have_art
        if not app.conn.ok then
            should_run = false
            if not opts.reuse then
                err = "No database connection — SchemaTool not started"
            end
        end
        if should_run then
            app.status_note = "Running SchemaTool…"
            show_mode(screen, app, "running")
            exit_code, app.log = invoke_schematool(opts, screen, app)
            ran = true
        end

        local state_fh = io.open(opts.state_file, "r")
        if state_fh then
            state_fh:close()
        else
            queue.create_state(opts.state_file, opts.design, opts.engine, opts.schema)
        end
        state = queue.load_state(opts.state_file)

        built = queue.build({
            out_dir = opts.out_dir,
            track = opts.track,
            state = state,
        })
        local ok_exit = (exit_code == 0 or exit_code == 2 or exit_code == 3)
        local have_meta = queue.artifacts_present(opts.out_dir, "metadata")
        local have_cat = queue.artifacts_present(opts.out_dir, "catalog")
        if ran and not ok_exit then
            if opts.track == "catalog" and not have_cat then
                err = "SchemaTool failed; see log"
            elseif have_meta then
                app.catalog_degraded = true
                app.show_mode_msg = "Catalog track failed; metadata findings kept"
            else
                err = "SchemaTool failed; see log"
            end
        elseif ran and ok_exit and (opts.track == "both" or opts.track == "catalog")
            and not have_cat and have_meta then
            app.catalog_degraded = true
            app.show_mode_msg = "Catalog track failed; metadata findings kept"
        end

        if not err then
            break
        end
        have_art = queue.artifacts_present(opts.out_dir, opts.track)
        app.result_lines = build_result_lines(opts, ran, exit_code, built, err)
        show_mode(screen, app, "result")
        local action = wait_result_action(screen, have_art)
        if action == "quit" then
            return
        elseif action == "reuse" then
            opts.reuse = true
            break
        else
            opts.wrapper = ""
            opts.reuse = false
            app.conn = nil
        end
    end

    app.state = state
    app.built = built
    app.review_index = 1
    app.review_stack = {}
    app.explore_lines = nil
    app.explore_scroll = 0
    app.show_mode_msg = ""

    local mode = "dashboard"
    while mode ~= "quit" do
        if mode == "dashboard" then
            mode = run_dashboard(screen, app, opts, state)
        elseif mode == "review" then
            mode = run_review(screen, app, opts)
        elseif mode == "explore" then
            mode = run_explore(screen, app, opts)
        end
        if mode == "quit" then
            break
        end
    end
    queue.save_cursor(opts.state_file, "")
end

-- Handle --version before terminal initialization (no TTY needed)
do
    local _opts = parse_args()
    if _opts.version then
        local tool_ver, tool_date = read_tool_version(_opts.schematool)
        print(string.format("SchemaHelper %s (%s)", VERSION, RELEASED))
        print(string.format("SchemaTool %s (%s)",
            tool_ver or "unknown", tool_date or "unknown"))
        print(string.format("Lua %s", _opts.lua_version))
        return
    end
end

t.initwrap(main, {
    displaybackup = true,
    filehandle = io.stdout,
    skip_width_detection = true,
    disable_sigint = true,
    autotermrestore = true,
})()
