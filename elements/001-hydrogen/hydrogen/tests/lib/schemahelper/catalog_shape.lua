-- Disk-fold catalog shape: a type-1000 column the live catalog lacks,
-- a type difference, a dropped object still live, and a live column
-- no migration mentions. The stored type-1003 dump has no row for the
-- added column. Info extras must not enter the review queue.

local env = require('schemahelper_test_env')
local queue = require('schemahelper_queue')
local apply = require('schemahelper_apply')

local function die(msg)
    print('ERR: ' .. msg)
    os.exit(1)
end

local lua_dir = env.schemagui .. '/lua'
local tmp = (os.getenv('TMPDIR') or '/tmp')
    .. '/schematool_shape72_' .. tostring(os.time())
os.execute('mkdir -p "' .. tmp .. '"')

local function write_all(path, data)
    local f = io.open(path, 'wb')
    if not f then
        die('cannot write ' .. path)
    end
    f:write(data)
    f:close()
end

local function run(cmd)
    local rc = os.execute(cmd)
    if rc ~= true and rc ~= 0 then
        die('command failed: ' .. cmd)
    end
end

local function jq_raw(filter, path)
    local fp = tmp .. '/q.jq'
    write_all(fp, filter .. '\n')
    local h = io.popen(string.format('jq -r -f "%s" "%s"', fp, path))
    if not h then
        die('jq popen failed')
    end
    local out = h:read('*a') or ''
    local closed = h:close()
    if closed ~= true and closed ~= 0 then
        die('jq failed: ' .. filter)
    end
    return (out:gsub('%s+$', ''))
end

local function must_eq(label, got, want)
    if got ~= want then
        die(label .. ' got=' .. tostring(got) .. ' want=' .. tostring(want))
    end
end

local expect_path = tmp .. '/expect.json'
local dump_path = tmp .. '/queries.json'
local live_path = tmp .. '/live.json'
local disk_fold = tmp .. '/disk_fold.json'
local stored_fold = tmp .. '/stored_fold.json'
local checklist = tmp .. '/catalog_checklist.json'
local findings = tmp .. '/catalog_findings.json'

write_all(expect_path, table.concat({
    '[{"ref":1000,"payloads":[{"query_type":1000,"code":',
    '"CREATE TABLE accounts (id integer NOT NULL, name varchar(32), ',
    'gone_col integer, kind char(8) NOT NULL);"}]},',
    '{"ref":1100,"payloads":[{"query_type":1000,"code":',
    '"ALTER TABLE accounts ADD COLUMN later_col text;"}]},',
    '{"ref":1200,"payloads":[{"query_type":1000,"code":',
    '"ALTER TABLE accounts DROP COLUMN gone_col;"}]},',
    '{"ref":1300,"payloads":[{"query_type":1000,"code":',
    '"CREATE TABLE shadow (id integer NOT NULL); DROP TABLE shadow;"}]},',
    '{"ref":1400,"payloads":[{"query_type":1000,"code":',
    '"CREATE TABLE rebuilt (id integer NOT NULL, old_col integer); ',
    'CREATE TABLE rebuilt_new (id integer NOT NULL); ',
    'DROP TABLE rebuilt; ',
    'ALTER TABLE rebuilt_new RENAME TO rebuilt;"}]}]',
}, ''))

write_all(dump_path, table.concat({
    '[{"query_ref":1000,"query_type":1003,"code":',
    '"CREATE TABLE accounts (id integer NOT NULL, name varchar(32), ',
    'gone_col integer, kind char(8) NOT NULL);"}]',
}, ''))

write_all(live_path, [[
{
  "schema": "",
  "tables": [
    {"table":"accounts","columns":[
      {"name":"id","data_type":"integer","nullable":false},
      {"name":"name","data_type":"VARCHAR (32)","nullable":true},
      {"name":"kind","data_type":"varchar(32)","nullable":false},
      {"name":"gone_col","data_type":"integer","nullable":true},
      {"name":"prod_extra","data_type":"text","nullable":true}
    ],"primary_key":[],"indexes":[]},
    {"table":"prod_only","columns":[
      {"name":"id","data_type":"integer","nullable":false}
    ],"primary_key":[],"indexes":[]},
    {"table":"shadow","columns":[
      {"name":"id","data_type":"integer","nullable":false}
    ],"primary_key":[],"indexes":[]},
    {"table":"rebuilt","columns":[
      {"name":"id","data_type":"integer","nullable":false}
    ],"primary_key":[],"indexes":[]}
  ]
}
]])

run(string.format(
    'lua "%s/schematool_catalog_fold.lua" --expected "%s" --out "%s"',
    lua_dir, expect_path, disk_fold))
