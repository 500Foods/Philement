-- schemahelper_connect.lua
-- Resolve wrapper credentials and ping the live DB. Never return passwords.
--
-- CHANGELOG
-- 0.4.8 - 2026-08-23 - Source wrapper exec line for computed host/password-env
-- 0.4.5 - 2026-08-23 - Parse wrapper --engine/--host/--database flags for ping
-- 0.4.1 - 2026-08-23 - Only resolve sqlite database as a file path
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

local function expand_token(tok)
    tok = tostring(tok or "")
    tok = tok:gsub("\\%s*$", "")
    tok = tok:gsub('^"(.*)"$', "%1")
    tok = tok:gsub("^'(.*)'$", "%1")
    tok = tok:gsub("%${([%w_]+)}", function(name)
        return getenv(name)
    end)
    tok = tok:gsub("%$([%w_]+)", function(name)
        return getenv(name)
    end)
    return tok
end

local function usable(val)
    if not val or val == "" then
        return false
    end
    if val:match("^%$") then
        return false
    end
    if val == "null" then
        return false
    end
    return true
end

local CLI_FLAGS = {
    "engine", "host", "port", "user", "database", "schema",
    "password-env", "design", "migrations",
}

local function read_wrapper_cli(path)
    local flags = {}
    local exports = {}
    local f = io.open(path, "r")
    if not f then
        return flags, exports
    end
    local pending
    for raw in f:lines() do
        local line = raw:gsub("^%s+", ""):gsub("%s+$", "")
        if line ~= "" and not line:match("^#") then
            local key, val = line:match('^export%s+(SCHEMATOOL_DB_%w+)="([^"]*)"')
            if key then
                exports[key] = val
            end
            local matched = false
            for i = 1, #CLI_FLAGS do
                local name = CLI_FLAGS[i]
                local tok = line:match("%-%-" .. name:gsub("%-", "%%-") .. "%s+(%S+)")
                if tok then
                    flags[name] = expand_token(tok)
                    pending = nil
                    matched = true
                    break
                end
            end
            if pending and not matched and not line:match("^%-%-") then
                local tok = line:match("^(%S+)")
                if tok then
                    flags[pending] = expand_token(tok)
                end
                pending = nil
            elseif not matched then
                for i = 1, #CLI_FLAGS do
                    local name = CLI_FLAGS[i]
                    if line:match("%-%-" .. name:gsub("%-", "%%-") .. "%s*$") then
                        pending = name
                        break
                    end
                end
            end
        end
    end
    f:close()
    return flags, exports
end

function M.parse_wrapper(path)
    local flags, exports = read_wrapper_cli(path)
    return flags, exports
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
    out = out:gsub("%s+$", "")
    return ok == true, out
end

local function harvest_exec_flags(wrapper)
    local script = string.format([[
exec() {
    while [ "$#" -gt 0 ]; do
        case "$1" in
            --host|--port|--user|--database|--schema|--engine|--password-env|--design|--migrations)
                printf 'flag:%%s=%%s\n' "${1#--}" "$2"
                shift 2
                ;;
            *)
                shift
                ;;
        esac
    done
    exit 0
}
# shellcheck disable=SC1090
. %s
]], sh_quote(wrapper))
    local _, out = run_shell(script)
    local flags = {}
    if not out or out == "" then
        return flags
    end
    for key, val in out:gmatch("flag:([%w%-]+)=([^\n]*)") do
        val = val:gsub("%s+$", "")
        if usable(val) then
            flags[key] = val
        end
    end
    return flags
end

local function scrub(text, pass)
    text = tostring(text or "")
    if pass ~= "" then
        text = text:gsub(pass:gsub("(%W)", "%%%1"), "***")
    end
    return text
end

local function apply_family(conn, engine, wrapper)
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
end

