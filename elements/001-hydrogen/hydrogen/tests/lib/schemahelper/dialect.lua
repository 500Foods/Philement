-- Phase 8: SQL text for eight engines. This fixture does not execute it.

local env = require('schemahelper_test_env')
local apply = require('schemahelper_apply')
local queue = require('schemahelper_queue')

local function die(msg)
    print('ERR: ' .. msg)
    os.exit(1)
end

local tmp = (os.getenv('TMPDIR') or '/tmp')
    .. '/schematool_dialect72_' .. tostring(os.time())
os.execute('mkdir -p "' .. tmp .. '"')

local expected_json = table.concat({
    '{"schema":"demo","tables":[',
    '{"table":"accounts","columns":[',
    '{"name":"stripe_customer_id","data_type":"varchar(100)","nullable":true},',
    '{"name":"password_hash","data_type":"char(128)","nullable":true},',
    '{"name":"locked","data_type":"char(1)","nullable":false},',
    '{"name":"kind","data_type":"integer","nullable":false}',
    '],"primary_key":[]},',
    '{"table":"contacts","columns":[',
    '{"name":"contact_id","data_type":"integer","nullable":false},',
    '{"name":"city","data_type":"varchar(40)","nullable":true}',
    '],"primary_key":["contact_id"]}',
    '],"dropped":[]}',
}, '')

local f = io.open(tmp .. '/catalog_expected.json', 'wb')
if not f then
    die('cannot write catalog_expected.json')
end
f:write(expected_json)
f:close()

local engines = {
    { id = 'postgresql', schema = 'demo', q = 'demo.accounts',
        c = 'demo.contacts', s = 'demo.shadow' },
    { id = 'yugabytedb', schema = 'demo', q = 'demo.accounts',
        c = 'demo.contacts', s = 'demo.shadow' },
    { id = 'mysql', schema = 'demo', q = '`demo`.`accounts`',
        c = '`demo`.`contacts`', s = '`demo`.`shadow`' },
    { id = 'mariadb', schema = 'demo', q = '`demo`.`accounts`',
        c = '`demo`.`contacts`', s = '`demo`.`shadow`' },
    { id = 'sqlite', schema = '', q = 'accounts', c = 'contacts', s = 'shadow' },
    { id = 'db2', schema = 'demo', q = 'DEMO.ACCOUNTS',
        c = 'DEMO.CONTACTS', s = 'DEMO.SHADOW' },
    { id = 'firebird', schema = '', q = 'accounts', c = 'contacts', s = 'shadow' },
    { id = 'mssql', schema = 'demoms', q = '[demoms].[accounts]',
        c = '[demoms].[contacts]', s = '[demoms].[shadow]' },
}

local function build(engine, finding)
    return apply.build_sql(finding, {
        engine = engine.id,
        schema = engine.schema,
    }, tmp)
end

local function must_eq(label, got, want)
    if got ~= want then
        die(label .. ' got [' .. tostring(got) .. '] want [' .. tostring(want) .. ']')
    end
end

local function must_refuse(label, got, err, needle)
    if got ~= nil then
        die(label .. ' emitted SQL: ' .. tostring(got))
    end
    if not err or not tostring(err):find(needle, 1, true) then
        die(label .. ' reason [' .. tostring(err) .. ']')
    end
end

local add_finding = {
    class = 'catalog missing column',
    kind = 'column',
    object = 'accounts',
    column = 'stripe_customer_id',
    expected = 'present',
}
local drop_null = {
    class = 'catalog nullability',
    kind = 'nullable',
    object = 'accounts',
    column = 'password_hash',
    expected = 'true',
}
local set_null = {
    class = 'catalog nullability',
    kind = 'nullable',
    object = 'accounts',
    column = 'locked',
    expected = 'false',
}
local type_finding = {
    class = 'catalog type',
    kind = 'type',
    object = 'accounts',
    column = 'kind',
    expected = 'integer',
}
local create_finding = {
    class = 'catalog missing table',
    kind = 'table',
    object = 'contacts',
    column = '-',
    expected = 'present',
}
local drop_col = {
    class = 'catalog dropped',
    kind = 'dropped',
    object = 'accounts',
    column = 'gone_col',
}
local drop_tab = {
    class = 'catalog dropped',
    kind = 'dropped',
    object = 'shadow',
    column = '-',
}

if apply.confirm_token(add_finding) ~= 'accounts.stripe_customer_id' then
    die('add confirm token')
