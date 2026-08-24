-- schemahelper_smoke_queue.lua
-- Headless smoke test for schemahelper_queue + schemahelper_packet.
-- Exercises queue build, field-level finding IDs, next_ref, and packet
-- collision/write against checked-in fixtures in
-- test/fixtures/sample_project/ (no live database required).
--
-- CHANGELOG
-- 0.5.6 - 2026-08-24 - Initial headless smoke: queue totals, finding IDs,
--   next_ref=1291, --ref 1148 collision, --ref 2000 packet write.

-- luacheck: globals arg package

local function fail(msg)
    io.stderr:write("SMOKE FAIL: " .. tostring(msg) .. "\n")
    os.exit(1)
end

local function pass(msg)
    print("OK: " .. tostring(msg))
end

local function check(cond, msg)
    if cond then
        pass(msg)
    else
        fail(msg)
    end
end

local script_dir
do
    local src = arg[0] or ""
    local d = src:match("^(.*)/[^/]+$") or "."
    script_dir = d
end
package.path = script_dir .. "/?.lua;" .. package.path

local queue = require("schemahelper_queue")
local packet = require("schemahelper_packet")

local opts = {
    out_dir = "",
    migrations = "",
    design = "acuranzo",
    engine = "sqlite",
    state_file = "",
    packet_dir = "",
    ref = 0,
}

local i = 1
while i <= #arg do
    local a = arg[i]
    if a == "--out-dir" and i < #arg then
        opts.out_dir = arg[i + 1]
        i = i + 2
    elseif a == "--migrations" and i < #arg then
        opts.migrations = arg[i + 1]
        i = i + 2
    elseif a == "--design" and i < #arg then
        opts.design = arg[i + 1]
        i = i + 2
    elseif a == "--engine" and i < #arg then
        opts.engine = arg[i + 1]
        i = i + 2
    elseif a == "--state-file" and i < #arg then
        opts.state_file = arg[i + 1]
        i = i + 2
    elseif a == "--packet-dir" and i < #arg then
        opts.packet_dir = arg[i + 1]
        i = i + 2
    elseif a == "--ref" and i < #arg then
        opts.ref = tonumber(arg[i + 1]) or 0
        i = i + 2
    else
        i = i + 1
    end
end

if opts.out_dir == "" then
    fail("--out-dir is required (fixture directory with findings.json)")
end

if opts.state_file == "" then
    opts.state_file = queue.default_state_path(opts.out_dir, opts.design, opts.engine)
end

if opts.migrations == "" then
    opts.migrations = opts.out_dir .. "/migrations"
end

local function file_exists(path)
    if not path or path == "" then
        return false
    end
    local f = io.open(path, "r")
    if not f then
        return false
    end
    f:close()
    return true
end

check(queue.artifacts_present(opts.out_dir, "both"),
    "artifacts present in out-dir")

local state = queue.load_state(opts.state_file)
check(state.design == opts.design, "state design = " .. opts.design)
check(state.engine == opts.engine, "state engine = " .. opts.engine)
check(state.cursor_id == "meta:drift:1148:1003:name",
    "cursor_id = meta:drift:1148:1003:name (field-level :name, not :code)")

local built = queue.build({
    out_dir = opts.out_dir,
    track = "both",
    state = state,
})

check(built.totals.total == 4,
    "totals.total == 4 (got " .. built.totals.total .. ")")
check(built.totals.perfect == 1,
    "totals.perfect == 1 (got " .. built.totals.perfect .. ")")
check(built.totals.accepted == 1,
    "totals.accepted == 1 (got " .. built.totals.accepted .. ")")
check(built.totals.subject == 4,
    "totals.subject == 4 (got " .. built.totals.subject .. ")")

local found_name_drift = queue.find_finding(built.findings,
    "meta:drift:1148:1003:name")
check(found_name_drift ~= nil,
    "finding id meta:drift:1148:1003:name exists (field-level variance on :name)")
check(found_name_drift.action == "accepted",
    "finding meta:drift:1148:1003:name action = accepted")

local found_code_drift = queue.find_finding(built.findings,
    "meta:drift:1148:1003:code")
check(found_code_drift == nil,
    "finding id meta:drift:1148:1003:code does not exist (no :code split)")

local packet_opts = {
    migrations = opts.migrations,
    packet_dir = opts.packet_dir,
    design = opts.design,
    engine = opts.engine,
    ref = opts.ref,
}

local next_ref = packet.next_ref(packet_opts)
check(next_ref == 1291,
    "next_ref == 1291 (got " .. tostring(next_ref) .. ")")

local collide_1148 = packet.collision(packet_opts, 1148)
check(collide_1148 ~= nil,
    "--ref 1148 collides with design_1148.lua on disk")

local collide_2000, dest_2000 = packet.collision(packet_opts, 2000)
check(collide_2000 == nil,
    "--ref 2000 does not collide (free ref)")

local orphan_finding = queue.find_finding(built.findings, "orphan:1290")
check(orphan_finding ~= nil,
    "orphan finding orphan:1290 exists for packet test")

local write_opts = {
    migrations = opts.migrations,
    packet_dir = opts.packet_dir,
    out_dir = opts.out_dir,
    design = opts.design,
    engine = opts.engine,
    schema = state.schema or "",
    ref = 2000,
    schemahelper_version = "0.5.6",
    schematool_version = "1.8.3",
}

local written, write_err = packet.write(write_opts, orphan_finding, {
    note = "smoke test packet",
    detail_text = "orphan ref 1290 in DB, not on disk",
})
check(written ~= nil,
    "packet.write ref=2000 succeeds: " .. tostring(write_err or "ok"))
check(written.ref == 2000,
    "written packet ref == 2000")

check(file_exists(dest_2000 .. "/MANIFEST.json"),
    "MANIFEST.json written")
check(file_exists(dest_2000 .. "/PACKET.md"),
    "PACKET.md written")
check(file_exists(dest_2000 .. "/FINDING.json"),
    "FINDING.json written")
check(file_exists(dest_2000 .. "/DETAIL.txt"),
    "DETAIL.txt written")
check(file_exists(dest_2000 .. "/SUGGESTED.sql"),
    "SUGGESTED.sql written")

os.execute('rm -rf "' .. dest_2000 .. '"')

return {
    total = built.totals.total,
    perfect = built.totals.perfect,
    accepted = built.totals.accepted,
    subject = built.totals.subject,
    next_ref = next_ref,
}
