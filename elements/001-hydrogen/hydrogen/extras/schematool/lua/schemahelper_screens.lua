-- schemahelper_screens.lua
-- Content painters for each SchemaHelper screen plus the screen builder.
-- Each *_content(self) matches the Panel content contract expected by
-- terminal.lua. The chrome dispatcher and the live explore painter stay in
-- schemahelper.lua (the orchestrator) because they are tightly coupled to the
-- explore cursor + shared hotspot state; they are passed into build_screen.
--
-- CHANGELOG
-- 0.6.5 - 2026-09-09 - Dashboard accepted list + [X] un-accept
-- 0.6.3 - 2026-09-08 - SchemaTool eighths bar + issue list
-- 0.6.1 - 2026-09-08 - Splash has no Instance block
-- 0.6.0 - 2026-09-08 - Instance header on every screen
-- 0.5.8 - 2026-08-25 - Extracted from schemahelper.lua (screen painters)

local C = require("schemahelper_const")
local P = require("schemahelper_paint")
local W = require("schemahelper_wrappers")
local Q = require("schemahelper_queue")
local packet = require("schemahelper_packet")
local apply = require("schemahelper_apply")
local I = require("schemahelper_invoke")

local t = C.t
local ATTR = C.ATTR

-- Mirrors schemahelper.lua's packet_next: compute next ref and surface a
-- collision reason if the ref is already taken.
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

