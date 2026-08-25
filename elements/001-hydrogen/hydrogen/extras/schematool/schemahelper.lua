-- schemahelper.lua
-- SchemaHelper — Interactive SchemaTool front-end (Lua 5.5 + terminal.lua)
-- Orchestrator: wires the focused modules under lua/ (const, ui, mouse,
-- paint, wrappers, invoke, screens, explore, queue, packet, apply, connect)
-- into the run loops and main entry point.
--
-- CHANGELOG
-- 0.5.8 - 2026-08-25 - Per-run /tmp work_dir for intermediates + cleanup on exit
-- 0.5.7 - 2026-08-24 - Mouseover highlighting on clickable options; click on release; capitalized option labels
-- 0.5.6 - 2026-08-24 - Mouse support: SGR 1006; click wrapper rows and click [key] actions
-- 0.5.5 - 2026-08-24 - Phase 7: catalog DDL apply (nullable / add column) with louder confirm (object.column)
-- 0.5.4 - 2026-08-24 - Phase 5 slice: confirmed orphan [U] DELETE (true orphans only)
-- 0.5.3 - 2026-08-24 - Dashboard/review [R] re-runs SchemaTool
-- 0.5.1 - 2026-08-23 - [U] labeled update; catalog review shows fold ref
-- 0.5.0 - 2026-08-23 - Phase 5: [U] one-field metadata apply
-- 0.4.14 - 2026-08-23 - Explore: Enter decodes brotli line; pageup/pagedown
-- 0.4.13 - 2026-08-23 - Explore: full field, both line nos, first-diff line
-- 0.4.12 - 2026-08-23 - Review: do not use Lua patterns to drop key lines
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

local function script_dir()
    local src = arg[0] or ""
    local dir = src:match("^(.*)/[^/]+$")
    return dir or "."
end

package.path = script_dir() .. "/lua/?.lua;" .. package.path

local C = require("schemahelper_const")
local Mouse = require("schemahelper_mouse")
local P = require("schemahelper_paint")
local W = require("schemahelper_wrappers")
local I = require("schemahelper_invoke")
local S = require("schemahelper_screens")
local E = require("schemahelper_explore")
local Q = require("schemahelper_queue")
local packet = require("schemahelper_packet")
local Actions = require("schemahelper_actions")

local t = C.t
local Screen = C.Screen
local Panel = C.Panel
local keys = C.keys
local ATTR = C.ATTR

