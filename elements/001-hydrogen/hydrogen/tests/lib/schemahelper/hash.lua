local env = require('schemahelper_test_env')
local queue = require('schemahelper_queue')
local fixture = env.fixture
local tmp_state = fixture .. '/.test_hash_state.json'
local f = io.open(tmp_state, 'w')
f:write('{"version":1,"design":"acuranzo","engine":"sqlite","schema":"",')
f:write('"updated_utc":"2026-09-09T00:00:00Z","cursor_id":"","decisions":[]}')
f:close()

local empty = queue.load_state(tmp_state)
local built0 = queue.build({ out_dir = fixture, track = 'both', state = empty })
local target
for _, item in ipairs(built0.findings) do
    if item.id == 'meta:drift:1148:1003:name' then
        target = item
        break
    end
end
assert(target, 'drift finding missing')
local good = queue.finding_hash(target)
assert(good ~= '', good)

queue.save_decision(tmp_state, target.id, 'accepted', { hash = good })
queue.save_decision(tmp_state, 'cat:accounts:id:nullable', 'skipped', nil)
local st = queue.load_state(tmp_state)
local built = queue.build({ out_dir = fixture, track = 'both', state = st })
assert(built.totals.accepted == 1, tostring(built.totals.accepted))
local in_subject = false
for _, item in ipairs(built.subject) do
    if item.id == target.id then in_subject = true end
end
assert(not in_subject, 'accepted id still in subject')
assert(#built.accepted == 1 and built.accepted[1].id == target.id, 'accepted list')

queue.save_decision(tmp_state, target.id, 'accepted', { hash = 'deadbeef' })
st = queue.load_state(tmp_state)
built = queue.build({ out_dir = fixture, track = 'both', state = st })
assert(built.totals.accepted == 0, 'stale hash still accepted')
in_subject = false
for _, item in ipairs(built.subject) do
    if item.id == target.id then in_subject = true end
end
assert(in_subject, 'hash mismatch did not re-queue')

queue.save_decision(tmp_state, target.id, 'accepted', { hash = good })
st = queue.load_state(tmp_state)
assert(st.by_id['cat:accounts:id:nullable']
    and st.by_id['cat:accounts:id:nullable'].action == 'skipped', 'other decision lost')
queue.remove_decision(tmp_state, target.id)
st = queue.load_state(tmp_state)
assert(st.by_id[target.id] == nil, 'un-accept left decision')
assert(st.by_id['cat:accounts:id:nullable']
    and st.by_id['cat:accounts:id:nullable'].action == 'skipped', 'un-accept deleted other')
built = queue.build({ out_dir = fixture, track = 'both', state = st })
in_subject = false
for _, item in ipairs(built.subject) do
    if item.id == target.id then in_subject = true end
end
assert(in_subject, 'un-accept did not restore queue')
assert(built.totals.accepted == 0, tostring(built.totals.accepted))

os.remove(tmp_state)
print('OK: hash mismatch re-queues; un-accept restores without dropping others')
os.exit(0)
