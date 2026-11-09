-- capture.lua: write a Reports.Render body exactly as the runner stores it.
-- Usage: lua capture.lua <migration.lua> <out_dir>
-- Takes the first body that starts with "-- Reports.Render" (the forward body)
-- and writes <out_dir>/<Module>.lua, or <out_dir>/Render.lua for the loader or
-- the single pre-split body. Prints the file, bytes, and lines.
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Initial version (Reports.Render split tooling)

local C = dofile(((arg[0]:match("^(.*)/[^/]*$")) or ".") .. "/common.lua")
local migration, out = arg[1], arg[2]
if not migration or not out then
  io.stderr:write("usage: lua capture.lua <migration.lua> <out_dir>\n")
  os.exit(2)
end

local _, blocks = C.generate(migration, "postgresql")
local body
for _, b in ipairs(blocks) do
  if b:match("^%s*%-%- Reports%.Render") then body = b; break end
end
if not body then
  io.stderr:write(migration .. ": no Reports.Render body\n")
  os.exit(1)
end
local name = body:match("^%s*%-%- Reports%.Render%.(%w+)") or "Render"
local path = out .. "/" .. name .. ".lua"
C.writef(path, body)
print(string.format("%s %d bytes %d lines", path, #body, select(2, body:gsub("\n", "")) + 1))
