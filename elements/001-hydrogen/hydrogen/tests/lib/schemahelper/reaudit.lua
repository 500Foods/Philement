local env = require('schemahelper_test_env')
local queue = require('schemahelper_queue')
local fixture = env.fixture
local state = queue.load_state(fixture .. '/schemahelper_acuranzo_sqlite.json')

local built = queue.build({
    out_dir = fixture,
    track = 'both',
    state = state,
})

local found_accepted = false
for _, item in ipairs(built.findings) do
    if item.id == 'meta:drift:1148:1003:name' and item.action == 'accepted' then
        found_accepted = true
        break
    end
end

if not found_accepted then
    print('ERR: accepted finding not found in findings')
    os.exit(1)
end

if built.totals.accepted < 1 then
    print('ERR: accepted count too low')
    os.exit(1)
end

print('OK: accepted findings persist across rebuild, accepted=' .. built.totals.accepted)
print('OK: subject count=' .. built.totals.subject)
os.exit(0)
