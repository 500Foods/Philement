local env = require('schemahelper_test_env')
env.stub_terminal(require('terminal'))

local S = require('schemahelper_screens')
local Q = require('schemahelper_queue')
local packet = require('schemahelper_packet')

assert(type(packet.list_reserved) == 'function', 'packet.list_reserved is not a function')
assert(type(Q.list_reserved) == 'nil', 'queue.list_reserved should be nil')

local fixture = env.fixture
local state = Q.load_state(fixture .. '/schemahelper_acuranzo_sqlite.json')
local app = { conn = nil, log = '(none)', state = state, built = nil,
  warn_in_repo = false, catalog_degraded = false, show_mode_msg = '' }
local opts = { wrapper = '(none)', out_dir = fixture, work_dir = fixture, state_file = '(none)',
  track = 'both', migrations = fixture .. '/migrations', schematool = '(none)',
  design = 'acuranzo', engine = 'sqlite', packet_dir = fixture, allow_write = true,
  ref = 0, lua_version = '5.5' }
local self_panel = { opts = opts, app = app, inner_row = 1, inner_col = 1,
  inner_height = 24, inner_width = 80 }

local ok, err = pcall(S.dashboard_content, self_panel)
if ok then
  print('OK: dashboard_content rendered without error')
else
  print('ERR: ' .. tostring(err))
  os.exit(1)
end
os.exit(0)
