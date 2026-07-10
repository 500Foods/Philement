# Lua Language Features

Pure Lua reference for everyday scripting: strings, tables, numbers, dates, patterns, and the standard libraries you will actually use. No Hydrogen `H` API here.

- Hydrogen scripting intro: [LUA_GUIDE.md](/docs/H/LUA_GUIDE.md)
- Host API contract: [lua_api.md](/docs/H/core/subsystems/scripting/lua_api.md)

Hydrogen embeds **Lua 5.4**. Try examples with `lua` or `lua -i`.

---

## Table of Contents

1. [Values, operators, control flow](#values-operators-control-flow)
2. [Functions](#functions)
3. [Strings](#strings)
4. [Multiline and long strings](#multiline-and-long-strings)
5. [Pattern matching](#pattern-matching)
6. [Tables](#tables)
7. [Numbers and precision](#numbers-and-precision)
8. [Math library](#math-library)
9. [Time and date formatting](#time-and-date-formatting)
10. [Iterators, errors, modules](#iterators-errors-modules)
11. [Metatables (short)](#metatables-short)
12. [Recipes](#recipes)
13. [Pitfalls and quick indexes](#pitfalls-and-quick-indexes)

---

## Values, operators, control flow

Eight types: `nil`, `boolean`, `number`, `string`, `table`, `function`, `userdata`, `thread`.

```lua
print(type(42), type("x"), type({}))  -- number string table
```

Only `false` and `nil` are falsey. **`0` and `""` are true.**

```lua
print(10 // 3, 10 / 3, 10 % 3, 2 ^ 10)  -- 3  3.333...  1  1024
print(1 ~= 2, "a" .. "b", #"hello")     -- true  ab  5
print(nil or "default", true and 5)     -- default  5
print(0x0F & 0x03, 1 << 4)              -- 3  16   (bitwise, 5.3+)
```

```lua
if n < 0 then
    sign = -1
elseif n == 0 then
    sign = 0
else
    sign = 1
end

for i = 1, 5 do end
for i = 10, 1, -2 do end
for i, v in ipairs(t) do end
for k, v in pairs(t) do end

local i = 1
while i <= 3 do i = i + 1 end

repeat x = x + 1 until x >= 3
```

No `continue`; use nested `if` or a rare `goto` label. Not-equal is `~=` (not `!=`). Concatenation is `..` (not `+`).

---

## Functions

```lua
local function add(a, b)
    return a + b
end

local function divmod(a, b)
    return a // b, a % b
end
local q, r = divmod(17, 5)   -- 3, 2

local function sum(...)
    local s = 0
    for _, v in ipairs({...}) do s = s + v end
    return s
end

-- named args via table
local function connect(opts)
    return opts.host or "localhost", opts.port or 5432
end
connect{ host = "db", port = 5433 }

-- closure
local function counter()
    local n = 0
    return function() n = n + 1; return n end
end
```

Missing args are `nil`; extra args are discarded. Multiple returns are normal.

---

## Strings

Immutable **byte** sequences. Indexing is **1-based**. There is no built-in `left`/`right`; use `string.sub`.

### Substring (left / right / mid / position)

```lua
local s = "Hello, World"

string.sub(s, 1, 5)      -- "Hello"     left 5
string.sub(s, -5)        -- "World"     right 5
string.sub(s, 8, 12)     -- "World"     mid
string.sub(s, 1, 1)      -- "H"         first char
string.sub(s, -1)        -- "d"         last char
#s                       -- 12

local function left(s, n)   return string.sub(s, 1, n) end
local function right(s, n)  return string.sub(s, -n) end
local function mid(s, i, n) return string.sub(s, i, i + n - 1) end
```

### Find position, contains, starts/ends

```lua
local s = "the quick brown fox"

-- find returns start, end  or nil
local i, j = string.find(s, "quick")     -- 5, 9
string.find(s, "o", 14)                  -- start search at 14
string.find("a+b", "+", 1, true)         -- plain search (no patterns)

local function contains(s, needle)
    return string.find(s, needle, 1, true) ~= nil
end
local function starts_with(s, prefix)
    return string.sub(s, 1, #prefix) == prefix
end
local function ends_with(s, suffix)
    return suffix == "" or string.sub(s, -#suffix) == suffix
end
```

### Search and replace

```lua
-- gsub returns new_string, count
string.gsub("cat bat rat", "at", "ot")       -- cot bot rot, 3
string.gsub("cat bat rat", "at", "ot", 1)    -- only first

-- capture reordering
string.gsub("John Smith", "(%w+) (%w+)", "%2, %1")  -- Smith, John

-- function / table replacement
string.gsub("a1 b2", "%d", function(d)
    return tostring(tonumber(d) * 10)
end)
string.gsub("x y", "%w+", { x = "ex", y = "why" })

-- plain fixed-string replace (no pattern magic)
local function replace_plain(s, old, new)
    local parts, i = {}, 1
    while true do
        local a, b = string.find(s, old, i, true)
        if not a then
            parts[#parts + 1] = string.sub(s, i)
            break
        end
        parts[#parts + 1] = string.sub(s, i, a - 1)
        parts[#parts + 1] = new
        i = b + 1
    end
    return table.concat(parts)
end
```

When you only want the string from `gsub` (discard count): `(string.gsub(...))`.

### Case, reverse, repeat, format, bytes

```lua
string.lower("HTML")           -- html
string.upper("html")           -- HTML
string.reverse("abc")          -- cba
string.rep("-", 10)            -- ----------
string.byte("ABC", 1, 3)       -- 65 66 67
string.char(65, 66, 67)        -- ABC

string.format("%s=%d", "n", 7)
string.format("%05d", 42)      -- 00042
string.format("%.2f", 3.14159) -- 3.14
string.format("%x", 255)       -- ff
string.format("%q", 'a "b"') -- quoted Lua literal
```

Common verbs: `%s %d %i %f %g %e %x %X %c %q %%`.

### Split, join, trim

```lua
local function split(s, sep)
    local out, last = {}, 1
    while true do
        local i, j = string.find(s, sep, last, true)
        if not i then
            out[#out + 1] = string.sub(s, last)
            break
        end
        out[#out + 1] = string.sub(s, last, i - 1)
        last = j + 1
    end
    return out
end

for word in string.gmatch("one two three", "%S+") do
    print(word)
end

table.concat({ "a", "b", "c" }, ", ")   -- a, b, c

local function trim(s)
    return (string.gsub(s, "^%s*(.-)%s*$", "%1"))
end
local function ltrim(s) return (string.gsub(s, "^%s+", "")) end
local function rtrim(s) return (string.gsub(s, "%s+$", "")) end
```

### Conversion and UTF-8

```lua
tostring(123)
tonumber("42")
tonumber("ff", 16)             -- 255
tonumber("nope")               -- nil

-- #s is BYTES; use utf8 for code points (5.3+)
utf8.len("café")
utf8.offset("café", 3)
for p, c in utf8.codes("Hi") do print(p, c) end
```

---

## Multiline and long strings

```lua
local sql = [[
SELECT id, name
  FROM users
 WHERE active = 1
]]
```

- No escape processing inside long strings (`\n` is two characters).
- Quotes need no escaping.
- A **newline immediately after the opening bracket is skipped** (so the example above does not start with a blank line).

If the text contains `]]`, raise the level:

```lua
local a = [=[ text with ]] inside ]=]
local b = [==[ text with ]=] inside ]==]
```

| Opener | Closer | Safe to contain |
|--------|--------|-----------------|
| `[[` | `]]` | anything except `]]` |
| `[=[` | `]=]` | `]]` |
| `[==[` | `]==]` | `]=]` |

Build large text with a table, not repeated `..` in a loop:

```lua
local lines = {}
for i = 1, 3 do
    lines[#lines + 1] = string.format("row %d", i)
end
local body = table.concat(lines, "\n")
```

Interpolate into long strings with `string.format`:

```lua
local html = string.format([=[
<p>Hello %s</p>
<p>Total: $%s</p>
]=], name, total)
```

---

## Pattern matching

Lua patterns are **not** PCRE. Magic characters: `( ) . % + - * ? [ ] ^ $`. Escape with `%` (not `\`): `%.` `%(` `%%`.

| Class | Meaning | Complement |
|-------|---------|------------|
| `.` | any char | |
| `%a` | letters | `%A` |
| `%d` | digits | `%D` |
| `%s` | space | `%S` |
| `%w` | alnum | `%W` |
| `%l` `%u` | lower / upper | |
| `%x` | hex | |
| `%p` | punctuation | |

Sets: `[abc]`, `[a-z]`, `[^0-9]`. Quantifiers: `*` `+` `-` (lazy) `?`.

```lua
string.match("42px", "^%d+")                    -- 42
string.match("price $12", "%$(%d+)")            -- 12
local y, m, d = string.match("2026-07-09",
    "^(%d%d%d%d)%-(%d%d)%-(%d%d)$")

for num in string.gmatch("a1 b22 c3", "%d+") do
    print(num)
end

string.match("a(b(c)d)e", "%b()")               -- (b(c)d)  balanced
```

| Function | Returns |
|----------|---------|
| `find` | start, end, captures… |
| `match` | captures (or whole match) |
| `gmatch` | iterator |
| `gsub` | replaced string, count |

Plain (literal) search: `string.find(s, text, 1, true)`.

---

## Tables

Only structured type: array-like and map-like in one value.

```lua
local t = { "a", "b", "c", name = "Ada", age = 36 }
print(t[1], t.name)    -- a  Ada   (arrays start at 1)
```

`#t` is the array border. **Holes** (`nil` in the middle) make length unreliable — keep arrays dense.

```lua
local t = { "a", "b", "c" }
table.insert(t, "d")            -- append
table.insert(t, 2, "x")         -- insert at 2, shift up
table.remove(t)                 -- pop last
table.remove(t, 1)              -- remove index 1, shift down
table.concat(t, ",")

table.sort(t)
table.sort(t, function(a, b) return a > b end)

local rows = {
    { name = "b", n = 2 },
    { name = "a", n = 1 },
}
table.sort(rows, function(a, b) return a.name < b.name end)
```

```lua
-- 5.3+
table.move(a, 1, 2, 5)          -- copy a[1..2] to a[5..]
local p = table.pack(1, nil, 3) -- p.n == 3
table.unpack(p, 1, p.n)

-- set
local set = { apple = true }
if set.apple then end
set.pear = nil                  -- remove

-- append idiom
rows[#rows + 1] = { id = 1 }
```

Shallow copy:

```lua
local function shallow_copy(t)
    local u = {}
    for k, v in pairs(t) do u[k] = v end
    return u
end
```

---

## Numbers and precision

One `number` type; values are integer or float (IEEE-754 double).

```lua
math.type(1)        -- "integer"
math.type(1.0)      -- "float"
math.tointeger(3.0) -- 3
math.tointeger(3.1) -- nil
math.maxinteger     -- often 2^63-1
math.mininteger
```

Doubles exact integers only to about **±2^53**. Beyond that, not every integer is representable as a float.

```lua
print(1 / 2)      -- 0.5   always float division
print(1 // 2)     -- 0     floor division
print(-5 // 2)    -- -3    toward -inf

print(0.1 + 0.2 == 0.3)   -- often false

local function nearly_equal(a, b, eps)
    return math.abs(a - b) <= (eps or 1e-9)
end
```

```lua
math.floor(3.7)     -- 3
math.ceil(3.2)      -- 4
math.modf(3.7)      -- 3, 0.7
math.fmod(7.5, 2)   -- 1.5

local function round(x)
    if x >= 0 then return math.floor(x + 0.5) end
    return math.ceil(x - 0.5)
end
```

Money: store **integer cents**, format for display.

```lua
local cents = 1999
print(string.format("$%.2f", cents / 100))
```

```lua
tonumber("  42 ")
tonumber("3.14e2")
tonumber("1010", 2)
0xFF                -- 255
1.5e2               -- 150.0
```

---

## Math library

```lua
math.abs(-3)
math.max(1, 5, 3)
math.min(1, 5, 3)
math.sqrt(2)
math.sin(math.pi / 2)
math.deg(math.pi)            -- 180
math.rad(180)
math.exp(1)
math.log(100, 10)            -- 2
math.ult(a, b)               -- unsigned integer compare

math.randomseed(os.time())
math.random()                -- [0,1)
math.random(6)               -- 1..6
math.random(10, 20)          -- 10..20

math.pi
math.huge                    -- infinity
-- 0/0 is NaN; NaN ~= NaN
```

---

## Time and date formatting

```lua
local now = os.time()
os.time({ year = 2026, month = 7, day = 9, hour = 12, min = 0, sec = 0 })

local t = os.date("*t")      -- local: year month day hour min sec wday yday isdst
local u = os.date("!*t")     -- UTC table
```

### `os.date` format (strftime-like)

```lua
os.date("%Y-%m-%d")
os.date("%Y-%m-%d %H:%M:%S")
os.date("!%Y-%m-%dT%H:%M:%SZ")   -- leading ! → UTC
os.date("%A, %d %B %Y")
os.date("%H:%M:%S", some_ts)
```

| Spec | Meaning |
|------|---------|
| `%Y %m %d` | year, month, day |
| `%H %M %S` | hour, minute, second |
| `%w` | weekday 0–6 (Sun=0) |
| `%a %A` | short/long weekday name |
| `%b %B` | short/long month name |
| `%j` | day of year |
| `%c %x %X` | preferred date/time (locale) |
| `%Z %z` | timezone (platform-dependent) |
| `%%` | literal `%` |

```lua
local t1 = os.time({ year = 2026, month = 7, day = 1 })
local t2 = os.time({ year = 2026, month = 7, day = 9 })
os.difftime(t2, t1) / 86400   -- days
```

Parse with patterns + `os.time` (no full parser in stdlib):

```lua
local function parse_ymd(s)
    local y, m, d = string.match(s, "^(%d%d%d%d)%-(%d%d)%-(%d%d)$")
    if not y then return nil end
    return os.time({
        year = tonumber(y), month = tonumber(m), day = tonumber(d),
        hour = 0, min = 0, sec = 0,
    })
end
```

`os.clock()` is CPU time (seconds), not wall clock. Standard Lua has **second** wall resolution only; milliseconds need a host API.

Timezone-correct math is libc-dependent. Prefer storing epoch seconds and formatting with `os.date("!...", ts)` for UTC display.

---

## Iterators, errors, modules

```lua
for i, v in ipairs(list) do end   -- 1..n consecutive
for k, v in pairs(t) do end       -- all keys; order undefined

local function count_to(n)
    local i = 0
    return function()
        i = i + 1
        if i <= n then return i end
    end
end
for i in count_to(3) do print(i) end
```

```lua
error("boom")
error("boom", 2)                 -- blame caller

local ok, result = pcall(function() return 1 + 2 end)
local ok2, err = pcall(function() error("nope") end)

local ok3, err3 = xpcall(function()
    error("traced")
end, function(e) return debug.traceback(e, 2) end)

local n = assert(tonumber(s), "not a number")
```

```lua
-- mymodule.lua
local M = {}
function M.greet(name) return "hello " .. name end
return M

local m = require("mymodule")
-- cached in package.loaded; path is package.path
```

In sandboxes, `package.loadlib` / file IO may be disabled.

---

## Metatables (short)

```lua
local proto = {
    greet = function(self) return "hi " .. self.name end,
}
local obj = setmetatable({ name = "Ada" }, { __index = proto })
print(obj:greet())   -- hi Ada   (obj:greet() == obj.greet(obj))
```

Useful keys: `__index`, `__newindex`, `__tostring`, `__add`, `__eq`, `__call`. Most report/query scripts never need these.

---

## Recipes

```lua
local function lpad(s, width, ch)
    s, ch = tostring(s), ch or " "
    return #s >= width and s or (string.rep(ch, width - #s) .. s)
end
local function rpad(s, width, ch)
    s, ch = tostring(s), ch or " "
    return #s >= width and s or (s .. string.rep(ch, width - #s))
end
lpad(42, 5, "0")                 -- 00042

local function slice(t, i, j)
    local u = {}
    for k = i, j do u[#u + 1] = t[k] end
    return u
end

local function map(t, f)
    local u = {}
    for i, v in ipairs(t) do u[i] = f(v) end
    return u
end

local function filter(t, pred)
    local u = {}
    for _, v in ipairs(t) do
        if pred(v) then u[#u + 1] = v end
    end
    return u
end

local function count_keys(t)
    local n = 0
    for _ in pairs(t) do n = n + 1 end
    return n
end

local function unique(list)
    local seen, out = {}, {}
    for _, v in ipairs(list) do
        if not seen[v] then seen[v] = true; out[#out + 1] = v end
    end
    return out
end

local function clamp(x, lo, hi)
    return math.max(lo, math.min(hi, x))
end

local function url_encode(s)
    return (string.gsub(s, "([^%w%-_%.%~])", function(c)
        return string.format("%%%02X", string.byte(c))
    end))
end
```

---

## Pitfalls and quick indexes

| Pitfall | Reality |
|---------|---------|
| Arrays start at 0 | Start at **1** |
| `!=` / `+` for strings | Use `~=` and `..` |
| `0` is false | Only `false` and `nil` |
| `#t` with holes | Unspecified; keep dense |
| Float `==` | Use epsilon |
| Patterns vs regex | Different; `%` escapes |
| `gsub` two returns | `(string.gsub(...))` for string only |
| Bare `x = 1` | Global; prefer `local` |
| `#s` on UTF-8 | Byte length; use `utf8.len` |
| `table.concat` nested | Elements must be string/number |

### String ops

| Goal | API |
|------|-----|
| Length | `#s` |
| Left / right / mid | `string.sub` |
| Find position | `string.find` |
| Extract | `string.match` |
| Iterate matches | `string.gmatch` |
| Replace | `string.gsub` |
| Case | `lower` / `upper` |
| Format | `string.format` |
| Join | `table.concat` |
| Repeat | `string.rep` |
| Bytes | `byte` / `char` |
| UTF-8 | `utf8.*` |

### Table ops

| Goal | API |
|------|-----|
| Append | `t[#t+1]=v` / `table.insert` |
| Delete | `table.remove` |
| Sort | `table.sort` |
| Join | `table.concat` |
| Pack/unpack | `table.pack` / `table.unpack` |
| Copy range | `table.move` |
| Iterate | `ipairs` / `pairs` |

### External references

- [Lua 5.4 Reference Manual](https://www.lua.org/manual/5.4/)
- [Programming in Lua](https://www.lua.org/pil/)
- [Learn Lua in 15 minutes](https://tylerneylon.com/a/learn-lua/)
- [LUA_GUIDE.md](/docs/H/LUA_GUIDE.md) — Hydrogen scripting
- [docs/He/LUA_INTRO.md](/docs/He/LUA_INTRO.md) — Helium migrations

*Under 1000 lines. Host APIs and job lifecycle live in [LUA_GUIDE.md](/docs/H/LUA_GUIDE.md).*
