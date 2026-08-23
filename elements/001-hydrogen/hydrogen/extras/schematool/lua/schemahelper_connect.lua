-- schemahelper_connect.lua
-- Resolve wrapper credentials and ping the live DB. Never return passwords.
--
-- CHANGELOG
-- 0.2.2 - 2026-08-23 - Lua connect probe (replaces schemahelper_connect.sh)

local M = {}

local function getenv(name)
    return os.getenv(name) or ""
end

local function wrapper_engine(path)
    local base = path:match("([^/]+)$") or path
    return base:match("^schematool_(.+)%.sh$") or ""
end

local function wrapper_dir(path)
    return path:match("^(.*)/[^/]+$") or "."
end

local function read_wrapper_exports(path)
    local out = {}
    local f = io.open(path, "r")
    if not f then
        return out
    end
    for line in f:lines() do
        local key, val = line:match('^export%s+(SCHEMATOOL_DB_%w+)="([^"]*)"')
        if key then
            out[key] = val
        end
    end
    f:close()
    return out
end

local function abs_file(path)
    if path == "" then
        return path
    end
    local dir = path:match("^(.*)/[^/]+$") or "."
    local base = path:match("([^/]+)$") or path
    local h = io.popen('cd "' .. dir:gsub('"', '\\"') .. '" 2>/dev/null && pwd')
    if not h then
        return path
    end
    local abs = (h:read("*l") or ""):gsub("%s+$", "")
    h:close()
    if abs == "" then
        return path
    end
    return abs .. "/" .. base
end

local function have_cmd(name)
    local h = io.popen("command -v " .. name .. " >/dev/null 2>&1 && printf yes")
    if not h then
        return false
    end
    local out = h:read("*a") or ""
    h:close()
    return out == "yes"
end

local function sh_quote(s)
    return "'" .. tostring(s):gsub("'", "'\\''") .. "'"
end

local function run_shell(script)
    local path = os.tmpname()
    local f, err = io.open(path, "w")
    if not f then
        return false, "cannot write temp script: " .. tostring(err)
    end
    f:write(script)
    f:close()
    local h = io.popen("bash " .. sh_quote(path) .. " 2>&1")
    if not h then
        os.remove(path)
        return false, "shell failed"
    end
    local out = h:read("*a") or ""
    local ok = h:close()
    os.remove(path)
    out = out:gsub("%s+$", ""):gsub("\n", " ")
    return ok == true, out
end

local function scrub(text, pass)
    text = tostring(text or "")
    if pass ~= "" then
        text = text:gsub(pass:gsub("(%W)", "%%%1"), "***")
    end
    return text
end

function M.resolve(wrapper)
    local engine = wrapper_engine(wrapper)
    local exports = read_wrapper_exports(wrapper)
    local conn = {
        ok = false,
        engine = engine,
        family = "",
        host = "",
        port = "",
        user = "",
        database = "",
        schema = "",
        password_env = "",
        detail = "not checked",
    }

    if engine == "yugabytedb" then
        conn.family = "YUGABYTE_DB_*"
        conn.host = getenv("YUGABYTE_DB_HOST")
        conn.port = getenv("YUGABYTE_DB_PORT")
        if conn.port == "" then
            conn.port = "5433"
        end
        conn.user = getenv("YUGABYTE_DB_USER")
        conn.database = getenv("YUGABYTE_DB_NAME")
        conn.password_env = "YUGABYTE_DB_PASS"
        conn.schema = getenv("YUGABYTE_DB_SCHEMA")
        if conn.schema == "" then
            conn.schema = "demo"
        end
    elseif engine == "postgresql" or engine == "cockroachdb" then
        conn.family = "ACURANZO_DB_*"
        conn.host = getenv("ACURANZO_DB_HOST")
        conn.port = getenv("ACURANZO_DB_PORT")
        if conn.port == "" then
            conn.port = "5432"
        end
        conn.user = getenv("ACURANZO_DB_USER")
        conn.database = getenv("ACURANZO_DB_NAME")
        conn.password_env = "ACURANZO_DB_PASS"
        conn.schema = engine == "cockroachdb" and "democrdb" or "demo"
    elseif engine == "mysql" or engine == "mariadb" then
        conn.family = "CANVAS_DB_*"
        conn.host = getenv("CANVAS_DB_HOST")
        conn.port = getenv("CANVAS_DB_PORT")
        if conn.port == "" then
            conn.port = "3306"
        end
        conn.user = getenv("CANVAS_DB_USER")
        conn.database = getenv("CANVAS_DB_NAME")
        conn.password_env = "CANVAS_DB_PASS"
        conn.schema = engine == "mariadb" and "demomrdb" or "demo"
    elseif engine == "db2" then
        conn.family = "HYDROTST_DB_*"
        conn.host = "localhost"
        conn.port = "55555"
        conn.user = getenv("HYDROTST_DB_USER")
        conn.database = getenv("HYDROTST_DB_NAME")
        conn.password_env = "HYDROTST_DB_PASS"
        conn.schema = "demo"
    elseif engine == "sqlite" then
        conn.family = "file"
        local root = getenv("HYDROGEN_ROOT")
        if root ~= "" then
            conn.database = root .. "/tests/artifacts/database/sqlite/hydrodemo.sqlite"
        else
            conn.database = wrapper_dir(wrapper)
                .. "/../../../tests/artifacts/database/sqlite/hydrodemo.sqlite"
        end
    else
        conn.family = "SCHEMATOOL_DB_*"
        conn.host = getenv("SCHEMATOOL_DB_HOST")
        conn.port = getenv("SCHEMATOOL_DB_PORT")
        conn.user = getenv("SCHEMATOOL_DB_USER")
        conn.database = getenv("SCHEMATOOL_DB_NAME")
        conn.schema = getenv("SCHEMATOOL_DB_SCHEMA")
        if getenv("SCHEMATOOL_DB_PASS") ~= "" then
            conn.password_env = "SCHEMATOOL_DB_PASS"
        end
    end

    if exports.SCHEMATOOL_DB_SCHEMA then
        conn.schema = exports.SCHEMATOOL_DB_SCHEMA
    end
    if exports.SCHEMATOOL_DB_HOST then
        conn.host = exports.SCHEMATOOL_DB_HOST
    end
    if exports.SCHEMATOOL_DB_PORT then
        conn.port = exports.SCHEMATOOL_DB_PORT
    end
    if conn.database ~= "" then
        conn.database = abs_file(conn.database)
    end
    return conn
