local env = require('schemahelper_test_env')
env.stub_terminal(require('terminal'))

local I = require('schemahelper_invoke')
local C = require('schemahelper_const')
local app = {
  mode = 'picker',
  picker = {
    list = {
      { engine = 'sqlite', path = '/tmp/schematool_sqlite.sh' },
      { engine = 'postgresql', path = '/tmp/schematool_postgresql.sh' },
    },
    selected = 1,
  },
  conn = nil,
  planned_work_dir = '/tmp/schemahelper-planned',
}
local opts = { wrapper = '', work_dir = '', track = 'both' }
local lines = I.instance_block(opts, app, 80)
assert(lines[1][1] == 'Wrapper' and lines[1][2] == 'schematool_sqlite.sh', lines[1][2])
assert(lines[2][1] == 'Logging' and lines[2][2] == 'schemahelper_schematool.log', lines[2][2])
assert(lines[3][1] == 'Working' and lines[3][2] == '/tmp/schemahelper-planned', lines[3][2])
assert(lines[4][1] == 'Connect' and lines[4][2] == 'Not Connected', lines[4][2])
assert(lines[1][3] ~= lines[1][4], 'label attr must differ from value attr')
app.picker.selected = 2
lines = I.instance_block(opts, app, 80)
assert(lines[1][1] == 'Wrapper' and lines[1][2] == 'schematool_postgresql.sh', lines[1][2])
assert(lines[4][1] == 'Connect' and lines[4][2] == 'Not Connected', lines[4][2])
app.mode = 'running'
app.conn = {
  ok = true, family = 'file', host = '', database = '/x/hydrodemo.sqlite',
  user = '', port = '', schema = '',
}
opts.wrapper = '/tmp/schematool_sqlite.sh'
opts.work_dir = '/tmp/schemahelper-planned'
lines = I.instance_block(opts, app, 80)
assert(lines[1][1] == 'Wrapper' and lines[1][2] == 'schematool_sqlite.sh', lines[1][2])
assert(lines[4][1] == 'Connect' and lines[4][2] == 'ok  hydrodemo.sqlite', lines[4][2])
assert(C.CHROME_TITLE.picker == ' SchemaHelper: Target ')
assert(C.CHROME_TITLE.running == ' SchemaHelper: SchemaTool ')
assert(C.CHROME_TITLE.dashboard == ' SchemaHelper: Dashboard ')
print('OK: instance_block follows picker selection; Connect locked on Target')
os.exit(0)