function M.resolve(wrapper)
    local stem = wrapper_engine(wrapper)
    local flags, exports = read_wrapper_cli(wrapper)
    local engine = flags.engine or stem
    local conn = {
        ok = false,
        engine = engine,
        stem = stem,
        family = "",
        host = "",
        port = "",
        user = "",
        database = "",
        schema = "",
        password_env = "",
        design = flags.design or "",
        detail = "not checked",
    }
    apply_family(conn, engine, wrapper)

    if usable(exports.SCHEMATOOL_DB_SCHEMA) then
        conn.schema = exports.SCHEMATOOL_DB_SCHEMA
    end
    if usable(exports.SCHEMATOOL_DB_HOST) then
        conn.host = exports.SCHEMATOOL_DB_HOST
    end
    if usable(exports.SCHEMATOOL_DB_PORT) then
        conn.port = exports.SCHEMATOOL_DB_PORT
    end
    if usable(flags.host) then
        conn.host = flags.host
    end
    if usable(flags.port) then
        conn.port = flags.port
    end
    if usable(flags.user) then
        conn.user = flags.user
    end
    if usable(flags.database) then
        conn.database = flags.database
    end
    if usable(flags.schema) then
        conn.schema = flags.schema
    end
    if usable(flags["password-env"]) then
        conn.password_env = flags["password-env"]
        conn.family = conn.password_env:gsub("_PASS$", "_*")
    end
    local harvested = harvest_exec_flags(wrapper)
    if usable(harvested.engine) then
        conn.engine = harvested.engine
    end
    if usable(harvested.host) then
        conn.host = harvested.host
    end
    if usable(harvested.port) then
        conn.port = harvested.port
    end
    if usable(harvested.user) then
        conn.user = harvested.user
    end
    if usable(harvested.database) then
        conn.database = harvested.database
    end
    if usable(harvested.schema) then
        conn.schema = harvested.schema
    end
    if usable(harvested.design) then
        conn.design = harvested.design
    end
    if usable(harvested["password-env"]) then
        conn.password_env = harvested["password-env"]
        conn.family = conn.password_env:gsub("_PASS$", "_*")
    end
    if conn.engine == "sqlite" and conn.database ~= "" then
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

local function ping_via_wrapper(wrapper, conn)
    local script = string.format([[
exec() {
    host=""; port=""; user=""; database=""; schema=""; engine=""; password_env=""
    while [ "$#" -gt 0 ]; do
        case "$1" in
            --host) host="$2"; shift 2 ;;
            --port) port="$2"; shift 2 ;;
            --user) user="$2"; shift 2 ;;
            --database) database="$2"; shift 2 ;;
            --schema) schema="$2"; shift 2 ;;
            --engine) engine="$2"; shift 2 ;;
            --password-env) password_env="$2"; shift 2 ;;
            *) shift ;;
        esac
    done
    if [ -z "$engine" ]; then
        engine=%s
    fi
    case "$engine" in
        postgresql|cockroachdb|yugabytedb)
            if [ -n "$password_env" ]; then
                eval "export PGPASSWORD=\"\${$password_env}\""
            fi
            psql -h "$host" -p "$port" -U "$user" -d "$database" -v ON_ERROR_STOP=1 -t -A -c "SELECT 1"
            ;;
        mysql|mariadb)
            if [ -n "$password_env" ]; then
                eval "export MYSQL_PWD=\"\${$password_env}\""
            fi
            db="$schema"
            if [ -z "$db" ]; then
                db="$database"
            fi
            mysql -h "$host" -P "$port" -u "$user" "$db" -N -e "SELECT 1"
            ;;
        sqlite)
            sqlite3 "file:${database}?mode=ro" "SELECT 1"
            ;;
        db2)
            . /home/db2inst1/sqllib/db2profile >/dev/null 2>&1 || true
            command -v db2 >/dev/null || { echo db2 client not found; exit 1; }
            eval "pw=\"\${$password_env}\""
            db2 -tv +o <<EOF
CONNECT TO $database USER $user USING '$pw';
VALUES 1;
CONNECT RESET;
EOF
            ;;
        *)
            echo "unsupported engine"
            exit 1
            ;;
    esac
    exit $?
}
# shellcheck disable=SC1090
. %s
]], sh_quote(conn.engine or ""), sh_quote(wrapper))
    return run_shell(script)
end

function M.probe(wrapper)
    local conn = M.resolve(wrapper)
    local ok, out
    local need_wrap = conn.password_env ~= "" and getenv(conn.password_env) == ""
    if need_wrap then
        ok, out = ping_via_wrapper(wrapper, conn)
    elseif conn.engine == "postgresql" or conn.engine == "cockroachdb"
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
        out = tostring(out or ""):gsub("\n", " "):gsub("%s+", " ")
        conn.detail = scrub(out, pass)
        if conn.detail == "" then
            conn.detail = "failed"
        end
    end
    return conn
end

return M
