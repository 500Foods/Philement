-- fontkit/cff/operand.lua -- port of fontkit/src/cff/CFFOperand.js
--
-- The number encoding shared by every CFF DICT and every Type 2 charstring.
-- Integers use one, two, three, or five bytes depending on magnitude. A value
-- tagged `ptr` is an offset whose width is not known while the DICT is being
-- sized, so it is forced to a five-byte integer, which is what fontkit writes
-- for every pointer in a subset.

local utils = require("pdfkit.utils")

local M = {}

local FLOAT_LOOKUP = {
    [0] = "0", "1", "2", "3", "4", "5", "6", "7",
    "8", "9", ".", "E", "E-", nil, "-",
}
local FLOAT_EOF = 0xF

local FLOAT_ENCODE = {
    ["."] = 10,
    E = 11,
    ["-"] = 14,
}

--- ToInt32, so the float-versus-integer test matches `(value | 0) !== value`.
---
--- Truncate toward zero before the 2^32 wrap. A bare modulo leaves 0.0625 as
--- 0.0625, and that value then takes the one-byte integer encoding.
local function to_int32(val)
    if val ~= val or val == math.huge or val == -math.huge then
        return 0
    end
    if val >= 0 then
        val = math.floor(val)
    else
        val = math.ceil(val)
    end
    local n = val % 4294967296
    if n < 0 then
        n = n + 4294967296
    end
    if n >= 2147483648 then
        n = n - 4294967296
    end
    return n
end

local function is_js_int(val)
    return type(val) == "number" and to_int32(val) == val
end

function M.decode(r, value)
    if value >= 32 and value <= 246 then
        return value - 139
    end
    if value >= 247 and value <= 250 then
        return (value - 247) * 256 + r:uint8() + 108
    end
    if value >= 251 and value <= 254 then
        return -(value - 251) * 256 - r:uint8() - 108
    end
    if value == 28 then
        return r:int16()
    end
    if value == 29 then
        return r:int32()
    end
    if value == 30 then
        local str = {}
        while true do
            local b = r:uint8()
            local n1 = b >> 4
            if n1 == FLOAT_EOF then break end
            str[#str + 1] = FLOAT_LOOKUP[n1]
            local n2 = b & 15
            if n2 == FLOAT_EOF then break end
            str[#str + 1] = FLOAT_LOOKUP[n2]
        end
        return tonumber(table.concat(str))
    end
    return nil
end

function M.size(value)
    if type(value) == "table" and value.ptr then
        return 5
    end
    local val = value
    if not is_js_int(val) then
        local str = utils.js_tostring(val)
        return 1 + math.ceil((#str + 1) / 2)
    elseif val >= -107 and val <= 107 then
        return 1
    elseif (val >= 108 and val <= 1131) or (val >= -1131 and val <= -108) then
        return 2
    elseif val >= -32768 and val <= 32767 then
        return 3
    else
        return 5
    end
end

function M.encode(w, value)
    if type(value) == "table" and value.ptr then
        w:uint8(29)
        w:int32(value.val)
        return
    end

    local val = value
    if not is_js_int(val) then
        w:uint8(30)
        local str = utils.js_tostring(val)
        local n2
        local i = 1
        while i <= #str do
            local c1 = str:sub(i, i)
            local n1 = FLOAT_ENCODE[c1] or tonumber(c1) or 0
            if i == #str then
                n2 = FLOAT_EOF
            else
                local c2 = str:sub(i + 1, i + 1)
                n2 = FLOAT_ENCODE[c2] or tonumber(c2) or 0
            end
            w:uint8((n1 << 4) | (n2 & 15))
            i = i + 2
        end
        if n2 ~= FLOAT_EOF then
            w:uint8(FLOAT_EOF << 4)
        end
        return
    end

    if val >= -107 and val <= 107 then
        w:uint8(val + 139)
    elseif val >= 108 and val <= 1131 then
        val = val - 108
        w:uint8((val >> 8) + 247)
        w:uint8(val & 0xFF)
    elseif val >= -1131 and val <= -108 then
        val = -val - 108
        w:uint8((val >> 8) + 251)
        w:uint8(val & 0xFF)
    elseif val >= -32768 and val <= 32767 then
        w:uint8(28)
        w:int16(val)
    else
        w:uint8(29)
        w:int32(val)
    end
end

return M
