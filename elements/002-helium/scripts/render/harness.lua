-- harness.lua: run two Reports.Render builds on every reporting fixture and
-- compare the results byte for byte (compose and html stages).
-- Usage: lua harness.lua <old_dir> <new_dir> [empty|error]
-- Each dir holds Render.lua and any Render.<Module> bodies, as written by
-- capture.lua. A pre-split dir holds Render.lua only. The third argument sets
-- the lookup 078 reply: empty rows (default) or a query error.
-- Env DUMP=<case>.<stage> writes the new result to harness_dump.txt, and
-- SHOWDIFF=1 writes the last differing pair to harness_a.txt and harness_b.txt,
-- in RENDER_CHECK_OUT (default: the current directory).
-- Exit 1 when any case differs.
--
-- luacheck: no max line length
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Initial version (Reports.Render split tooling)

local C = dofile(((arg[0]:match("^(.*)/[^/]*$")) or ".") .. "/common.lua")
local OLDDIR, NEWDIR, LOOKUP = arg[1], arg[2], arg[3] or "empty"
if not OLDDIR or not NEWDIR then
  io.stderr:write("usage: lua harness.lua <old_dir> <new_dir> [empty|error]\n")
  os.exit(2)
end
local FIX = C.FIXTURES
local PAY = C.PAYLOAD
local OUT = (os.getenv("RENDER_CHECK_OUT") or ".") .. "/"
package.path = PAY .. "?.lua;" .. PAY .. "?/init.lua;" .. package.path
local SVG_URI = "data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAxMCAxMCIgd2lkdGg9IjEwIiBoZWlnaHQ9IjEwIj48cGF0aCBkPSJNMCAwIEwxMCAwIEwxMCAxMCBaIiBmaWxsPSJyZWQiLz48L3N2Zz4="

local readf = C.readf

-- JSON decode: borrow Core's decoder (same input for old and new).
local jenv = setmetatable({ params = {} }, { __index = _G })
assert(load(assert(readf(NEWDIR .. "/Core.lua"), "harness needs Core.lua in new_dir"), "=Core", "t", jenv))()
local function jload(p) local s = readf(p); if not s then return nil end local v, e = jenv.json_decode(s); assert(not e, p .. ": " .. tostring(e)); return v end

