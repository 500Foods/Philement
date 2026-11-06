-- object.lua -- port of pdfkit/lib/object.js
--
-- Converts Lua values into PDF syntax. The JS original leans on JavaScript type
-- distinctions that Lua simply does not have, so the interesting part of this
-- port is reconstructing those distinctions explicitly.
--
--   JavaScript                     PDF output        this port
--   ---------------------------    --------------    ---------------------
--   'Foo'          (primitive)     /Foo              plain Lua string
--   new String('Foo')  (object)    (Foo)             M.str("Foo")
--   Buffer                         <hex>             M.hex(bytes)
--   Date                           (D:...Z)          M.date(epoch)
--   Array.isArray(x)               [a b c]           M.array{...}
--   plain object                   << /K v >>        M.dict()
--   number                         123.456           Lua number
--
-- Two of these deserve more than a table row.
--
-- STRING vs NAME. In JS a bare string literal becomes a PDF *name* and a
-- `new String(...)` wrapper becomes a PDF *string*. That distinction is load
-- bearing -- `/Helvetica` and `(Helvetica)` are different objects -- and Lua
-- has exactly one string type, so the wrapper has to be explicit here.
--
-- KEY ORDER. JavaScript objects iterate their string keys in insertion order,
-- so `for key in object` walks the dictionary in the order the source wrote it
-- and the output has a stable, reproducible key sequence. Lua's `pairs` makes
-- no such promise -- it will happily emit /MediaBox before /Type, and differently
-- on the next run. Every dictionary in this port is therefore an ordered
-- structure that remembers insertion order, built by M.dict(). Assigning to a
-- new key appends it; assigning to an existing key updates in place without
-- moving it; assigning nil removes it. That matches JS object semantics closely
-- enough for our purposes.

local utils = require("pdfkit.utils")

local M = {}

--------------------------------------------------------------------------------
-- Type tags
--------------------------------------------------------------------------------

local DICT = {}   -- metatable for insertion-ordered dictionaries
local ARRAY = {}  -- metatable marking a plain sequence as a PDF array
local STR = {}    -- metatable for PDF literal strings
local HEX = {}    -- metatable for PDF hex strings
local DATE = {}   -- metatable for PDF dates

M.DICT, M.ARRAY, M.STR, M.HEX, M.DATE = DICT, ARRAY, STR, HEX, DATE

--------------------------------------------------------------------------------
-- Ordered dictionary
--------------------------------------------------------------------------------

DICT.__index = function(t, k)
    return rawget(t, "_values")[k]
end