local function parse_args()
    local opts = {
        wrapper = "",
        migrations = "",
        out_dir = "",
        work_dir = "",
        state_file = "",
        packet_dir = "",
        track = "both",
        reuse = false,
        allow_write = false,
        ref = 0,
        keep_work_dir = false,
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
        elseif a == "--work-dir" then
            opts.work_dir = arg[i + 1] or ""
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
        elseif a == "--keep-work-dir" then
            opts.keep_work_dir = true
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

-- Unified key reader: (raw, name, extra). Mouse hits synthesize the same
-- raw/name a keyboard press would yield; wrapper rows return extra.pick.
-- Motion events update the mouseover highlight and return nil so the loop
-- continues without acting; release events are processed as a click.
local function read_key(screen, _app)
    local raw = t.input.readansi(0.2)
    if raw == nil then
        return nil, nil, nil
    end
    if Mouse.is_mouse(raw) then
        local m = Mouse.parse_mouse(raw)
        if not m or m.btn ~= 0 then
            return "", "_ignore", nil
        end
        if not m.release then
            Mouse.highlight_hotspot(screen, m.x, m.y)
            return nil, nil, nil
        else
            Mouse.highlight_hotspot(screen, nil, nil)
            local v = Mouse.mouse_vkey(raw)
            if not v then
                return "", "_ignore", nil
            end
            if v.pick then
                return nil, nil, { pick = v.pick }
            end
            return v.raw or "", v.name or nil, nil
        end
    end
    return raw, C.key_map[raw], nil
end

local function wait_enter_or_esc(screen)
    while true do
        local raw, name = read_key(screen, nil)
        if raw == nil then
            if screen then
                screen:check_resize(true)
            end
        elseif name == keys.enter or name == keys.escape then
            return name
        end
    end
end

local show_mode
show_mode = function(screen, app, mode)
    app.mode = mode
    screen:calculate_layout()
    screen:render()
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
        P.write_span(y, col + 1, width - 2, facts[i], ATTR.PATH)
        y = y + 1
    end
    local top_rule = y
    if y < footer_rule then
        P.paint_hline_join(self, y)
        y = y + 1
    end
    local layout = view.layout or "both"
    local single = layout == "left" or layout == "right"
    local split = col + math.floor(width / 2)
    if single then
        split = col + width
    end
    local left_w = math.max(8, split - col - 2)
    local right_w = math.max(8, (col + width) - split - 2)
    if single then
        left_w = math.max(8, width - 2)
        right_w = left_w
    end
    local maxn = 0
    local vrows = view.rows or {}
    for i = 1, #vrows do
        if vrows[i].n and vrows[i].n > maxn then
            maxn = vrows[i].n
        end
    end
    local nw = math.max(3, #tostring(maxn))
    local gutter = nw + 1
    local left_text_w = math.max(4, left_w - gutter)
    local right_text_w = math.max(4, right_w - gutter)
    local lhead = "Migration"
    local rhead = "Database"
    if view.decoded then
        lhead = "Migration (decoded)"
        rhead = "Database (decoded)"
    end
    if layout == "left" then
        P.write_span(y, col + 1, gutter, string.rep(" ", gutter), ATTR.COLHEAD)
        P.write_span(y, col + 1 + gutter, left_text_w, lhead, ATTR.COLHEAD)
    elseif layout == "right" then
        P.write_span(y, col + 1, gutter, string.rep(" ", gutter), ATTR.COLHEAD)
        P.write_span(y, col + 1 + gutter, left_text_w, rhead, ATTR.COLHEAD)
    else
        P.write_span(y, col + 1, gutter, string.rep(" ", gutter), ATTR.COLHEAD)
        P.write_span(y, col + 1 + gutter, left_text_w, lhead, ATTR.COLHEAD)
        P.write_span(y, split + 1, gutter, string.rep(" ", gutter), ATTR.COLHEAD)
        P.write_span(y, split + 1 + gutter, right_text_w, rhead, ATTR.COLHEAD)
    end
    y = y + 1
    local body_top = y
    local vis = footer_rule - body_top
    if vis < 1 then
        vis = 1
    end
    local flat = E.flatten_explore_rows(view, left_text_w, right_text_w)
    if not app.explore_cursor or app.explore_cursor < 1 then
        app.explore_cursor = E.first_diff_line_index(flat, view.first_diff)
        if app.explore_cursor < 1 then
            app.explore_cursor = 1
        end
    end
    if app.explore_cursor > #flat then
        app.explore_cursor = #flat
    end
    local cursor = app.explore_cursor
    local cursor_line = flat[cursor] and flat[cursor].line
    local line_end = cursor
    if cursor_line then
        while line_end < #flat and flat[line_end + 1]
            and flat[line_end + 1].line == cursor_line do
            line_end = line_end + 1
        end
    end
    local scroll = app.explore_scroll or 0
    if cursor > 0 then
        if cursor <= scroll then
            scroll = math.max(0, cursor - 1)
        elseif line_end > scroll + vis then
            scroll = line_end - vis
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
    local num_fmt = "%" .. tostring(nw) .. "d"
    local blank_num = string.rep(" ", nw)
    for i = 1, vis do
        local idx = scroll + i
        local r = flat[idx]
        local rr = body_top + i - 1
        if not r then
            break
        end
        if r.kind == "label" then
            P.write_span(rr, col + 1, width - 2, r.text, ATTR.SECTION)
        else
            local attr = ATTR.PATH
            local num_attr = ATTR.DATE
            if cursor_line and r.line == cursor_line then
                attr = ATTR.HL
                num_attr = ATTR.HL
                P.write_span(rr, col, width, string.rep(" ", width), attr)
            end
            local lnum = r.n and string.format(num_fmt, r.n) or blank_num
            local rnum = lnum
            if single then
                local txt = (layout == "right") and (r.right or "") or (r.left or "")
                P.write_span(rr, col + 1, nw, lnum, num_attr)
                P.write_span(rr, col + 1 + gutter, left_text_w, txt, attr)
            else
                P.write_span(rr, col + 1, nw, lnum, num_attr)
                P.write_span(rr, col + 1 + gutter, left_text_w, r.left or "", attr)
                P.write_span(rr, split + 1, nw, rnum, num_attr)
                P.write_span(rr, split + 1 + gutter, right_text_w, r.right or "", attr)
            end
        end
    end
    P.paint_hline_join(self, footer_rule)
    if not single then
        P.paint_vline_join(self, split, top_rule, footer_rule)
    end
    local pair = E.explore_current_pair(app)
    local can_decode = pair and not view.decoded
        and (Q.has_embed(pair.left) or Q.has_embed(pair.right))
    local foot = "[Q]/Esc back   j/k line   PgUp/PgDn"
    if can_decode then
        foot = foot .. "   Enter decode"
    end
    P.write_span(footer_row, col + 1, width - 2, foot, ATTR.PROMPT)
end

local function chrome_content(self)
    local mode = self.app.mode
    if mode == "splash" then
        S.splash_content(self)
    elseif mode == "picker" then
        S.picker_content(self)
    elseif mode == "running" then
        S.running_content(self)
    elseif mode == "dashboard" then
        S.dashboard_content(self)
    elseif mode == "review" then
        S.review_content(self)
    elseif mode == "explore" then
        explore_content(self)
    elseif mode == "note" then
        S.note_content(self)
    elseif mode == "apply" then
        S.apply_content(self)
    else
        S.result_content(self)
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

local function ingest_audit(app, opts, ran, exit_code)
    return Actions.ingest_audit(app, opts, ran, exit_code)
end

local function reaudit(screen, app, opts)
    return Actions.reaudit(screen, app, opts)
end

local function apply_finding(screen, app, opts)
    return Actions.apply_finding(screen, app, opts)
end

local function generate_packet(screen, app, opts)
    return Actions.generate_packet(screen, app, opts)
end

local function promote_finding(screen, app, opts)
    return Actions.promote_finding(screen, app, opts)
end

local function run_dashboard(screen, app, opts, state)
    app.state = state
    app.built = Q.build({
        out_dir = opts.work_dir,
        track = opts.track,
        state = state,
    })
    show_mode(screen, app, "dashboard")
    while true do
        local raw, name = read_key(screen, app)
        if raw == nil then
            screen:check_resize(true)
        else
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
                reaudit(screen, app, opts)
                show_mode(screen, app, "dashboard")
            elseif raw == "q" or name == keys.escape then
                return "quit"
            end
        end
    end
end

local function run_note(screen, app, opts)
    app.note_buf = app.note_buf or ""
    show_mode(screen, app, "note")
    while true do
        local raw, name = read_key(screen, app)
        if raw == nil then
            screen:check_resize(true)
        else
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

local function run_apply_confirm(screen, app)
    app.apply_buf = app.apply_buf or ""
    show_mode(screen, app, "apply")
    while true do
        local raw, name = read_key(screen, app)
        if raw == nil then
            screen:check_resize(true)
        else
            if name == keys.enter then
                return app.apply_buf
            elseif name == keys.escape then
                return nil
            elseif name == keys.backspace or raw == "\127" or raw == "\8" then
                app.apply_buf = app.apply_buf:sub(1, -2)
                show_mode(screen, app, "apply")
            elseif raw and #raw == 1 and raw:match("[%g]") and raw ~= "\27" then
                if #app.apply_buf < 80 then
                    app.apply_buf = app.apply_buf .. raw
                end
                show_mode(screen, app, "apply")
            end
        end
    end
end

local function run_review(screen, app, opts)
    app.review_index = app.review_index < 1 and 1 or app.review_index
    show_mode(screen, app, "review")
    while true do
        local raw, name = read_key(screen, app)
        if raw == nil then
            screen:check_resize(true)
        else
            if raw == "e" then
                local explore = { "No finding selected" }
                if app.built and app.built.subject and
                   app.review_index >= 1 and
                   app.review_index <= #app.built.subject then
                      local f = app.built.subject[app.review_index]
                      app.explore_view = Q.build_explore_view(f)
                      explore = Q.explore_lines(
                          opts.work_dir,
                          f.id,
                          app.built.subject,
                          app.state)
                end
                app.explore_lines = explore
                app.explore_scroll = 0
                app.explore_cursor = 0
                app.explore_stack = {}
                app.review_stack[#app.review_stack + 1] = "review"
                return "explore"
            elseif raw == "s" then
                if app.built and app.built.subject then
                    local f = app.built.subject[app.review_index]
                    if f then
                        local ok, why = Q.save_decision(
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
                        local ok, why = Q.save_decision(
                            opts.state_file, f.id, "accepted", nil)
                        if ok then
                            app.show_mode_msg = "accepted: " .. f.id
                        else
                            app.show_mode_msg = "error: " .. tostring(why)
                        end
                    end
                end
                show_mode(screen, app, "review")
            elseif raw == "u" then
                local next_mode = apply_finding(screen, app, opts)
                if next_mode then
                    return next_mode
                end
                show_mode(screen, app, "review")
            elseif raw == "g" then
                local next_mode = generate_packet(screen, app, opts)
                if next_mode then
                    return next_mode
                end
                show_mode(screen, app, "review")
            elseif raw == "m" then
                local next_mode = promote_finding(screen, app, opts)
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
                reaudit(screen, app, opts)
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
        local raw, name = read_key(screen, app)
        if raw == nil then
            screen:check_resize(true)
        else
            local step = app.explore_vis or 10
            if name == keys.escape or raw == "q" then
                local stack = app.explore_stack or {}
                if #stack > 0 then
                    local prevv = stack[#stack]
                    stack[#stack] = nil
                    app.explore_view = prevv.view
                    app.explore_cursor = prevv.cursor or 0
                    app.explore_scroll = prevv.scroll or 0
                    show_mode(screen, app, "explore")
                else
                    app.explore_lines = nil
                    app.explore_view = nil
                    app.explore_flat = nil
                    app.explore_stack = {}
                    app.explore_scroll = 0
                    app.explore_cursor = 0
                    local prev = app.review_stack[#app.review_stack]
                    app.review_stack[#app.review_stack] = nil
                    if prev == "review" then
                        return "review"
                    end
                    return "dashboard"
                end
            elseif name == keys.enter then
                local view = app.explore_view or {}
                if not view.decoded then
                    local pair = E.explore_current_pair(app)
                    local nextv = pair and Q.build_line_decode_view(
                        pair.left, pair.right)
                    if nextv then
                        local stack = app.explore_stack or {}
                        stack[#stack + 1] = {
                            view = app.explore_view,
                            cursor = app.explore_cursor,
                            scroll = app.explore_scroll,
                        }
                        app.explore_stack = stack
                        app.explore_view = nextv
                        app.explore_cursor = 0
                        app.explore_scroll = 0
                        show_mode(screen, app, "explore")
                    end
                end
            elseif name == keys.up or raw == "k" then
                local flat = app.explore_flat or {}
                app.explore_cursor = E.next_line_index(
                    flat, app.explore_cursor or 1, -1)
                show_mode(screen, app, "explore")
            elseif name == keys.down or raw == "j" then
                local flat = app.explore_flat or {}
                app.explore_cursor = E.next_line_index(
                    flat, app.explore_cursor or 0, 1)
                show_mode(screen, app, "explore")
            elseif name == keys.pageup or raw == "b" then
                local flat = app.explore_flat or {}
                local cur = app.explore_cursor or 1
                for _ = 1, step do
                    cur = E.next_line_index(flat, cur, -1)
                end
                app.explore_cursor = cur
                show_mode(screen, app, "explore")
            elseif name == keys.pagedown or raw == "f" then
                local flat = app.explore_flat or {}
                local cur = app.explore_cursor or 0
                for _ = 1, step do
                    cur = E.next_line_index(flat, cur, 1)
                end
                app.explore_cursor = cur
                show_mode(screen, app, "explore")
            end
        end
    end
end

local function pick_wrapper(screen, app, opts)
    local list = W.discover_wrappers(script_dir())
    if #list == 0 then
        return nil, "no schematool_*.sh wrappers found"
    end
    app.picker.list = list
    app.picker.selected = 1
    show_mode(screen, app, "picker")
    while true do
        local raw, name, mextra = read_key(screen, app)
        if raw == nil and not mextra then
            screen:check_resize(true)
        elseif mextra and mextra.pick then
            local idx = mextra.pick
            if idx >= 1 and idx <= #list then
                app.picker.selected = idx
                opts.wrapper = list[idx].path
                return true
            end
        else
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
    local design, engine, schema = W.wrapper_meta(opts.wrapper)
    opts.design = design
    opts.engine = engine
    opts.schema = schema
    if opts.out_dir == "" then
        opts.out_dir = W.wrapper_dir(opts.wrapper)
    end
    W.ensure_dir(opts.out_dir)
    if opts.work_dir == "" then
        local tmp = os.getenv("TMPDIR") or "/tmp"
        local stamp = os.date("!%Y%m%dT%H%M%SZ")
        local rand = tostring(math.random(100000, 999999))
        opts.work_dir = string.format("%s/schemahelper-%s-%s",
            tmp, stamp, rand)
    end
    W.ensure_dir(opts.work_dir)
    if opts.packet_dir == "" then
        opts.packet_dir = opts.out_dir
    end
    W.ensure_dir(opts.packet_dir)
    if opts.state_file == "" then
        opts.state_file = Q.default_state_path(opts.out_dir, design, engine)
    end
end

local function cleanup_work_dir(opts)
    if opts and opts.work_dir and opts.work_dir ~= ""
        and not opts.keep_work_dir then
        os.execute('rm -rf "' .. opts.work_dir:gsub('"', '\\"') .. '"')
    end
end

local function wait_result_action(screen, can_reuse)
    while true do
        local raw, name = read_key(screen, nil)
        if raw == nil then
            screen:check_resize(true)
        else
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
    Actions.init({
        show_mode = show_mode,
        read_key = read_key,
        run_apply_confirm = run_apply_confirm,
        run_note = run_note,
    })
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
        explore_stack = {},
        show_mode_msg = "",
        catalog_degraded = false,
    }
    local screen = build_screen(opts, app)
    show_mode(screen, app, "splash")
    Mouse.enable_mouse()
    if wait_enter_or_esc(screen) == keys.escape then
        Mouse.disable_mouse()
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
                Mouse.disable_mouse()
                return
            end
        end

        finish_paths(opts)
        app.warn_in_repo = packet.in_git_tree(opts.packet_dir)
        app.catalog_degraded = false
        app.log = opts.work_dir .. "/schemahelper_schematool.log"

        app.status_note = "Connecting…"
        show_mode(screen, app, "running")
        app.conn = I.probe_connect(opts)
        show_mode(screen, app, "running")

        local ran = false
        local exit_code = 0
        local err
        local have_art = Q.artifacts_present(opts.work_dir, opts.track)
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
            exit_code, app.log = I.invoke_schematool(opts, screen, app, show_mode)
            ran = true
        end

        local ingest_err = ingest_audit(app, opts, ran, exit_code)
        state = app.state
        built = app.built
        if not err then
            err = ingest_err
        end

        if not err then
            break
        end
        have_art = Q.artifacts_present(opts.work_dir, opts.track)
        app.result_lines = I.build_result_lines(opts, ran, exit_code, built, err, Q, ATTR.OK)
        show_mode(screen, app, "result")
        local action = wait_result_action(screen, have_art)
        if action == "quit" then
            Mouse.disable_mouse()
            cleanup_work_dir(opts)
            return
        elseif action == "reuse" then
            opts.reuse = true
            break
        else
            cleanup_work_dir(opts)
            opts.wrapper = ""
            opts.out_dir = ""
            opts.work_dir = ""
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
    Q.save_cursor(opts.state_file, "")
    Mouse.disable_mouse()
    cleanup_work_dir(opts)
end

-- Handle --version before terminal initialization (no TTY needed)
do
    local _opts = parse_args()
    if _opts.version then
        local tool_ver, tool_date = W.read_tool_version(_opts.schematool)
        print(string.format("SchemaHelper %s (%s)", C.VERSION, C.RELEASED))
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
