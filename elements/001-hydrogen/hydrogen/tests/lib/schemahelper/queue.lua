local env = require('schemahelper_test_env')
local queue = require('schemahelper_queue')
local fixture = env.fixture

local required = {
    'build', 'load_state', 'create_state', 'default_state_path',
    'artifacts_present', 'load_metadata', 'load_detail_section',
    'note_for', 'explore_lines', 'save_decision', 'save_cursor',
    'jq_update_state', 'build_dashboard_lines', 'build_review_lines',
    'find_finding', 'json_subobj', 'payload_text',
}
env.fail_if(env.require_fns(queue, 'queue', required))
print('OK: all ' .. #required .. ' required functions present')

local present = queue.artifacts_present(fixture, 'both')
if not present then
    print('ERR: artifacts_present returned false')
    os.exit(1)
end
print('OK: artifacts_present=true')

local state = queue.load_state(fixture .. '/schemahelper_acuranzo_sqlite.json')
if state.design ~= 'acuranzo' then
    print('ERR: design=' .. tostring(state.design))
    os.exit(1)
end
print('OK: state design=' .. state.design .. ' engine=' .. state.engine)

if state.cursor_id ~= 'meta:drift:1148:1003:name' then
    print('ERR: cursor_id=' .. tostring(state.cursor_id))
    os.exit(1)
end
print('OK: cursor_id=' .. state.cursor_id)

if #state.decisions < 1 then
    print('ERR: no decisions loaded')
    os.exit(1)
end
print('OK: decisions loaded: ' .. #state.decisions)

local built = queue.build({
    out_dir = fixture,
    track = 'both',
    state = state,
})
print('OK: build totals total=' .. built.totals.total .. ' subject=' .. built.totals.subject)

local dash = queue.build_dashboard_lines({
    out_dir = fixture,
    track = 'both',
    state = state,
})
if not dash or #dash == 0 then
    print('ERR: dashboard lines empty')
    os.exit(1)
end
local dash_text = table.concat(dash, '\n')
if not dash_text:find('Findings for review', 1, true) then
    print('ERR: dashboard missing findings-for-review label')
    os.exit(1)
end
if dash_text:find('Subject for review', 1, true) then
    print('ERR: dashboard still labels the queue as migrations/subject')
    os.exit(1)
end
print('OK: dashboard produces ' .. #dash .. ' lines')

local review_lines = queue.build_review_lines(built.findings[1])
if not review_lines or #review_lines == 0 then
    print('ERR: review lines empty')
    os.exit(1)
end
print('OK: review produces ' .. #review_lines .. ' lines')

local explore = queue.explore_lines(fixture, 'orphan:1290', built.findings)
if not explore or #explore == 0 then
    print('ERR: explore lines empty')
    os.exit(1)
end
print('OK: explore produces ' .. #explore .. ' lines')

local note = queue.note_for('meta:drift:1148:1003:name', fixture, state)
if note ~= 'known drift' then
    print('ERR: note_for returned: ' .. tostring(note))
    os.exit(1)
end
print('OK: note_for returns: ' .. note)

local tmp_state = fixture .. '/.test_roundtrip_state.json'
local f = io.open(tmp_state, 'w')
f:write('{"version":1,"design":"acuranzo","engine":"sqlite","schema":"",')
f:write('"updated_utc":"2026-08-23T12:00:00Z","cursor_id":"","decisions":[]}')
f:close()

queue.save_decision(tmp_state, 'cat:accounts:id:nullable', 'accepted', {hash='test', note='test note'})
queue.save_cursor(tmp_state, 'cat:accounts:id:nullable')

local h = io.open(tmp_state, 'r')
local content = h:read('*a')
h:close()
assert(content:find('"accepted"'), 'decision not persisted')
assert(content:find('"cursor_id"'), 'cursor not persisted')
assert(content:find('"cat:accounts:id:nullable"'), 'finding id not persisted')
print('OK: save_decision + save_cursor round-trip works')

os.remove(tmp_state)
os.exit(0)
