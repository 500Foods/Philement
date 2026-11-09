-- common.lua: shared paths and a Helium generator wrapper for the render tools.
-- Loaded with dofile() by the other tools in this folder. Paths come from the
-- tool's own location, so the tools run from any working directory.
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Initial version (Reports.Render split tooling)

-- luacheck: globals database

local M = {}

local here = (arg and arg[0] or ""):match("^(.*)/[^/]*$") or "."
M.DIR = here .. "/"
M.ROOT = M.DIR .. "../../../../"
M.HELIUM = M.ROOT .. "elements/002-helium/helium/"
M.FIXTURES = M.ROOT .. "elements/003-lithium/tests/fixtures/reporting/"
M.PAYLOAD = M.ROOT .. "elements/001-hydrogen/hydrogen/payloads/lua/"

function M.readf(path)
  local f = io.open(path, "rb")
  if not f then return nil end
  local s = f:read("a")
  f:close()
  return s
end

function M.writef(path, data)
  local f = assert(io.open(path, "wb"))
  f:write(data)
  f:close()
end

-- Run one migration through the Helium generator, the way the Hydrogen runner
-- does. Returns the generated SQL and every string handed to brotli (each
-- long-bracket block, before encoding), in order.
function M.generate(migration, engine)
  package.path = M.HELIUM .. "?.lua;" .. package.path
  local real = require("brotli")
  local blocks = {}
  package.loaded.brotli = {
    compress = function(data, q) blocks[#blocks + 1] = data; return real.compress(data, q) end,
    decompress = real.decompress,
  }
  database = dofile(M.HELIUM .. "database.lua")
  local cfg = database.defaults[engine]
  assert(cfg, "unknown engine " .. tostring(engine))
  local queries = dofile(migration)(engine, "acuranzo", "ACURANZO", cfg)
  local sql = database:run_migration(queries, engine, "acuranzo", "ACURANZO")
  return sql, blocks
end

return M
