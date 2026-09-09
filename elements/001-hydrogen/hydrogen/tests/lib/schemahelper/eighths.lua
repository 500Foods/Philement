require('schemahelper_test_env')
package.loaded['terminal'] = nil
package.preload['terminal'] = function()
  return {
    input = { keymap = { default_key_map = {}, default_keys = {} } },
    text = { width = {}, push_seq = function() return '' end, pop_seq = function() return '' end },
    cursor = { position = { set = function() end } },
    output = { write = function() end, flush = function() end },
  }
end
package.preload['terminal.ui.panel.screen'] = function() return {} end
package.preload['terminal.ui.panel'] = function() return {} end
local t = require('terminal')
t.output.write = function() end
t.cursor.position.set = function() end
t.text.push_seq = function() return nil end
t.text.pop_seq = function() return nil end
t.text.width.utf8swidth = function(s) return #tostring(s or '') end
t.text.width.truncate_ellipsis = function(w, s, _) return tostring(s or ''):sub(1, w) end

local I = require('schemahelper_invoke')
assert(I.eighths_width(378) == 48, tostring(I.eighths_width(378)))
assert(I.eighths_width(8) == 1)
assert(I.eighths_width(9) == 2)
assert(I.eighths_width(0) == 0)

local full = I.progress_eighths({ current = 378, total = 378, cell_issue = {} })
assert(#full == 48, tostring(#full))
assert(full[1].ch == '█', full[1].ch)
assert(full[47].ch == '█', full[47].ch)
assert(full[48].ch == '▎', full[48].ch)
assert(full[1].attr.bg == 236, tostring(full[1].attr.bg))
assert(full[48].attr.bg == 236, tostring(full[48].attr.bg))

local mid = I.progress_eighths({ current = 10, total = 378, cell_issue = {} })
assert(mid[1].ch == '█', mid[1].ch)
assert(mid[2].ch == '▎', mid[2].ch)
assert(mid[3].ch == ' ', mid[3].ch)
assert(mid[3].attr.bg == 236, tostring(mid[3].attr.bg))

local path = os.tmpname()
local f = io.open(path, 'w')
assert(f)
f:write([[
expect 1/16 ref 1000 name=alpha
expect 2/16 ref 1001 name=beta
expect 8/16 ref 1007 name=h
expect 16/16 ref 1015 name=p
compare 1/16 ref 1000 ok
compare 2/16 ref 1001 drift
compare 16/16 ref 1015 ok
catalog ref 1015 missing_column
Compare: total=16 ok=15 drift=1 missing_load=0 missing_apply=0 orphans=0 exit=2
]])
f:close()
local prog = I.parse_schematool_progress(path, 16)
os.remove(path)
assert(prog.total == 16, tostring(prog.total))
assert(prog.current == 16, tostring(prog.current))
assert(prog.name == 'p', tostring(prog.name))
assert(#prog.issues == 2, tostring(#prog.issues))
assert(prog.issues[1].class == 'drift', prog.issues[1].class)
assert(prog.issues[1].ref == 1001)
assert(prog.issues[2].class == 'catalog', prog.issues[2].class)
assert(prog.cell_issue[1] == 'drift', tostring(prog.cell_issue[1]))
assert(prog.cell_issue[2] == 'catalog', tostring(prog.cell_issue[2]))
local tinted = I.progress_eighths(prog)
assert(tinted[1].class == 'drift', tostring(tinted[1].class))
assert(tinted[2].class == 'catalog', tostring(tinted[2].class))
local spans = I.progress_bar_spans(prog)
assert(spans[#spans][1] == '  100%', spans[#spans][1])
assert(#spans == 3, tostring(#spans))

prog.issue_vis = 1
local app = { progress = prog }
I.apply_issue_scroll(app, 1)
assert(app.progress.issue_scroll == 1, tostring(app.progress.issue_scroll))
I.handle_run_input('j', app)
assert(app.progress.issue_scroll == 0, tostring(app.progress.issue_scroll))

print('OK: eighths 378→48; fake log tints cell and lists issues')
os.exit(0)
