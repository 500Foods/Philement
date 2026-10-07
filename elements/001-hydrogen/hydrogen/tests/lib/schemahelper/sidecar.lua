local env = require('schemahelper_test_env')
local queue = require('schemahelper_queue')
local W = require('schemahelper_wrappers')
local C = require('schemahelper_const')

local function fail(msg)
    print('ERR: ' .. msg)
    os.exit(1)
end

local engine, role = W.wrapper_engine_role('sqlite_test')
if engine ~= 'sqlite' or role ~= 'test' then
    fail('sqlite_test stem engine=' .. tostring(engine) .. ' role=' .. tostring(role))
end
local plain_engine, plain_role = W.wrapper_engine_role('sqlite')
if plain_engine ~= 'sqlite' or plain_role ~= 'demo' then
    fail('unsuffixed stem is role demo')
end

local fixture_path = env.fixture .. '/schematool_sqlite_test.sh'
local ff = io.open(fixture_path, 'r')
if not ff then
    fail('missing fixture schematool_sqlite_test.sh')
end
ff:close()
local fixture_stem = W.wrapper_engine(fixture_path)
if fixture_stem ~= 'sqlite_test' then
    fail('fixture stem=' .. tostring(fixture_stem))
end

local demo = queue.default_state_path('/tmp/schemahelper-role', 'acuranzo', 'sqlite', 'demo')
local testp = queue.default_state_path('/tmp/schemahelper-role', 'acuranzo', 'sqlite', 'test')
if demo == testp then
    fail('test and demo sidecars share a path')
end
if not demo:find('schemahelper_acuranzo_sqlite_demo.json', 1, true) then
    fail('demo path=' .. demo)
end
if not testp:find('schemahelper_acuranzo_sqlite_test.json', 1, true) then
    fail('test path=' .. testp)
end

local h = io.popen('mktemp -d')
local dir = (h:read('*l') or ''):gsub('%s+$', '')
h:close()
if dir == '' then
    fail('mktemp failed')
end

local function write_file(path, body)
    local f = io.open(path, 'w')
    if not f then
        fail('cannot write ' .. path)
    end
    f:write(body)
    f:close()
end

write_file(dir .. '/schematool_sqlite.sh', '#!/bin/sh\nexport SCHEMATOOL_DB_SCHEMA=""\n')
write_file(dir .. '/schematool_sqlite_test.sh', '#!/bin/sh\nexport SCHEMATOOL_DB_SCHEMA="test"\n')
local list = W.discover_wrappers(dir)
if #list ~= 2 then
    fail('discover count=' .. tostring(#list))
end
if list[1].path == list[2].path then
    fail('discover collapsed test and demo')
end
if list[1].role ~= 'test' or list[2].role ~= 'demo' then
    fail('order role=' .. tostring(list[1].role) .. ',' .. tostring(list[2].role))
end
if list[1].engine ~= 'sqlite' or list[2].engine ~= 'sqlite' then
    fail('discover engine collapsed to stem')
end

local demo_side = dir .. '/schemahelper_acuranzo_sqlite_demo.json'
local test_side = dir .. '/schemahelper_acuranzo_sqlite_test.json'
write_file(demo_side, 'keep\n')
if not queue.create_state(test_side, 'acuranzo', 'sqlite', 'test') then
    fail('create_state test sidecar')
end
local rf = io.open(demo_side, 'r')
local kept = rf:read('*a')
rf:close()
if kept ~= 'keep\n' then
    fail('demo sidecar changed')
end

os.execute('rm -rf ' .. "'" .. dir:gsub("'", "'\\''") .. "'")

local live = W.discover_wrappers(env.schemagui)
local order = C.WRAPPER_ORDER
if #live ~= #order then
    fail('live wrapper count=' .. tostring(#live))
end
if #order ~= 16 then
    fail('wrapper order count=' .. tostring(#order))
end
for i, stem in ipairs(order) do
    local row = live[i]
    local expect_role = 'demo'
    if i <= 8 then
        expect_role = 'test'
    end
    local expect_engine = stem:gsub('_test$', ''):gsub('_demo$', '')
    if row.stem ~= stem or row.role ~= expect_role or row.engine ~= expect_engine then
        fail('live row ' .. tostring(i) .. ' stem=' .. tostring(row.stem)
            .. ' role=' .. tostring(row.role) .. ' engine=' .. tostring(row.engine))
    end
end

print('OK: test and demo sidecars are different files')
os.exit(0)
