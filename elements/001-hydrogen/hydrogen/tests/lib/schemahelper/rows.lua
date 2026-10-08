-- Phase 9: keyed default rows. This fixture does not execute SQL.
-- Net migration rows are keys 1 and 2. Key 3 was deleted. Key 9 is
-- a live person no migration names, so it stays out of the queue.

require('schemahelper_test_env')
local apply = require('schemahelper_apply')
local queue = require('schemahelper_queue')
local rows = require('schematool_rows')

local function die(msg)
    print('ERR: ' .. msg)
    os.exit(1)
end

local cafe = 'Caf' .. string.char(0xC3, 0xA9)

local tmp = (os.getenv('TMPDIR') or '/tmp')
    .. '/schematool_rows72_' .. tostring(os.time())
os.execute('mkdir -p "' .. tmp .. '"')

local function write_all(path, data)
    local f = io.open(path, 'wb')
    if not f then
        die('cannot write ' .. path)
    end
    f:write(data)
    f:close()
end

local function must_eq(label, got, want)
    if got ~= want then
        die(label .. ' got [' .. tostring(got)
            .. '] want [' .. tostring(want) .. ']')
    end
end

local function must_find(label, hay, needle)
    if not tostring(hay or ''):find(needle, 1, true) then
        die(label .. ' missing [' .. needle .. '] in ['
            .. tostring(hay) .. ']')
    end
end

local function must_absent(label, hay, needle)
    if tostring(hay or ''):find(needle, 1, true) then
        die(label .. ' unexpectedly contains [' .. needle .. ']')
    end
end

local contacts_sql = table.concat({
    'CREATE TABLE contacts (',
    'contact_id integer NOT NULL PRIMARY KEY, ',
    'name text, city text); ',
    'INSERT INTO contacts (contact_id, name, city) VALUES ',
    "(1, '", cafe, "', 'London'), ",
    "(2, 'Bea', 'Paris'), (3, 'Cara', 'Rome'); ",
    'DELETE FROM contacts WHERE contact_id = 3; ',
    "UPDATE contacts SET city = 'X'; ",
    "UPDATE queries SET name = 'book' WHERE query_ref = 1;",
}, '')

local labels_sql = table.concat({
    'CREATE TABLE labels (',
    'label_id integer NOT NULL, ',
    'code text NOT NULL UNIQUE, name text); ',
    "INSERT INTO labels (label_id, code, name) ",
    "VALUES (7, 'gold', 'Gold'); ",
    "UPDATE labels SET name = 'Golden' WHERE code = 'gold';",
}, '')

local model = rows.extract({
    { ref = 2100, code = contacts_sql },
    { ref = 2110, code = labels_sql },
}, {}, { engine = 'sqlite', schema = '' })

local probe = 'SELECT contact_id, name, city FROM contacts WHERE '
    .. '(contact_id = 1) OR (contact_id = 2) OR (contact_id = 3);'
must_eq('probe', model.tables.contacts.probe_sql, probe)
must_absent('probe star', probe, '*')
must_absent('probe extra key', probe, '9')

local gold = model.tables.labels.rows.gold
if not gold then
    die('labels key gold was not extracted')
end
must_eq('labels order', table.concat(gold.order, ','),
    'code,label_id,name')
must_eq('labels name', gold.values.name, 'Golden')

local live = {
    probed = { contacts = true, labels = true },
    tables = {
        contacts = {
            ['2'] = {
                contact_id = '2', name = 'Bea', city = 'Berlin',
            },
            ['3'] = {
                contact_id = '3', name = 'Cara', city = 'Rome',
            },
            ['9'] = {
                contact_id = '9', name = 'Zoe', city = 'Oslo',
            },
        },
        labels = {
            gold = {
                label_id = '7', code = 'gold', name = 'Golden',
            },
        },
    },
}

local findings, counts = rows.compare(model, live, false)
must_eq('missing', counts.row_missing, 1)
must_eq('diff', counts.row_diff, 1)
must_eq('present', counts.row_present, 1)
must_eq('unkeyed', counts.unkeyed, 1)

write_all(tmp .. '/rows_findings.json',
    rows.encode_findings(findings, counts) .. '\n')

