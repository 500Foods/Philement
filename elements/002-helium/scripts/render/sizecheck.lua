-- sizecheck.lua: report generated statement and literal sizes for a migration.
-- Usage: lua sizecheck.lua <migration.lua> [engine]   (engine defaults to db2)
-- max_literal is the longest quoted literal in the generated SQL (DB2 rejects a
-- string constant over 32,672 bytes; keep a Render module under ~24 KB).
-- max_body_raw is the largest stored Reports.Render body (MariaDB and MySQL
-- `text` holds 65,535 bytes; keep it under 60 KB).
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Initial version (Reports.Render split tooling)

local C = dofile(((arg[0]:match("^(.*)/[^/]*$")) or ".") .. "/common.lua")
local file, engine = arg[1], arg[2] or "db2"
if not file then
  io.stderr:write("usage: lua sizecheck.lua <migration.lua> [engine]\n")
  os.exit(2)
end

local sql, blocks = C.generate(file, engine)
local maxlit, nstmt, maxstmt = 0, 0, 0
for stmt in (sql .. "\n-- QUERY DELIMITER\n"):gmatch("(.-)\n%-%- QUERY DELIMITER\n") do
  nstmt = nstmt + 1
  if #stmt > maxstmt then maxstmt = #stmt end
  for lit in stmt:gmatch("'([^']*)'") do
    if #lit > maxlit then maxlit = #lit end
  end
end
local braw = 0
for _, b in ipairs(blocks) do
  if b:match("^%s*%-%- Reports%.Render") and #b > braw then braw = #b end
end
print(string.format("%s %s stmts=%d max_stmt=%d max_literal=%d max_body_raw=%d",
  file:match("[^/]+$"), engine, nstmt, maxstmt, maxlit, braw))
