local env = require('schemahelper_test_env')
local packet = require('schemahelper_packet')
local fixture = env.fixture
local migrations = fixture .. '/migrations'
local errs = {}

local required = {
    'packet_name', 'packet_path', 'in_git_tree', 'list_reserved',
    'scan_refs', 'next_ref', 'collision', 'suggested_sql', 'write', 'promote',
}
for _, e in ipairs(env.require_fns(packet, 'packet', required)) do
    errs[#errs + 1] = e
end

local pname = packet.packet_name('acuranzo', 'sqlite', 1099)
if pname ~= 'schemahelper_acuranzo_sqlite_1099' then
    errs[#errs + 1] = 'packet_name=' .. tostring(pname)
end

local ppath = packet.packet_path(fixture, 'acuranzo', 'sqlite', 1099)
if ppath ~= fixture .. '/schemahelper_acuranzo_sqlite_1099' then
    errs[#errs + 1] = 'packet_path=' .. tostring(ppath)
end

local scan = packet.scan_refs({
    migrations = migrations,
    design = 'acuranzo',
    engine = 'sqlite',
    packet_dir = fixture,
})
if scan.max_ref < 1000 then
    errs[#errs + 1] = 'scan_refs max_ref=' .. tostring(scan.max_ref)
end
print('OK: scan_refs max_ref=' .. scan.max_ref)

local nextRef = packet.next_ref({
    migrations = migrations,
    design = 'acuranzo',
    engine = 'sqlite',
    packet_dir = fixture,
})
if type(nextRef) ~= 'number' or nextRef <= scan.max_ref then
    errs[#errs + 1] = 'next_ref=' .. tostring(nextRef) .. ' expected > ' .. scan.max_ref
end
print('OK: next_ref=' .. nextRef)

local collision_result = packet.collision({
    migrations = migrations,
    design = 'acuranzo',
    engine = 'sqlite',
    packet_dir = fixture,
}, 99999)
if collision_result ~= nil then
    errs[#errs + 1] = 'collision should be nil for unused ref, got ' .. tostring(collision_result)
end

local inv = packet.collision({
    migrations = migrations,
    design = 'acuranzo',
    engine = 'sqlite',
    packet_dir = fixture,
}, 0)
if inv ~= 'invalid ref' then
    errs[#errs + 1] = 'collision invalid ref expected, got ' .. tostring(inv)
end

local reserved = packet.list_reserved(fixture, 'acuranzo', 'sqlite')
if type(reserved) ~= 'table' then
    errs[#errs + 1] = 'list_reserved not table'
end
print('OK: list_reserved count=' .. #reserved)

env.fail_if(errs)
print('OK: all packet function checks passed (' .. #required .. ' functions)')
os.exit(0)
