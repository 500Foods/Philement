-- Live schematooltest round trip on the eight Test 40 demo databases.
-- Each step is schemahelper_apply.build_sql then schemahelper_connect.exec_sql.
-- SQLite DROP COLUMN is a pass when the tool asks for a table rebuild.
-- The table is dropped again at the end, including after a failed step.
-- Prints one OK line only after every engine has passed.

local env = require('schemahelper_test_env')
local apply = require('schemahelper_apply')
local connect = require('schemahelper_connect')

local TABLE = 'schematooltest'
local CAFE = 'Caf' .. string.char(0xC3, 0xA9)
local ZOE = 'Zoe'

local ENGINES = {
    { id = 'postgresql', schema = 'demo' },
    { id = 'mysql', schema = 'demo' },
    { id = 'sqlite', schema = '' },
    { id = 'db2', schema = 'demo' },
    { id = 'mariadb', schema = 'demo' },
    { id = 'firebird', schema = '' },
    { id = 'yugabytedb', schema = 'demo' },
    { id = 'mssql', schema = 'demoms' },
}

local function die(msg)
    print('ERR: ' .. msg)
    os.exit(1)
end

local function trim_err(text)
    local s = tostring(text or ''):gsub('%s+', ' ')
    s = s:gsub("USING%s+'[^']*'", "USING '***'")
    s = s:gsub('PWD=[^%s]+', 'PWD=***')
    local code = s:match('(SQL%d%d%d%d[A-Z].*)')
        or s:match('(Msg %d+, Level %d+.*)')
    if code then
        s = code
    end
    if #s > 200 then
        s = s:sub(1, 200)
    end
    return s
end

local function write_text(path, data)
    local f = io.open(path, 'wb')
    if not f then
        die('cannot write ' .. path)
    end
    f:write(data)
    f:close()
end

local function catalog_text(with_city)
    local cols = table.concat({
        '{"name":"id","data_type":"integer","nullable":false},',
        '{"name":"name","data_type":"varchar(40)","nullable":true}',
    })
    if with_city then
        cols = cols .. ','
            .. '{"name":"city","data_type":"varchar(40)","nullable":true}'
    end
    return '{"tables":[{"table":"schematooltest","columns":['
        .. cols
        .. '],"primary_key":["id"]}]}'
end

local function js_quote(text)
    local s = tostring(text or '')
    s = s:gsub('\\', '\\\\')
    s = s:gsub('"', '\\"')
    return '"' .. s .. '"'
end

local function insert_expected(key, name)
    return '{"__keys":"id","__order":"id,name","__nums":"id",'
        .. '"id":"' .. key .. '","name":' .. js_quote(name) .. '}'
end

local function delete_expected(key)
    return '{"__keys":"id","__order":"id","__nums":"id","id":"'
        .. key .. '"}'
end

local function sql_error(out)
    local t = tostring(out or '')
    local state = t:match('SQLSTATE[= ]+(%d%d%d%d%d)')
    if state and state ~= '00000' then
        return true
    end
    if t:find('SQL%d%d%d%dN') then
        return true
    end
    if t:find('ERROR', 1, true) or t:find('Error', 1, true) then
        return true
    end
    if t:find('Msg ', 1, true) and t:find('Level', 1, true) then
        return true
    end
    if t:find('no such', 1, true) or t:find('does not exist', 1, true) then
        return true
    end
    if t:find('Invalid column', 1, true) then
        return true
    end
    if t:find('must be rebuilt', 1, true) then
        return true
    end
    return false
end

local function absent_table(out)
    local t = tostring(out or '')
    if t:find('not found', 1, true) then
        return true
    end
    if t:find('does not exist', 1, true) then
        return true
    end
    if t:find('no such table', 1, true) then
        return true
    end
    if t:find('SQL0204N', 1, true) or t:find('42S02', 1, true) then
        return true
    end
    return false
end

local function warning_only(out)
    local t = tostring(out or '')
    if sql_error(out) then
        return false
    end
    if t:find('SQL0100W', 1, true) then
        return true
    end
    if t:find('0 record(s) selected', 1, true) then
        return true
    end
    return false
end

local function ran_ok(ran, out)
    if sql_error(out) then
        return false
    end
    if ran or warning_only(out) then
        return true
    end
    return false
end

local function exec_file(wrapper, out_dir, name, sql)
    local path = out_dir .. '/' .. name .. '.sql'
    write_text(path, sql .. '\n')
    return connect.exec_sql(wrapper, path)
end

local function probe(wrapper, out_dir, sql)
    return exec_file(wrapper, out_dir, 'probe', sql)
end