local function enc(v, out)
  local t = type(v)
  if t == "table" then
    local n = #v
    local keys = {}
    for k in pairs(v) do keys[#keys + 1] = k end
    local isarr = (#keys == n)
    if isarr then
      out[#out + 1] = "["
      for i = 1, n do if i > 1 then out[#out + 1] = "," end enc(v[i], out) end
      out[#out + 1] = "]"
    else
      table.sort(keys, function(a, b) return tostring(a) < tostring(b) end)
      out[#out + 1] = "{"
      for i, k in ipairs(keys) do if i > 1 then out[#out + 1] = "," end out[#out + 1] = string.format("%q:", tostring(k)); enc(v[k], out) end
      out[#out + 1] = "}"
    end
  elseif t == "string" then out[#out + 1] = string.format("%q", v)
  elseif t == "number" then out[#out + 1] = (math.type(v) == "integer") and tostring(v) or string.format("%.17g", v)
  else out[#out + 1] = tostring(v) end
end
local function ser(v) local o = {}; enc(v, o); return table.concat(o) end

local function deep(v) if type(v) ~= "table" then return v end local c = {} for k, x in pairs(v) do c[k] = deep(x) end return c end

local function make_H(fetched, modsrc)
  local H = { system = {} }
  local result
  H.set_result_json = function(t) result = t end
  H.altquery_sync = function(_, sql)
    if sql:find("scripts", 1, true) and sql:find("Render", 1, true) then
      local rows = {}
      for name in sql:gmatch("'Render%.([%w]+)'") do
        fetched[#fetched + 1] = name
        if modsrc[name] then rows[#rows + 1] = { SCRIPT_NAME = "Render." .. name, CODE = modsrc[name] } end
      end
      return { rows = rows }, nil
    end
    if sql:find("lookup_id = 78", 1, true) then
      if LOOKUP == "error" then return nil, "lookup unavailable" end
      return { rows = {} }, nil
    end
    return nil, "harness: no such query"
  end
  return H, function() return result end
end

local MODS = { "Core", "Format", "Layout", "Data", "Items", "Paging", "Zone", "Aggregate", "Shapes", "Images", "Barcode", "Html", "Metrics" }
local function load_dir(dir)
  local mods = {}
  for _, m in ipairs(MODS) do mods[m] = readf(dir .. "/" .. m .. ".lua") end
  return assert(readf(dir .. "/Render.lua"), dir .. "/Render.lua"), mods
end
local old_src, old_mods = load_dir(OLDDIR)
local render_src, modsrc = load_dir(NEWDIR)

local function run(src, p, modsource)
  local fetched = {}
  local H, get = make_H(fetched, modsource or {})
  local env = setmetatable({ params = deep(p), H = H }, { __index = _G })
  -- Production sandbox: io is nil unless scripts.options.sandbox.io
  -- (Reports.Render). Simulate that: enable io only when Metrics is
  -- present in this build (Phase 79+).
  if not (modsource and modsource.Metrics) then
    rawset(env, "io", nil)
  end
  local chunk = assert(load(src, "=render", "t", env))
  local ok, rv = pcall(chunk)
  return ser({ ok = ok, rv = ok and rv or tostring(rv), result = get() }), fetched
end

-- cases
local cases = {}
local defs = {}
for name in io.popen("ls '" .. FIX .. "'"):lines() do
  local base = name:match("^(.-)%.definition%.json$")
  if base then defs[#defs + 1] = base end
end
table.sort(defs)
for _, base in ipairs(defs) do
  local def = jload(FIX .. base .. ".definition.json")
  local src = jload(FIX .. base .. ".sources.json")
  local variants = { { tag = base, params = jload(FIX .. base .. ".params.json") or {} } }
  if base == "system" then
    variants = { { tag = "system.anon", params = jload(FIX .. "system.anon.params.json") }, { tag = "system.claim", params = jload(FIX .. "system.claim.params.json") } }
  end
  for _, v in ipairs(variants) do cases[#cases + 1] = { tag = v.tag, def = def, src = src, params = v.params } end
end
-- synthetic: barcodes, field image, field shape, field datetime with a zone
do
  local def = jload(FIX .. "detail.definition.json")
  local band
  for _, b in ipairs(def.bands) do if b.typeKey == 15 then band = b end end
  band.height = "1.5 in"
  local function box(t) return { left = "0 in", top = t, width = "1.5 in", height = "0.4 in" } end
  local add = {
    { id = "bc1", typeKey = 43, box = box("0.3 in"), barcode = { symbology = "code128", data = "Hello" } },
    { id = "bc2", typeKey = 45, datasource = "orders", field = "customer", box = box("0.3 in"), barcode = { symbology = "qr" } },
    { id = "bc3", typeKey = 43, box = box("0.3 in"), barcode = { symbology = "nope", data = "x" } },
    { id = "img1", typeKey = 22, datasource = "orders", field = "logo", box = box("0.7 in") },
    { id = "shp1", typeKey = 26, datasource = "orders", field = "kind", box = box("1.0 in") },
    { id = "dt1", typeKey = 24, field = "ts", timezone = "UTC", mask = "yyyy-MM-dd HH:mm", box = box("1.1 in") },
  }
  for _, it in ipairs(add) do band.items[#band.items + 1] = it end
  local src = { orders = { { customer = "North", amount = 1234.5, logo = SVG_URI, kind = "ellipse", ts = 1772719629 },
                           { customer = "South", amount = 7, logo = "not an image", kind = "zigzag", ts = 1772719000 } } }
  cases[#cases + 1] = { tag = "synthetic.extras", def = def, src = src, params = { now = 1772719629 } }
end

local fails = 0
for _, c in ipairs(cases) do
  for _, stage in ipairs({ "compose", "html" }) do
    local p = c.params and deep(c.params) or {}
    p.stage = stage
    p.definition = c.def
    if c.src then p.sources = c.src end
    p._hydrogen = type(p._hydrogen) == "table" and p._hydrogen or {}
    p._hydrogen.database = "acz"
    local a = run(old_src, p, old_mods)
    local b, fetched = run(render_src, p, modsrc)
    local same = (a == b)
    if not same then fails = fails + 1 end
    local warns = select(2, a:gsub('"warn"', ""))
    print(string.format("%-20s %-8s %s bytes=%d warns=%d fetched=%s", c.tag, stage, same and "IDENTICAL" or "DIFF", #a, warns, table.concat(fetched, ",")))
    if os.getenv("DUMP") == c.tag .. "." .. stage then C.writef(OUT .. "harness_dump.txt", b) end
    if not same and os.getenv("SHOWDIFF") then
      C.writef(OUT .. "harness_a.txt", a); C.writef(OUT .. "harness_b.txt", b)
    end
  end
end
print("cases=" .. #cases * 2 .. " diffs=" .. fails)
os.exit(fails == 0 and 0 or 1)