local function splash_content(self)
    local col = self.inner_col
    local height = self.inner_height
    local width = self.inner_width
    if height < 1 or width < 1 then
        return
    end
    local UI = require("schemahelper_ui")
    UI.hotspots_reset()
    local row = self.inner_row
    local opts = self.opts
    local tool_ver, tool_date = W.read_tool_version(opts.schematool)
    local term_ver = t._VERSION or "0.1.0"
    local versions = {
        { "SchemaHelper", C.VERSION, C.RELEASED, ATTR.TITLE },
        { "SchemaTool", tool_ver or "?", tool_date or "", ATTR.TITLE },
        { "Lua", opts.lua_version, C.LUA_RELEASES[opts.lua_version] or "", ATTR.RUNTIME },
        { "terminal.lua", term_ver, C.TERMINAL_RELEASES[term_ver] or "", ATTR.RUNTIME },
    }

    local name_w = 0
    local ver_w = 0
    local date_w = 0
    for i = 1, #versions do
        name_w = math.max(name_w, P.swidth(versions[i][1]))
        ver_w = math.max(ver_w, P.swidth(versions[i][2]))
        date_w = math.max(date_w, P.swidth(versions[i][3]))
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
        { "text", "Welcome to SchemaHelper", ATTR.TITLE },
        { "text", "Frontend to Schema Tool", ATTR.SUB },
        { "blank" },
        { "version", 1 },
        { "version", 2 },
        { "version", 3 },
        { "version", 4 },
        { "blank" },
        { "text", "Migration Comparator", ATTR.SECTION },
        { "path", migrations },
        { "blank" },
        { "text", "Database Comparator", ATTR.SECTION },
        { "path", wrapper },
        { "blank" },
        { "text", "[Enter] continue", ATTR.PROMPT },
        { "text", "[ESC] exit", ATTR.PROMPT },
    }

    local start = row + math.max(0, math.floor((height - #lines) / 2))

    for i = 1, #lines do
        local line_row = start + i - 1
        if line_row > last then
            break
        end
        local spec = lines[i]
        if spec[1] == "text" then
            P.write_centered(line_row, col, width, spec[2], spec[3], "right")
            local shown = P.display_text(spec[2])
            local w = P.swidth(shown)
            local startc = col + math.max(0, math.floor((width - w) / 2))
            P.add_hotspots_for_line(spec[2], line_row, startc - 1)
            P.paint_hotspot_highlight_for_row(line_row, col, width, spec[2], spec[3])
        elseif spec[1] == "path" then
            P.write_centered(line_row, col, width, spec[2], ATTR.PATH, "left")
        elseif spec[1] == "version" then
            local item = versions[spec[2]]
            local name_pad = string.rep(" ", name_w - P.swidth(item[1]))
            P.write_span(line_row, table_col, name_w, name_pad .. item[1], item[4])
            if date_col <= col + width then
                P.write_span(line_row, ver_col, ver_w, item[2], ATTR.VERSION)
                if item[3] ~= "" and date_col + date_w <= col + width then
                    P.write_span(line_row, date_col, date_w, item[3], ATTR.DATE)
                end
            end
        end
    end
end

local function picker_content(self)
    local app = self.app
    local list = app.picker.list
    local selected = app.picker.selected
    local header = I.instance_block(self.opts, app, self.inner_width)
    local lines = {
        { "Select SchemaTool target", ATTR.TITLE },
        { "", ATTR.PATH },
    }
    for i, item in ipairs(list) do
        local mark = i == selected and "● " or "○ "
        local attr = i == selected and ATTR.TITLE or ATTR.PATH
        lines[#lines + 1] = { mark .. W.wrapper_label(item), attr }
    end
    local hotrows = {}
    for i = 1, #list do
        hotrows[i + 2] = "PICK:" .. i
    end
    P.paint_framed(self, header, lines, "[Enter] select   [ESC] exit", hotrows)
end

local function running_content(self)
    local app = self.app
    local header = I.instance_block(self.opts, app, self.inner_width)
    local body = {
        { app.status_note or "Working…", ATTR.TITLE },
    }
    local prog = app.progress
    if prog then
        local label = "  phase    " .. (prog.phase or "starting")
        if prog.phase == "expect" and prog.total and prog.total > 0 then
            local desc = prog.name
            if not desc or desc == "" then
                desc = prog.ref and ("ref " .. tostring(prog.ref)) or ""
            end
            label = string.format("  expect   %d / %d", prog.current or 0, prog.total)
            if desc ~= "" then
                label = label .. "   " .. desc
            end
        elseif prog.phase == "compare" then
            label = string.format("  compare  %d / %d",
                prog.compare_current or prog.current or 0, prog.total or 0)
        elseif prog.total and prog.total > 0 and (prog.current or 0) > 0 then
            label = string.format("  %s     %d / %d",
                prog.phase or "work", prog.current, prog.total)
        end
        body[#body + 1] = { label, ATTR.VERSION }
        body[#body + 1] = { spans = I.progress_bar_spans(prog) }
        local issues = prog.issues or {}
        local height = self.inner_height or 20
        local vis = math.max(1, height - (#header + 6))
        prog.issue_vis = vis
        local n = #issues
        local scroll = prog.issue_scroll or 0
        local max_off = math.max(0, n - vis)
        if scroll > max_off then
            scroll = max_off
            prog.issue_scroll = scroll
        end
        local last = n - scroll
        local first = math.max(1, last - vis + 1)
        if last >= first then
            for i = first, last do
                local it = issues[i]
                local text = string.format("  ref %-6s  %s",
                    tostring(it.ref or "-"), it.class)
                body[#body + 1] = { text, I.issue_attr(it.class) }
            end
        end
    end
    P.paint_framed(self, header, body, "Running SchemaTool…   [j]/[k] issues")
end

local function result_content(self)
    local header = I.session_header(self.opts, self.app, self.inner_width)
    local raw = self.app.result_lines or {}
    local body = {}
    local footer = "[W] pick another wrapper   [Q]uit"
    for i = 1, #raw do
        local text = raw[i][1] or ""
        if text:match("^%[W%]") or text:match("^%[Enter%]") then
            if text:match("%[Enter%]") then
                footer = footer .. "   [Enter] review artifacts"
            end
        else
            body[#body + 1] = raw[i]
        end
    end
    P.paint_framed(self, header, body, footer)
end

local function dashboard_content(self)
    local app = self.app
    local header = I.session_header(self.opts, app, self.inner_width)
    local reserved = packet.list_reserved(
        self.opts.packet_dir, self.opts.design, self.opts.engine)
    local dash_lines, built = Q.build_dashboard_lines({
        out_dir = self.opts.work_dir,
        track = self.opts.track,
        state = app.state,
        reserved = reserved,
    })
    app.built = built
    local acc = built.accepted or {}
    local sel = app.accepted_index or 1
    if sel < 1 then
        sel = 1
    elseif #acc > 0 and sel > #acc then
        sel = #acc
    end
    app.accepted_index = sel
    local body = {}
    for i = 1, #dash_lines do
        body[#body + 1] = { dash_lines[i], ATTR.PATH }
        if dash_lines[i]:match("^Accepted variations") then
            if #acc == 0 then
                body[#body + 1] = { "  (none)", ATTR.PATH }
            else
                for j = 1, #acc do
                    local mark = j == sel and "● " or "○ "
                    local attr = j == sel and ATTR.TITLE or ATTR.PATH
                    body[#body + 1] = { "  " .. mark .. acc[j].id, attr }
                end
            end
        end
    end
    if app.warn_in_repo then
        body[#body + 1] = { "", ATTR.PATH }
        body[#body + 1] = {
            "Warning: packet path is inside the git tree",
            ATTR.ERR,
        }
    end
    if app.catalog_degraded then
        body[#body + 1] = { "", ATTR.PATH }
        body[#body + 1] = {
            "Catalog track failed; metadata findings kept",
            ATTR.ERR,
        }
    end
    local msg = app.show_mode_msg or ""
    if msg ~= "" and msg ~= "Catalog track failed; metadata findings kept" then
        body[#body + 1] = { msg, ATTR.OK }
    end
    local footer = "[Enter] begin review   [R]e-audit   [Q]uit"
    if #acc > 0 then
        footer = "[Enter] begin review   [X] un-accept   [R]e-audit   [Q]uit"
    end
    P.paint_framed(self, header, body, footer)
end

local function is_review_key_line(line)
    if line:sub(1, 21) == "What would you like t" then
        return true
    end
    if line:sub(1, 3) ~= "  [" then
        return false
    end
    local key = line:sub(4, 5)
    local kl = key:lower()
    return kl == "e]" or kl == "s]" or kl == "a]" or kl == "u]"
        or kl == "g]" or kl == "n]" or kl == "p]"
        or kl == "m]"
end

local function review_content(self)
    local app = self.app
    local header = I.session_header(self.opts, app, self.inner_width)
    if not app.built or not app.built.subject then
        P.paint_framed(self, header,
            { { "No findings to review", ATTR.TITLE } },
            "[Q]uit to dashboard")
        return
    end
    local subj = app.built.subject
    if #subj == 0 then
        P.paint_framed(self, header,
            { { "All findings reviewed — none for review", ATTR.TITLE } },
            "[Q]uit to dashboard")
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
    local u_reason = apply.refuse_reason(finding, self.opts.allow_write)
    local review_lines = Q.build_review_lines_detailed(
        finding, self.opts.work_dir, app.state, next_ref,
        g_reason, u_reason, self.opts.allow_write)
    local body = {}
    for i = 1, #review_lines do
        local line = review_lines[i]
        if not is_review_key_line(line) then
            body[#body + 1] = { line, ATTR.PATH }
        end
    end
    local u_hint
    if u_reason then
        u_hint = ""
    else
        u_hint = " [U]pdate Database"
    end
    local m_label = ""
    if self.opts.allow_write then
        m_label = " [M] Promote"
    end
    P.paint_framed(self, header, body,
        string.format("[%d of %d]  [E]xplore [S]kip [A]ccept%s [G]enerate Migration%s  [N]ext/[P]rev  [Q]uit",
            idx, #subj, u_hint, m_label))
end

local function note_content(self)
    local app = self.app
    local header = I.session_header(self.opts, app, self.inner_width)
    local dest = packet.packet_path(
        self.opts.packet_dir,
        self.opts.design,
        self.opts.engine,
        app.note_ref or 0)
    local lines = {
        { "Generate a migration packet", ATTR.TITLE },
        { "  next ref  " .. tostring(app.note_ref or "?"), ATTR.VERSION },
        { "  path      " .. dest, ATTR.PATH },
    }
    if app.note_finding then
        lines[#lines + 1] = { "  finding   " .. (app.note_finding.id or ""), ATTR.PATH }
        lines[#lines + 1] = { "  class     " .. (app.note_finding.class or ""), ATTR.PATH }
    end
    if app.warn_in_repo then
        lines[#lines + 1] = { "", ATTR.PATH }
        lines[#lines + 1] = {
            "Warning: packet path is inside the git tree",
            ATTR.ERR,
        }
    end
    lines[#lines + 1] = { "", ATTR.PATH }
    lines[#lines + 1] = { "Optional one-line note:", ATTR.SECTION }
    lines[#lines + 1] = { "  " .. (app.note_buf or ""), ATTR.PROMPT }
    P.paint_framed(self, header, lines,
        "Press Enter to write (empty note is OK)   ESC cancel")
end

local function split_lines(text)
    local lines = {}
    for line in (tostring(text or "") .. "\n"):gmatch("(.-)\n") do
        lines[#lines + 1] = line
    end
    return lines
end

local function apply_content(self)
    local app = self.app
    local header = I.session_header(self.opts, app, self.inner_width)
    local token = app.apply_token or "?"
    local finding = app.apply_finding
    local is_orphan = finding and finding.kind == "orphan"
    local is_catalog = finding and finding.class
        and finding.class:find("^catalog") ~= nil
    local title = is_orphan
        and "Delete this orphan from the database"
        or is_catalog
        and "Apply catalog DDL to the database"
        or "Update this field on the database"
    local lines = {
        { title, ATTR.TITLE },
        { "  type      " .. token, ATTR.VERSION },
    }
    if finding then
        lines[#lines + 1] = { "  finding   " .. (finding.id or ""), ATTR.PATH }
        if is_catalog then
            lines[#lines + 1] = {
                "  table     " .. (finding.object or ""), ATTR.PATH }
            if finding.column and finding.column ~= ""
                and finding.column ~= "-" then
                lines[#lines + 1] = {
                    "  column    " .. finding.column, ATTR.PATH }
            end
            local want_null = (finding.kind == "nullable")
            if want_null then
                local dn = (finding.expected == "true"
                    or finding.expected == "YES" or finding.expected == "1")
                local verb = dn and "DROP NOT NULL" or "SET NOT NULL"
                lines[#lines + 1] = {
                    "  action    ALTER COLUMN " .. verb, ATTR.PATH }
            else
                lines[#lines + 1] = {
                    "  action    ADD COLUMN", ATTR.PATH }
            end
        elseif not is_orphan then
            lines[#lines + 1] = { "  field     " .. (finding.field or ""), ATTR.PATH }
            local ref_line = "  ref       " .. tostring(finding.ref or "?")
                .. "  /  type=" .. tostring(finding.db_type or "?")
            lines[#lines + 1] = { ref_line, ATTR.PATH }
        end
    end
    lines[#lines + 1] = { "", ATTR.PATH }
    if is_orphan then
        lines[#lines + 1] = {
            "This deletes orphan rows from queries. It does not author a migration.",
            ATTR.ERR,
        }
    elseif is_catalog then
        lines[#lines + 1] = {
            "This ALTER statement mutates live DDL shape.",
            ATTR.ERR,
        }
    else
        lines[#lines + 1] = {
            "This updates queries metadata only. It does not replay DDL.",
            ATTR.ERR,
        }
    end
    lines[#lines + 1] = { "", ATTR.PATH }
    if app.apply_sql and app.apply_sql ~= "" then
        lines[#lines + 1] = {
            "Proposed DDL (review before confirming):", ATTR.SECTION }
        for _, l in ipairs(split_lines(app.apply_sql)) do
            lines[#lines + 1] = { "  " .. l, ATTR.PATH }
        end
        lines[#lines + 1] = { "", ATTR.PATH }
    end
    lines[#lines + 1] = { "Type " .. token .. " to confirm:", ATTR.SECTION }
    lines[#lines + 1] = { "  " .. (app.apply_buf or ""), ATTR.PROMPT }
    local cancel
    if is_orphan then
        cancel = "Press Enter to delete   ESC cancel"
    elseif is_catalog then
        cancel = "Press Enter to apply DDL   ESC cancel"
    else
        cancel = "Press Enter to update   ESC cancel"
    end
    P.paint_framed(self, header, lines, cancel)
end

return {
    splash_content = splash_content,
    picker_content = picker_content,
    running_content = running_content,
    result_content = result_content,
    dashboard_content = dashboard_content,
    review_content = review_content,
    note_content = note_content,
    apply_content = apply_content,
    is_review_key_line = is_review_key_line,
}
