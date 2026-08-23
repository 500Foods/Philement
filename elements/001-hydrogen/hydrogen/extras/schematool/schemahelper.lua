-- schemahelper.lua
-- SchemaHelper — Interactive SchemaTool front-end (Lua 5.5 + terminal.lua)
--
-- CHANGELOG
-- 0.2.2 - 2026-08-23 - Session header + live connect probe
-- 0.2.1 - 2026-08-23 - Persistent chrome; wrapper picker inset at top
-- 0.2.0 - 2026-08-23 - Phase 1: picker, SchemaTool invoke, queue result screen
-- 0.1.0 - 2026-08-23 - Splash: real versions/dates, wrapper paths, Esc exits

-- luacheck: globals arg package

local VERSION = "0.2.2"
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

local function script_dir()
    local src = arg[0] or ""
    local dir = src:match("^(.*)/[^/]+$")
    return dir or "."
end

package.path = script_dir() .. "/lua/?.lua;" .. package.path
local queue = require("schemahelper_queue")
local connect = require("schemahelper_connect")

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
        schematool = script_dir() .. "/schematool.sh",
        lua_version = _VERSION:match("Lua%s+([%d.]+)") or _VERSION,
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
    local design = "acuranzo"
    local engine = wrapper_engine(path)
    local schema = ""
    local f = io.open(path, "r")
    if f then
        for line in f:lines() do
            local d = line:match("%-%-design%s+([%w_]+)")
            if d then
                design = d
            end
            local s = line:match('SCHEMATOOL_DB_SCHEMA="([^"]*)"')
            if s then
                schema = s
            end
        end
        f:close()
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

local function invoke_schematool(opts)
    local log = opts.out_dir .. "/schemahelper_schematool.log"
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
    local cmd = table.concat(parts, " ") .. " > " .. sh_quote(log) .. " 2>&1"
    local ok, why, code = os.execute(cmd)
    if ok == true then
        return 0, log
    end
    if why == "exit" then
        return code or 1, log
    end
    return 1, log
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
    return "fail  " .. family .. target .. "  (" .. conn.detail .. ")", ATTR_ERR
end

local function session_header(opts, app, width)
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
    local rule_w = math.max(1, (width or 72) - 4)
    add(string.rep("─", rule_w), ATTR_SUB)
    return lines
end

local function swidth(text)
    return t.text.width.utf8swidth(text)
end

local function write_span(row, col, width, text, attr)
    if width < 1 then
        return
    end
    local shown = t.text.width.truncate_ellipsis(width, text, "right")
    t.cursor.position.set(row, col)
    t.output.write(t.text.push_seq(attr), shown, t.text.pop_seq())
end

local function write_centered(row, col, width, text, attr, trunc)
    local shown = t.text.width.truncate_ellipsis(width, text, trunc or "right")
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

local function paint_top_lines(self, lines)
    local row = self.inner_row
    local col = self.inner_col
    local height = self.inner_height
    local width = self.inner_width
    if height < 1 or width < 1 then
        return
    end
    local last = row + height - 1
    local start = row + 1
    for i = 1, #lines do
        local line_row = start + i - 1
        if line_row > last then
            break
        end
        local spec = lines[i]
        write_span(line_row, col + 2, width - 4, spec[1], spec[2] or ATTR_PATH)
    end
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
    lines[#lines + 1] = { "", ATTR_PATH }
    lines[#lines + 1] = { "Press Enter to select", ATTR_PROMPT }
    lines[#lines + 1] = { "Press ESC to exit", ATTR_PROMPT }
    paint_top_lines(self, lines)
end

local function running_content(self)
    local lines = session_header(self.opts, self.app, self.inner_width)
    lines[#lines + 1] = { "", ATTR_PATH }
    lines[#lines + 1] = { self.app.status_note or "Working…", ATTR_TITLE }
    paint_top_lines(self, lines)
end

local function result_content(self)
    local lines = session_header(self.opts, self.app, self.inner_width)
    local body = self.app.result_lines or {}
    for i = 1, #body do
        lines[#lines + 1] = body[i]
    end
    paint_top_lines(self, lines)
end

local function chrome_content(self)
    local mode = self.app.mode
    if mode == "splash" then
        splash_content(self)
    elseif mode == "picker" then
        picker_content(self)
    elseif mode == "running" then
        running_content(self)
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

local function show_mode(screen, app, mode)
    app.mode = mode
    screen:calculate_layout()
    screen:render()
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
    add("Press Enter or ESC to exit", ATTR_PROMPT)
    return lines
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
    }
    local screen = build_screen(opts, app)
    show_mode(screen, app, "splash")
    if wait_enter_or_esc(screen) == keys.escape then
        return
    end

    if opts.wrapper == "" then
        local ok, err = pick_wrapper(screen, app, opts)
        if not ok then
            if err ~= "cancelled" then
                io.stderr:write("Error: " .. tostring(err) .. "\n")
            end
            return
        end
    end

    finish_paths(opts)
    app.log = opts.out_dir .. "/schemahelper_schematool.log"

    app.status_note = "Connecting…"
    show_mode(screen, app, "running")
    app.conn = probe_connect(opts)
    show_mode(screen, app, "running")

    local ran = false
    local exit_code = 0
    local err
    local should_run = not opts.reuse or not queue.artifacts_present(opts.out_dir, opts.track)
    if not app.conn.ok then
        should_run = false
        if not opts.reuse then
            err = "No database connection — SchemaTool not started"
        end
    end
    if should_run then
        app.status_note = "Running SchemaTool…"
        show_mode(screen, app, "running")
        exit_code, app.log = invoke_schematool(opts)
        ran = true
    end

    local state_fh = io.open(opts.state_file, "r")
    if state_fh then
        state_fh:close()
    else
        queue.create_state(opts.state_file, opts.design, opts.engine, opts.schema)
    end
    local state = queue.load_state(opts.state_file)

    local built = queue.build({
        out_dir = opts.out_dir,
        track = opts.track,
        state = state,
    })
    if ran and exit_code ~= 0 and exit_code ~= 2 and exit_code ~= 3 then
        err = "SchemaTool failed; see log"
    end

    app.result_lines = build_result_lines(opts, ran, exit_code, built, err)
    show_mode(screen, app, "result")
    wait_enter_or_esc(screen)
end

t.initwrap(main, {
    displaybackup = true,
    filehandle = io.stdout,
    skip_width_detection = true,
})()