end

local function ping_pg(conn)
    if not have_cmd("psql") then
        return false, "psql not found"
    end
    if conn.host == "" or conn.user == "" or conn.database == "" then
        return false, "missing host/user/database"
    end
    if conn.password_env ~= "" and getenv(conn.password_env) == "" then
        return false, conn.password_env .. " not set"
    end
    local script = string.format(
        'export PGPASSWORD="${%s}"\npsql -h %s -p %s -U %s -d %s -v ON_ERROR_STOP=1 -t -A -c "SELECT 1"\n',
        conn.password_env,
        sh_quote(conn.host),
        sh_quote(conn.port),
        sh_quote(conn.user),
        sh_quote(conn.database)
    )
    return run_shell(script)
end

local function ping_mysql(conn)
    if not have_cmd("mysql") then
        return false, "mysql client not found"
    end
    if conn.host == "" or conn.user == "" or conn.database == "" then
        return false, "missing host/user/database"
    end
    if conn.password_env ~= "" and getenv(conn.password_env) == "" then
        return false, conn.password_env .. " not set"
    end
    local db = conn.schema ~= "" and conn.schema or conn.database
    local script = string.format(
        'export MYSQL_PWD="${%s}"\nmysql -h %s -P %s -u %s %s -N -e "SELECT 1"\n',
        conn.password_env,
        sh_quote(conn.host),
        sh_quote(conn.port),
        sh_quote(conn.user),
        sh_quote(db)
    )
    return run_shell(script)
end

local function ping_sqlite(conn)
    if not have_cmd("sqlite3") then
        return false, "sqlite3 not found"
    end
    if conn.database == "" then
        return false, "missing sqlite file path"
    end
    local f = io.open(conn.database, "r")
    if not f then
        return false, "file not found"
    end
    f:close()
    return run_shell("sqlite3 " .. sh_quote("file:" .. conn.database .. "?mode=ro") .. ' "SELECT 1"\n')
end

local function ping_db2(conn)
    if conn.user == "" or conn.database == "" then
        return false, "missing user/database"
    end
    if conn.password_env ~= "" and getenv(conn.password_env) == "" then
        return false, conn.password_env .. " not set"
    end
    local script = string.format(
        ". /home/db2inst1/sqllib/db2profile >/dev/null 2>&1 || true; "
            .. "command -v db2 >/dev/null || { echo db2 client not found; exit 1; }; "
            .. "db2 -tv +o <<EOF\n"
            .. "CONNECT TO %s USER %s USING '${%s}';\n"
            .. "VALUES 1;\n"
            .. "CONNECT RESET;\n"
            .. "EOF",
        conn.database,
        conn.user,
        conn.password_env
    )
    return run_shell(script)
end

function M.probe(wrapper)
    local conn = M.resolve(wrapper)
    local ok, out
    if conn.engine == "postgresql" or conn.engine == "cockroachdb"
        or conn.engine == "yugabytedb" then
        ok, out = ping_pg(conn)
    elseif conn.engine == "mysql" or conn.engine == "mariadb" then
        ok, out = ping_mysql(conn)
    elseif conn.engine == "sqlite" then
        ok, out = ping_sqlite(conn)
    elseif conn.engine == "db2" then
        ok, out = ping_db2(conn)
    else
        ok, out = false, "unsupported engine"
    end
    local pass = ""
    if conn.password_env ~= "" then
        pass = getenv(conn.password_env)
    end
    conn.ok = ok and true or false
    if ok then
        conn.detail = "connected"
    else
        conn.detail = scrub(out, pass)
        if conn.detail == "" then
            conn.detail = "failed"
        end
    end
    return conn
end

return M