end
if apply.confirm_token(drop_col) ~= 'DROP accounts.gone_col' then
    die('drop column token')
end
if apply.confirm_token(drop_tab) ~= 'DROP shadow' then
    die('drop table token')
end

for _, engine in ipairs(engines) do
    local add_sql, add_err = build(engine, add_finding)
    local add_want
    if engine.id == 'firebird' or engine.id == 'mssql' then
        add_want = 'ALTER TABLE ' .. engine.q
            .. ' ADD stripe_customer_id varchar(100);'
    else
        add_want = 'ALTER TABLE ' .. engine.q
            .. ' ADD COLUMN stripe_customer_id varchar(100);'
    end
    must_eq(engine.id .. ' add', add_sql, add_want)
    if add_err then
        die(engine.id .. ' add err ' .. tostring(add_err))
    end

    local null_sql, null_err = build(engine, drop_null)
    local set_sql, set_err = build(engine, set_null)
    local type_sql, type_err = build(engine, type_finding)
    local col_sql, col_err = build(engine, drop_col)
    if engine.id == 'sqlite' then
        must_refuse(engine.id .. ' null', null_sql, null_err, 'rebuild')
        must_refuse(engine.id .. ' set', set_sql, set_err, 'rebuild')
        must_refuse(engine.id .. ' type', type_sql, type_err, 'rebuild')
        must_refuse(engine.id .. ' drop col', col_sql, col_err, 'DROP COLUMN')
    else
        local null_want
        local set_want
        local type_want
        local col_want
        if engine.id == 'postgresql' or engine.id == 'yugabytedb'
            or engine.id == 'db2' then
            null_want = 'ALTER TABLE ' .. engine.q
                .. ' ALTER COLUMN password_hash DROP NOT NULL;'
            set_want = 'ALTER TABLE ' .. engine.q
                .. ' ALTER COLUMN locked SET NOT NULL;'
        elseif engine.id == 'firebird' then
            null_want = 'ALTER TABLE ' .. engine.q
                .. ' ALTER password_hash DROP NOT NULL;'
            set_want = 'ALTER TABLE ' .. engine.q
                .. ' ALTER locked SET NOT NULL;'
        elseif engine.id == 'mysql' or engine.id == 'mariadb' then
            null_want = 'ALTER TABLE ' .. engine.q
                .. ' MODIFY COLUMN password_hash char(128) NULL;'
            set_want = 'ALTER TABLE ' .. engine.q
                .. ' MODIFY COLUMN locked char(1) NOT NULL;'
        else
            null_want = 'ALTER TABLE ' .. engine.q
                .. ' ALTER COLUMN password_hash char(128) NULL;'
            set_want = 'ALTER TABLE ' .. engine.q
                .. ' ALTER COLUMN locked char(1) NOT NULL;'
        end
        if engine.id == 'postgresql' or engine.id == 'yugabytedb' then
            type_want = 'ALTER TABLE ' .. engine.q
                .. ' ALTER COLUMN kind TYPE integer;'
        elseif engine.id == 'db2' then
            type_want = 'ALTER TABLE ' .. engine.q
                .. ' ALTER COLUMN kind SET DATA TYPE integer;'
        elseif engine.id == 'firebird' then
            type_want = 'ALTER TABLE ' .. engine.q
                .. ' ALTER kind TYPE integer;'
        elseif engine.id == 'mysql' or engine.id == 'mariadb' then
            type_want = 'ALTER TABLE ' .. engine.q
                .. ' MODIFY COLUMN kind integer NOT NULL;'
        else
            type_want = 'ALTER TABLE ' .. engine.q
                .. ' ALTER COLUMN kind integer NOT NULL;'
        end
        if engine.id == 'firebird' then
            col_want = 'ALTER TABLE ' .. engine.q .. ' DROP gone_col;'
        elseif engine.id ~= 'mssql' then
            col_want = 'ALTER TABLE ' .. engine.q
                .. ' DROP COLUMN gone_col;'
        end
        must_eq(engine.id .. ' drop null', null_sql, null_want)
        must_eq(engine.id .. ' set null', set_sql, set_want)
        must_eq(engine.id .. ' type', type_sql, type_want)
        if engine.id == 'mssql' then
            if not col_sql or not col_sql:find('DROP CONSTRAINT', 1, true) then
                die('mssql drop column missing DROP CONSTRAINT')
            end
            if not col_sql:find('sys.default_constraints', 1, true) then
                die('mssql drop column missing default lookup')
            end
            if not col_sql:find(
                'ALTER TABLE ' .. engine.q .. ' DROP COLUMN gone_col;',
                1, true) then
                die('mssql drop column missing DROP COLUMN')
            end
        else
            must_eq(engine.id .. ' drop col', col_sql, col_want)
        end
        if null_err or set_err or type_err or col_err then
            die(engine.id .. ' unexpected err')
        end
    end

    local create_sql, create_err = build(engine, create_finding)
    if not create_sql then
        die(engine.id .. ' create ' .. tostring(create_err))
    end
    local create_head = 'CREATE TABLE ' .. engine.c .. ' (\n'
        .. '    contact_id integer NOT NULL,\n'
        .. '    city varchar(40),\n'
        .. '    PRIMARY KEY (contact_id)\n);'
    must_eq(engine.id .. ' create', create_sql, create_head)

    local drop_sql, drop_err = build(engine, drop_tab)
    must_eq(engine.id .. ' drop table', drop_sql, 'DROP TABLE ' .. engine.s .. ';')
    if drop_err then
        die(engine.id .. ' drop table err')
    end