DICT.__newindex = function(t, k, v)
    local values = rawget(t, "_values")
    local order = rawget(t, "_order")

    if v == nil then
        if values[k] ~= nil then
            values[k] = nil
            for i = 1, #order do
                if order[i] == k then
                    table.remove(order, i)
                    break
                end
            end
        end
        return
    end

    if values[k] == nil then
        order[#order + 1] = k
    end
    values[k] = v
end

-- Iterate the dictionary in insertion order. Used by convert() and by anything
-- that needs to walk a dictionary deterministically.
function M.dict_pairs(d)
    local order = rawget(d, "_order")
    local values = rawget(d, "_values")
    local i = 0
    return function()
        i = i + 1
        local k = order[i]
        if k == nil then return nil end
        return k, values[k]
    end
end

-- The number of keys currently in the dictionary. The JS equivalent is
-- Object.keys(o).length, which spot-colour naming relies on.
function M.dict_count(d)
    return #rawget(d, "_order")
end

-- M.dict()                       -> empty dictionary, fill it by assignment
-- M.dict{ {"Type","Page"}, ... } -> dictionary with keys in the listed order
--
-- The pair-list form exists because a Lua table constructor cannot express key
-- order; `{Type = "Page", Parent = ref}` would be reordered by the runtime.
function M.dict(pairs_list)
    local d = setmetatable({}, DICT)
    rawset(d, "_values", {})
    rawset(d, "_order", {})
    if pairs_list then
        for _, kv in ipairs(pairs_list) do
            d[kv[1]] = kv[2]
        end
    end
    return d
end

function M.is_dict(v)
    return type(v) == "table" and getmetatable(v) == DICT
end

--------------------------------------------------------------------------------
-- Arrays, strings, hex strings, dates
--------------------------------------------------------------------------------

-- An explicit null. Lua cannot hold nil in a table without punching a hole in
-- it -- `#` stops at the hole and ipairs stops with it -- so an array slot that
-- is deliberately empty needs a value that stands for "nothing here". The
-- structure tree's parent tree reserves slots this way before anything claims
-- them. Renders as `null`, which is what the JS null in the same slot produces.
M.NULL = setmetatable({}, {
    __tostring = function() return "null" end,
    __name = "pdf.null",
})

-- Arrays carry a marker metatable only -- no __index or __newindex -- so that
-- `#arr`, `arr[i] = v` and table.insert all behave like a normal sequence.
function M.array(t)
    return setmetatable(t or {}, ARRAY)
end

function M.is_array(v)
    return type(v) == "table" and getmetatable(v) == ARRAY
end

--- JavaScript's `Array.isArray`, for values that came from the caller.
---
--- `is_array` above asks a question about *our* representation -- is this thing
--- going to be written out as a PDF array -- and the answer has to be exact, or
--- convert() would guess wrong about a dictionary. This asks the looser question
--- the API boundary needs: did the caller mean a list? A caller writing
--- `{ bbox = { 72, 240, 172, 340 } }` wrote a JS array and should be believed,
--- so a bare table counts alongside a tagged one. An ordered dictionary or a
--- wrapped string carries a metatable of its own and does not.
function M.is_arrayish(v)
    if type(v) ~= "table" then return false end
    if getmetatable(v) == ARRAY then return true end
    return getmetatable(v) == nil
end

-- A PDF literal string: (like this). The JS equivalent is `new String(s)`.
function M.str(s)
    return setmetatable({ value = s }, STR)
end

-- A PDF hex string: <like this>. The JS equivalent is a Buffer.
function M.hex(bytes)
    return setmetatable({ value = bytes }, HEX)
end

-- A PDF date. `epoch` is an os.time() value; nil means now. Always rendered in
-- UTC with a trailing Z, matching PDFObject.convert's Date branch.
--
-- `ms` is the same instant in milliseconds since the epoch -- what JavaScript's
-- Date.getTime() returns. It is carried separately because the file ID hashes
-- the millisecond value while the visible date is only accurate to the second,
-- and because os.time() cannot express sub-second precision. Callers that need
-- a reproducible ID pass it explicitly; otherwise it is the whole second.
function M.date(epoch, ms)
    epoch = epoch or os.time()
    return setmetatable({
        value = epoch,
        ms = ms or (math.tointeger(epoch) or math.floor(epoch)) * 1000,
    }, DATE)
end

--------------------------------------------------------------------------------
-- Name escaping
--------------------------------------------------------------------------------
--
-- Port of escapeName(). Delimiters, whitespace and control characters become
-- #XX escapes; bytes above 0x7f are deliberately left alone, which the JS
-- comments flag as backward compatibility with existing output.

local function is_safe_char_code(code)
    if code > 0x7f then return true end
    return code > 0x20
        and code ~= 0x7f
        and code ~= 0x23 -- #
        and code ~= 0x25 -- %
        and code ~= 0x28 -- (
        and code ~= 0x29 -- )
        and code ~= 0x2f -- /
        and code ~= 0x3c -- <
        and code ~= 0x3e -- >
        and code ~= 0x5b -- [
        and code ~= 0x5d -- ]
        and code ~= 0x7b -- {
        and code ~= 0x7d -- }
end

function M.escape_name(name)
    local out = {}
    for i = 1, #name do
        local code = name:byte(i)
        if is_safe_char_code(code) then
            out[#out + 1] = name:sub(i, i)
        else
            out[#out + 1] = string.format("#%02X", code)
        end
    end
    return table.concat(out)
end

--------------------------------------------------------------------------------
-- String escaping inside PDF literal strings
--------------------------------------------------------------------------------

local escapable = {
    ["\n"] = "\\n",
    ["\r"] = "\\r",
    ["\t"] = "\\t",
    ["\b"] = "\\b",
    ["\f"] = "\\f",
    ["\\"] = "\\\\",
    ["("] = "\\(",
    [")"] = "\\)",
}

local function escape_string(s)
    return (s:gsub("[\n\r\t\b\f()\\]", escapable))
end

--------------------------------------------------------------------------------
-- Numbers
--------------------------------------------------------------------------------

M.number = utils.number

--------------------------------------------------------------------------------
-- convert
--------------------------------------------------------------------------------

local function to_hex(bytes)
    return (bytes:gsub(".", function(c)
        return string.format("%02x", c:byte())
    end))
end

-- Big-endian UTF-16 with a BOM, for strings containing non-ASCII bytes. This
-- mirrors the isUnicode branch of PDFObject.convert.
local function utf16be_with_bom(s)
    local out = { "\254\255" } -- U+FEFF
    for _, code in utf8.codes(s) do
        if code < 0x10000 then
            out[#out + 1] = string.pack(">I2", code)
        else
            local v = code - 0x10000
            out[#out + 1] = string.pack(">I2", 0xD800 + (v >> 10))
            out[#out + 1] = string.pack(">I2", 0xDC00 + (v & 0x3FF))
        end
    end
    return table.concat(out)
end

function M.convert(object, encrypt_fn)
    local t = type(object)

    -- Plain Lua strings are PDF names.
    if t == "string" then
        return "/" .. object
    end

    if t == "number" then
        return M.number(object)
    end

    if t == "boolean" then
        return object and "true" or "false"
    end

    if t == "nil" then
        return "null"
    end

    if t == "table" then
        if object == M.NULL then
            return "null"
        end

        local mt = getmetatable(object)

        if mt == STR then
            local s = object.value
            -- Detect non-ASCII and switch to UTF-16BE, as the JS does.
            local is_unicode = false
            for i = 1, #s do
                if s:byte(i) > 0x7f then
                    is_unicode = true
                    break
                end
            end

            local buf = is_unicode and utf16be_with_bom(s) or s
            if encrypt_fn then
                buf = encrypt_fn(buf)
            end
            return "(" .. escape_string(buf) .. ")"
        end

        if mt == HEX then
            return "<" .. to_hex(object.value) .. ">"
        end

        if mt == DATE then
            local d = os.date("!*t", object.value)
            local s = string.format(
                "D:%04d%02d%02d%02d%02d%02dZ",
                d.year, d.month, d.day, d.hour, d.min, d.sec
            )
            if encrypt_fn then
                s = escape_string(encrypt_fn(s))
            end
            return "(" .. s .. ")"
        end

        if mt == ARRAY then
            local items = {}
            for i = 1, #object do
                items[i] = M.convert(object[i], encrypt_fn)
            end
            return "[" .. table.concat(items, " ") .. "]"
        end

        -- Anything carrying a to_pdf_string method serialises itself. This is
        -- the PDFAbstractReference / PDFTree branch of the JS: references
        -- render as "N 0 R", name trees render their whole subtree.
        if object.to_pdf_string then
            return object:to_pdf_string()
        end

        if mt == DICT then
            local out = { "<<" }
            for k, v in M.dict_pairs(object) do
                out[#out + 1] = "/" .. k .. " " .. M.convert(v, encrypt_fn)
            end
            out[#out + 1] = ">>"
            return table.concat(out, "\n")
        end

        error("object.convert: unrecognised table; use M.dict/M.array/M.str/M.hex/M.date")
    end

    error("object.convert: cannot convert a " .. t)
end

return M