local function check_conn(spec, conn)
    if conn.engine ~= spec.id then
        return 'resolved engine ' .. tostring(conn.engine)
    end
    if not conn.database or conn.database == '' then
        return 'database is empty'
    end
    if (conn.schema or '') ~= spec.schema then
        return 'schema is [' .. tostring(conn.schema) .. ']'
    end
    if spec.id == 'mysql' then
        local host = conn.host or ''
        if host == '' or host == 'localhost' or host == '127.0.0.1' then
            return 'mysql host is empty or local'
        end
    end
    if spec.id == 'sqlite' then
        local f = io.open(conn.database, 'rb')
        if not f then
            return 'sqlite demo file is missing'
        end
        f:close()
        return nil
    end
    local penv = conn.password_env or ''
    if penv == '' or (os.getenv(penv) or '') == '' then
        return 'password env is unset'
    end
    return nil
end

local function hygiene(wrapper, out_dir, engine, qualified)
    local sql
    if engine == 'firebird' then
        sql = 'DROP TABLE ' .. qualified .. ';'
    else
        sql = 'DROP TABLE IF EXISTS ' .. qualified .. ';'
    end
    local ran, out = exec_file(wrapper, out_dir, 'hygiene', sql)
    if ran_ok(ran, out) or absent_table(out) then
        return nil
    end
    if engine ~= 'firebird' then
        ran, out = exec_file(
            wrapper, out_dir, 'hygiene',
            'DROP TABLE ' .. qualified .. ';')
        if ran_ok(ran, out) or absent_table(out) then
            return nil
        end
    end
    return trim_err(out)
end

local function apply_step(wrapper, out_dir, conn, finding)
    local sql, err = apply.build_sql(finding, conn, out_dir)
    if not sql then
        return nil, err or 'refused'
    end
    local ran, out = exec_file(wrapper, out_dir, 'step', sql)
    if not ran_ok(ran, out) then
        return sql, trim_err(out)
    end
    return sql, nil
end

local function must_query(wrapper, out_dir, sql)
    local ran, out = probe(wrapper, out_dir, sql)
    if not ran_ok(ran, out) then
        return nil, trim_err(out)
    end
    return out, nil
end

local function must_miss(wrapper, out_dir, sql)
    local ran, out = probe(wrapper, out_dir, sql)
    if ran_ok(ran, out) then
        return 'query succeeded'
    end
    return nil
end

local function select_sql(qualified, cols, where)
    return 'SELECT ' .. cols .. ' FROM ' .. qualified
        .. ' WHERE ' .. where .. ';'
end

local function write_catalog(out_dir, with_city)
    write_text(out_dir .. '/catalog_expected.json', catalog_text(with_city))
end

local function drop_city(spec, wrapper, out_dir, conn, qualified)
    local finding = {
        class = 'catalog dropped',
        kind = 'dropped',
        object = TABLE,
        column = 'city',
    }
    if spec.id == 'sqlite' then
        local sql, err = apply.build_sql(finding, conn, out_dir)
        if sql ~= nil then
            return 'sqlite DROP COLUMN returned SQL'
        end
        if not tostring(err or ''):find('rebuild', 1, true) then
            return 'sqlite DROP COLUMN: ' .. trim_err(err)
        end
        local _, qerr = must_query(
            wrapper, out_dir,
            select_sql(qualified, 'city', '1 = 0'))
        if qerr then
            return 'sqlite city left after refusal: ' .. qerr
        end
        return nil
    end
    local _, err = apply_step(wrapper, out_dir, conn, finding)
    if err then
        return 'drop column: ' .. err
    end
    local miss = must_miss(
        wrapper, out_dir, select_sql(qualified, 'city', '1 = 0'))
    if miss then
        return 'city still present after drop'
    end
    local _, qerr = must_query(
        wrapper, out_dir, select_sql(qualified, 'id', '1 = 0'))
    if qerr then
        return 'table missing after drop column: ' .. qerr
    end
    return nil
end

local function insert_row(wrapper, out_dir, conn, key, name)
    local _, err = apply_step(wrapper, out_dir, conn, {
        kind = 'row_missing',
        object = TABLE,
        column = key,
        expected = insert_expected(key, name),
    })
    if err then
        return 'insert ' .. key .. ': ' .. err
    end
    return nil
end