end

local extra = {
    class = 'catalog live extra',
    kind = 'extra_column',
    object = 'accounts',
    column = 'prod_extra',
}
local extra_why = apply.refuse_reason(extra, true, 'sqlite')
if not extra_why or not extra_why:find('not applicable', 1, true) then
    die('extra column refuse=' .. tostring(extra_why))
end
local extra_tok = apply.confirm_token(extra) or ''
if extra_tok:find('DROP', 1, true) then
    die('extra column was offered as a drop')
end
local extra_sql = apply.build_sql(extra, { engine = 'sqlite', schema = '' }, tmp)
if extra_sql then
    die('extra column emitted SQL')
end

local unicode = {
    class = 'metadata content drift',
    kind = 'drift',
    field = 'code',
    ref = 12,
    db_type = 1000,
    expected = '{"code":"caf\\u00e9!"}',
}
local unicode_sql = apply.build_sql(unicode, { engine = 'sqlite', schema = '' }, tmp)
if not unicode_sql or not unicode_sql:find('caf\195\169!', 1, true) then
    die('unicode literal did not round-trip')
end
if unicode_sql:find('?', 1, true) then
    die('unicode literal used a question mark')
end

local one = {
    class = 'metadata content drift',
    kind = 'drift',
    field = 'code',
    ref = 1223,
    db_type = 1003,
    expected = '{"code":"C"}',
}
if apply.confirm_token(one) ~= '1223.code' then
    die('one-field token')
end

local row = {
    id = 'meta:drift:1223:1003:row',
    class = 'metadata content drift',
    kind = 'drift',
    field = 'row',
    ref = 1223,
    db_type = 1003,
    expected = '{"code":"C","name":"N\\u00e9","summary":"S"}',
}
if apply.confirm_token(row) ~= '1223' then
    die('whole-row token=' .. tostring(apply.confirm_token(row)))
end
local row_sql = apply.build_sql(row, { engine = 'sqlite', schema = '' }, tmp)
if not row_sql then
    die('whole-row SQL missing')
end
if not row_sql:find("code = 'C'", 1, true)
    or not row_sql:find('name = \'N\195\169\'', 1, true)
    or not row_sql:find("summary = 'S'", 1, true) then
    die('whole-row SQL did not set code, name, and summary')
end
local review = table.concat(queue.build_review_lines(row), '\n')
if not review:find('does not replay DDL', 1, true) then
    die('review copy does not say a metadata replace skips DDL')
end

local connect_path = env.schemagui .. '/lua/schemahelper_connect.lua'
local cf = io.open(connect_path, 'r')
if not cf then
    die('cannot read connect module')
end
local src = cf:read('*a')
cf:close()
local at = src:find('SET TRANSACTION;', 1, true)
if not at then
    die('firebird exec_sql has no SET TRANSACTION')
end
local window = src:sub(math.max(1, at - 180), at + 180)
if not window:find('isql-fb', 1, true) or not window:find('COMMIT;', 1, true) then
    die('firebird transaction is not around isql-fb')
end

os.execute('rm -rf "' .. tmp .. '"')
print('OK: dialect SQL text for eight engines, no execution')
os.exit(0)
