local env = require('schemahelper_test_env')
local connect = require('schemahelper_connect')
local fixture = env.fixture
local errs = {}

local required = { 'parse_wrapper', 'resolve', 'probe', 'exec_sql', 'picker_blurb' }
for _, e in ipairs(env.require_fns(connect, 'connect', required)) do
    errs[#errs + 1] = e
end

local flags, exports = connect.parse_wrapper(fixture .. '/schematool_sqlite_fixture.sh')
if type(flags) ~= 'table' then
    errs[#errs + 1] = 'parse_wrapper flags not table'
end
if type(exports) ~= 'table' then
    errs[#errs + 1] = 'parse_wrapper exports not table'
end
if exports.SCHEMATOOL_DB_ENGINE ~= 'sqlite' then
    errs[#errs + 1] = 'parse_wrapper exports.SCHEMATOOL_DB_ENGINE=' .. tostring(exports.SCHEMATOOL_DB_ENGINE)
end
print('OK: parse_wrapper exports engine=' .. tostring(exports.SCHEMATOOL_DB_ENGINE))

local conn = connect.resolve(fixture .. '/schematool_sqlite_fixture.sh')
if type(conn) ~= 'table' then
    errs[#errs + 1] = 'resolve not table'
end
if type(conn.ok) ~= 'boolean' then
    errs[#errs + 1] = 'resolve ok not boolean'
end
if not conn.engine or conn.engine == '' then
    errs[#errs + 1] = 'resolve engine empty'
end
print('OK: resolve engine=' .. conn.engine .. ' family=' .. conn.family)

local flags2, exports2 = connect.parse_wrapper(fixture .. '/does_not_exist.sh')
if type(flags2) ~= 'table' or type(exports2) ~= 'table' then
    errs[#errs + 1] = 'parse_wrapper missing file should return tables'
end

env.fail_if(errs)
print('OK: all connect function checks passed (' .. #required .. ' functions)')
os.exit(0)