local function change_rows(wrapper, out_dir, conn, qualified)
    local err = insert_row(wrapper, out_dir, conn, '1', CAFE)
    if err then
        return err
    end
    err = insert_row(wrapper, out_dir, conn, '9', ZOE)
    if err then
        return err
    end
    local out
    out, err = must_query(
        wrapper, out_dir, select_sql(qualified, 'name', 'id = 1'))
    if err then
        return 'read key 1: ' .. err
    end
    if not tostring(out):find(CAFE, 1, true) then
        return 'key 1 did not store Cafe'
    end
    out, err = must_query(
        wrapper, out_dir, select_sql(qualified, 'name', 'id = 9'))
    if err then
        return 'read key 9: ' .. err
    end
    if not tostring(out):find(ZOE, 1, true) then
        return 'key 9 did not store Zoe'
    end
    local sql
    sql, err = apply.build_sql({
        kind = 'row_present',
        object = TABLE,
        column = '1',
        expected = delete_expected('1'),
    }, conn, out_dir)
    if not sql then
        return 'delete: ' .. trim_err(err)
    end
    if not sql:find('id = 1', 1, true) then
        return 'delete missed key 1'
    end
    if sql:find('id = 9', 1, true) then
        return 'delete named key 9'
    end
    local ran, dout = exec_file(wrapper, out_dir, 'step', sql)
    if not ran_ok(ran, dout) then
        return 'delete: ' .. trim_err(dout)
    end
    out, err = must_query(
        wrapper, out_dir, select_sql(qualified, 'name', 'id = 1'))
    if err then
        return 'read key 1 after delete: ' .. err
    end
    if tostring(out):find(CAFE, 1, true) then
        return 'key 1 still holds Cafe'
    end
    out, err = must_query(
        wrapper, out_dir, select_sql(qualified, 'name', 'id = 9'))
    if err then
        return 'read key 9 after delete: ' .. err
    end
    if not tostring(out):find(ZOE, 1, true) then
        return 'key 9 was removed'
    end
    return nil
end

local function round_trip(spec, wrapper, out_dir, conn, qualified)
    write_catalog(out_dir, false)
    local _, err = apply_step(wrapper, out_dir, conn, {
        class = 'catalog missing table',
        kind = 'table',
        object = TABLE,
    })
    if err then
        return 'create: ' .. err
    end
    local qerr
    _, qerr = must_query(
        wrapper, out_dir, select_sql(qualified, 'id, name', '1 = 0'))
    if qerr then
        return 'table missing after create: ' .. qerr
    end
    local miss = must_miss(
        wrapper, out_dir, select_sql(qualified, 'city', '1 = 0'))
    if miss then
        return 'city existed before add'
    end
    write_catalog(out_dir, true)
    _, err = apply_step(wrapper, out_dir, conn, {
        class = 'catalog missing column',
        kind = 'column',
        object = TABLE,
        column = 'city',
    })
    if err then
        return 'add column: ' .. err
    end
    _, qerr = must_query(
        wrapper, out_dir, select_sql(qualified, 'city', '1 = 0'))
    if qerr then
        return 'city missing after add: ' .. qerr
    end
    err = drop_city(spec, wrapper, out_dir, conn, qualified)
    if err then
        return err
    end
    err = change_rows(wrapper, out_dir, conn, qualified)
    if err then
        return err
    end
    _, err = apply_step(wrapper, out_dir, conn, {
        class = 'catalog dropped',
        kind = 'dropped',
        object = TABLE,
        column = '-',
    })
    if err then
        return 'drop table: ' .. err
    end
    miss = must_miss(
        wrapper, out_dir, select_sql(qualified, 'id', '1 = 0'))
    if miss then
        return 'table still present after drop'
    end
    return nil
end

local function run_engine(spec, root)
    local wrapper = env.schemagui .. '/schematool_' .. spec.id .. '_demo.sh'
    local conn = connect.resolve(wrapper)
    local err = check_conn(spec, conn)
    if err then
        return err
    end
    local out_dir = root .. '/' .. spec.id
    os.execute('mkdir -p "' .. out_dir .. '"')
    local qualified = apply.qualify_table(conn.engine, conn.schema, TABLE)
    err = hygiene(wrapper, out_dir, spec.id, qualified)
    if err then
        return 'cleanup before: ' .. err
    end
    local ok_work, work_err = xpcall(function()
        return round_trip(spec, wrapper, out_dir, conn, qualified)
    end, function(caught)
        return trim_err(debug.traceback(tostring(caught), 2))
    end)
    local final_err = hygiene(wrapper, out_dir, spec.id, qualified)
    if not ok_work or work_err then
        return work_err
    end
    if final_err then
        return 'cleanup after: ' .. final_err
    end
    return nil
end

local function main()
    local handle = io.popen('mktemp -d /tmp/schematooltest72.XXXXXX')
    if not handle then
        die('mktemp failed')
    end
    local root = handle:read('*l')
    handle:close()
    if not root or root == '' then
        die('mktemp returned nothing')
    end
    local failures = {}
    for _, spec in ipairs(ENGINES) do
        print('roundtrip ' .. spec.id .. ' start')
        io.stdout:flush()
        local err = run_engine(spec, root)
        if err then
            print('FAIL ' .. spec.id .. ' ' .. err)
            failures[#failures + 1] = spec.id
        else
            print('roundtrip ' .. spec.id .. ' pass')
        end
    end
    if #failures > 0 then
        print('ERR: round trip failed: ' .. table.concat(failures, ', '))
        print('roundtrip kept ' .. root)
        os.exit(1)
    end
    os.execute('rm -rf "' .. root .. '"')
    print('OK: schematooltest round trip on eight Test 40 databases')
end

main()
