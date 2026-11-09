-- jsoncheck.lua: every JSON_INGEST literal in the DB2 SQL must be a JSON object.
-- Usage: lua jsoncheck.lua <migration.lua> [...]
-- Generates DB2 SQL, decodes each BASE64DECODE block (nested blocks too), and
-- parses every JSON_INGEST('...') literal. DB2 JSON2BSON rejects a top-level
-- array, so anything that is not a valid object fails. Exit 1 on a failure.
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Initial version (Reports.Render split tooling)

local C = dofile(((arg[0]:match("^(.*)/[^/]*$")) or ".") .. "/common.lua")
if not arg[1] then
  io.stderr:write("usage: lua jsoncheck.lua <migration.lua> [...]\n")
  os.exit(2)
end

local B64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
local function unb64(s)
  s = s:gsub("[^%w%+/]", "")
  local out, bits, nbits = {}, 0, 0
  for i = 1, #s do
    bits = bits * 64 + (B64:find(s:sub(i, i), 1, true) - 1)
    nbits = nbits + 6
    if nbits >= 8 then
      nbits = nbits - 8
      local byte = bits // (1 << nbits)
      out[#out + 1] = string.char(byte)
      bits = bits - byte * (1 << nbits)
    end
  end
  return table.concat(out)
end

-- Minimal strict JSON validator. Returns true when s is one JSON value.
local function valid_json(s)
  local pos = 1
  local value
  local function ws() pos = s:find("[^ \t\r\n]", pos) or #s + 1 end
  local function str()
    pos = pos + 1
    while true do
      local c = s:sub(pos, pos)
      if c == "" then return false end
      if c == '"' then pos = pos + 1; return true end
      if c == "\\" then
        local e = s:sub(pos + 1, pos + 1)
        if e == "u" then
          if not s:sub(pos + 2, pos + 5):match("^%x%x%x%x$") then return false end
          pos = pos + 6
        elseif e:match('["\\/bfnrt]') then pos = pos + 2
        else return false end
      elseif c:byte() < 32 then return false
      else pos = pos + 1 end
    end
  end
  local function seq(close, member)
    pos = pos + 1; ws()
    if s:sub(pos, pos) == close then pos = pos + 1; return true end
    while true do
      if member then
        if s:sub(pos, pos) ~= '"' or not str() then return false end
        ws()
        if s:sub(pos, pos) ~= ":" then return false end
        pos = pos + 1
      end
      if not value() then return false end
      ws()
      local c = s:sub(pos, pos)
      pos = pos + 1
      if c == close then return true end
      if c ~= "," then return false end
      ws()
    end
  end
  value = function()
    ws()
    local c = s:sub(pos, pos)
    if c == "{" then return seq("}", true) end
    if c == "[" then return seq("]", false) end
    if c == '"' then return str() end
    for _, lit in ipairs({ "true", "false", "null" }) do
      if s:sub(pos, pos + #lit - 1) == lit then pos = pos + #lit; return true end
    end
    local n = s:match("^%-?%d+%.?%d*[eE]?[%+%-]?%d*", pos)
    if n and n ~= "" and n ~= "-" then pos = pos + #n; return true end
    return false
  end
  if not value() then return false end
  ws()
  return pos > #s
end

local failed = false
for _, file in ipairs(arg) do
  local sql, blocks = C.generate(file, "db2")
  local queue = { sql }
  for _, b in ipairs(blocks) do queue[#queue + 1] = b end
  local i, n, ok = 0, 0, 0
  while i < #queue do
    i = i + 1
    local blk = queue[i]
    for enc in blk:gmatch("BASE64DECODE%('([^']*)'%)") do
      local d = unb64(enc)
      if d:find("JSON_INGEST", 1, true) then queue[#queue + 1] = d end
    end
    local lits = {}
    for lit in blk:gmatch("JSON_INGEST%(%w+%.BASE64DECODE%('([^']*)'%)") do lits[#lits + 1] = unb64(lit) end
    for lit in blk:gmatch("JSON_INGEST%('([^']*)'%)") do lits[#lits + 1] = lit end
    for _, t in ipairs(lits) do
      n = n + 1
      if t:match("^%s*{") and valid_json(t) then
        ok = ok + 1
      else
        failed = true
        print("NOT AN OBJECT: " .. t:sub(1, 80))
      end
    end
  end
  print(string.format("%s db2 JSON_INGEST literals=%d objects=%d", file:match("[^/]+$"), n, ok))
end
os.exit(failed and 1 or 0)