run(string.format(
    'lua "%s/schematool_catalog_fold.lua" --db "%s" --out "%s"',
    lua_dir, dump_path, stored_fold))
run(string.format(
    'lua "%s/schematool_catalog_compare.lua" --expected "%s" --live "%s"'
        .. ' --checklist-out "%s" --findings-out "%s"',
    lua_dir, disk_fold, live_path, checklist, findings))

must_eq(
    'disk later_col ref',
    jq_raw(
        '.tables[] | select(.table=="accounts") | .columns[]'
            .. ' | select(.name=="later_col") | .ref',
        disk_fold),
    '1100')
must_eq(
    'stored fold has no later_col',
    jq_raw(
        '[.tables[]? | .columns[]? | select(.name=="later_col")] | length',
        stored_fold),
    '0')
must_eq('missing_column', jq_raw('.counts.missing_column', findings), '1')
must_eq('type', jq_raw('.counts.type', findings), '1')
must_eq('dropped', jq_raw('.counts.dropped', findings), '2')
must_eq('nullability', jq_raw('.counts.nullability', findings), '0')
must_eq('exit', jq_raw('.exit_code', findings), '2')
must_eq('info tables', jq_raw('.counts.live_extra_table', findings), '1')
must_eq('info columns', jq_raw('.counts.live_extra_column', findings), '2')
must_eq(
    'failure rows',
    jq_raw('.failures | length', findings),
    '4')
must_eq(
    'later_col ref on the finding',
    jq_raw(
        '.failures[] | select(.column=="later_col" and .check=="column") | .ref',
        findings),
    '1100')
must_eq(
    'type column',
    jq_raw('.failures[] | select(.check=="type") | .column', findings),
    'kind')
must_eq(
    'prod_extra is not a failure',
    jq_raw('[.failures[] | select(.column=="prod_extra")] | length', findings),
    '0')
must_eq(
    'prod_extra info status',
    jq_raw(
        '.live_extras[] | select(.column=="prod_extra") | .status',
        findings),
    'I')

local built = queue.build({
    out_dir = tmp,
    track = 'catalog',
    state = { by_id = {} },
})

local function in_subject(id)
    for _, item in ipairs(built.subject) do
        if item.id == id then
            return true
        end
    end
    return false
end

if not in_subject('cat:accounts:later_col:column') then
    die('missing disk column did not enter the review queue')
end
if not in_subject('cat:accounts:kind:type') then
    die('type finding did not enter the review queue')
end
if not in_subject('cat:accounts:gone_col:dropped') then
    die('dropped column did not enter the review queue')
end
if not in_subject('cat:shadow:-:dropped') then
    die('dropped table did not enter the review queue')
end
for _, item in ipairs(built.subject) do
    if item.kind == 'extra_column' or item.kind == 'extra_table' then
        die('info extra entered the review queue: ' .. tostring(item.id))
    end
end
must_eq('info total', tostring(built.totals.info), '3')

local dash = queue.build_dashboard_lines({
    out_dir = tmp,
    track = 'catalog',
    state = { by_id = {} },
})
local dash_text = table.concat(dash, '\n')
if not dash_text:find('Live extras (not applicable)', 1, true) then
    die('dashboard does not show info extras')
end
if not dash_text:find('catalog type', 1, true) then
    die('dashboard review classes omit type')
end
if not dash_text:find('catalog dropped', 1, true) then
    die('dashboard review classes omit dropped')
end

local type_finding = queue.find_finding(built.subject, 'cat:accounts:kind:type')
if not type_finding then
    die('type finding missing from subject')
end
local review = table.concat(queue.build_review_lines(type_finding), '\n')
if not review:find('change column type', 1, true) then
    die('type review does not describe a type change')
end
if apply.refuse_reason(type_finding, true) ~= nil then
    die('type apply refuse=' .. tostring(apply.refuse_reason(type_finding, true)))
end
local dropped_finding = queue.find_finding(
    built.subject, 'cat:accounts:gone_col:dropped')
if apply.refuse_reason(dropped_finding, true) ~= nil then
    die('dropped apply refused without an engine')
end
local sqlite_type = apply.refuse_reason(type_finding, true, 'sqlite')
if not sqlite_type or not sqlite_type:find('rebuild', 1, true) then
    die('sqlite type should refuse a rebuild')
end
local sqlite_drop = apply.refuse_reason(dropped_finding, true, 'sqlite')
if not sqlite_drop or not sqlite_drop:find('DROP COLUMN', 1, true) then
    die('sqlite DROP COLUMN should refuse a rebuild')
end

os.execute('rm -rf "' .. tmp .. '"')
print('OK: disk column is a finding with no type-1003 row')
print('OK: a live column no migration mentions is not a failure')
os.exit(0)