local built = queue.build({ out_dir = tmp, track = 'catalog' })
must_eq('queue size', #built.subject, 4)

local function take(id)
    local found = queue.find_finding(built.subject, id)
    if not found then
        die('missing queue id ' .. id)
    end
    return found
end

local missing = take('row:contacts:1:row_missing')
local diff = take('row:contacts:2:row_diff')
local present = take('row:contacts:3:row_present')
local unkeyed = take('row:contacts:unkeyed:2100:1')

must_find('cafe survived json', missing.expected, cafe)
must_absent('queue expected', missing.expected, 'Zoe')

local function quoted(engine, text)
    if engine == 'postgresql' or engine == 'yugabytedb' then
        return '$schematool$' .. text .. '$schematool$'
    end
    if engine == 'mssql' then
        return "N'" .. text .. "'"
    end
    return "'" .. text .. "'"
end

local engines = {
    { id = 'postgresql', schema = 'demo', q = 'demo.contacts' },
    { id = 'yugabytedb', schema = 'demo', q = 'demo.contacts' },
    { id = 'mysql', schema = 'demo', q = '`demo`.`contacts`' },
    { id = 'mariadb', schema = 'demo', q = '`demo`.`contacts`' },
    { id = 'sqlite', schema = '', q = 'contacts' },
    { id = 'db2', schema = 'demo', q = 'DEMO.CONTACTS' },
    { id = 'firebird', schema = '', q = 'contacts' },
    { id = 'mssql', schema = 'demoms', q = '[demoms].[contacts]' },
}

local function build(engine, finding)
    return apply.build_sql(finding, {
        engine = engine.id,
        schema = engine.schema,
    }, tmp)
end

local all_sql = {}
for _, engine in ipairs(engines) do
    local ins = build(engine, missing)
    local upd = build(engine, diff)
    local del = build(engine, present)
    local qcafe = quoted(engine.id, cafe)
    local qlon = quoted(engine.id, 'London')
    local qpar = quoted(engine.id, 'Paris')
    must_eq(engine.id .. ' insert', ins,
        'INSERT INTO ' .. engine.q
        .. ' (contact_id, name, city) VALUES (1, '
        .. qcafe .. ', ' .. qlon .. ');')
    must_eq(engine.id .. ' update', upd,
        'UPDATE ' .. engine.q .. ' SET city = ' .. qpar
        .. ' WHERE contact_id = 2;')
    must_eq(engine.id .. ' delete', del,
        'DELETE FROM ' .. engine.q .. ' WHERE contact_id = 3;')
    must_find(engine.id .. ' cafe bytes', ins, cafe)
    must_absent(engine.id .. ' insert question', ins, '?')
    must_absent(engine.id .. ' delete extra', del, 'contact_id = 9')
    must_absent(engine.id .. ' update name', upd, 'Bea')
    all_sql[#all_sql + 1] = ins
    all_sql[#all_sql + 1] = upd
    all_sql[#all_sql + 1] = del
end

must_eq('token missing', apply.confirm_token(missing), 'contacts.1')
must_eq('token diff', apply.confirm_token(diff), 'contacts.2')
must_eq('token present', apply.confirm_token(present), 'contacts.3')

local refused_sql, refused = build(
    { id = 'sqlite', schema = '' }, unkeyed)
if refused_sql ~= nil then
    die('unkeyed emitted SQL')
end
must_find('unkeyed refuse', refused, 'AutoMigration')

local function review_blob(finding)
    local why = apply.refuse_reason(finding, true, 'sqlite')
    local lines = queue.build_review_lines(finding, nil, nil, why)
    return table.concat(lines, '\n')
end

local blob = table.concat(all_sql, '\n') .. '\n'
    .. review_blob(missing) .. '\n'
    .. review_blob(diff) .. '\n'
    .. review_blob(present) .. '\n'
    .. review_blob(unkeyed)
must_find('review insert', review_blob(missing), 'INSERT this default row')
must_find('review update', review_blob(diff),
    'UPDATE migration-owned columns')
must_find('review delete', review_blob(present), 'DELETE this key only')
must_find('review token', review_blob(missing), 'table.key')
must_absent('extra person', blob, 'Zoe')
must_absent('extra delete', blob, 'contact_id = 9')
for _, item in ipairs(built.subject) do
    if item.column == '9' or item.object == 'labels' then
        die('queue listed ' .. item.id)
    end
end

local uni = {
    kind = 'row_missing',
    class = 'default row',
    object = 'contacts',
    column = '4',
    expected = table.concat({
        '{"__keys":"contact_id","__order":"contact_id,name",',
        '"__nums":"contact_id","contact_id":"4","name":"caf\\u00e9"}',
    }, ''),
}
local acute = string.char(0xC3, 0xA9)
local uni_sql = build({ id = 'sqlite', schema = '' }, uni)
must_find('unicode bytes', uni_sql, 'caf' .. acute)
must_absent('unicode question', uni_sql, '?')
local uni_pg = build({ id = 'postgresql', schema = 'demo' }, uni)
must_find('unicode pg', uni_pg, '$schematool$caf' .. acute .. '$schematool$')

print('OK: default rows; extra person key 9 stayed out')
