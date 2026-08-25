-- schemahelper_qdecode.lua
-- Brotli / base64 embedded-value decoding and decode-view builder.
-- Depends only on schemahelper_qutil (json/text helpers).
--
-- CHANGELOG
-- 0.5.8 - 2026-08-25 - Extracted from schemahelper_queue.lua (decode cluster)

local U = require("schemahelper_qutil")

local M = {}

local brotli_mod
do
    local ok_b, mod = pcall(require, "brotli")
    if ok_b then
        brotli_mod = mod
    end
end

local function base64_decode(data)
    local b = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
    data = tostring(data or ""):gsub("%s", ""):gsub("[^" .. b .. "=]", "")
    return (data:gsub(".", function(x)
        if x == "=" then
            return ""
        end
        local r, f = "", (b:find(x, 1, true) - 1)
        for i = 6, 1, -1 do
            r = r .. (f % 2 ^ i - f % 2 ^ (i - 1) > 0 and "1" or "0")
        end
        return r
    end):gsub("%d%d%d?%d?%d?%d?%d?%d?", function(x)
        if #x ~= 8 then
            return ""
        end
        local c = 0
        for i = 1, 8 do
            c = c + (x:sub(i, i) == "1" and 2 ^ (8 - i) or 0)
        end
        return string.char(c)
    end))
end

local function decode_brotli_b64(b64)
    local raw = base64_decode(b64)
    if raw == "" then
        return nil
    end
    if not brotli_mod then
        return nil
    end
    local ok, dec = pcall(brotli_mod.decompress, raw)
    if ok and type(dec) == "string" then
        return dec
    end
    return nil
end

function M.decode_embedded(s)
    s = tostring(s or "")
    s = s:gsub(
        "BROTLI_DECOMPRESS%s*%(%s*CRYPTO_DECODE%s*%(%s*'([^']+)'[^)]*%)%s*%)",
        function(b64)
            return decode_brotli_b64(b64) or "«brotli decode failed»"
        end)
    s = s:gsub(
        "BROTLI_DECOMPRESS%s*%(%s*FROM_BASE64%s*%(%s*'([^']+)'%s*%)%s*%)",
        function(b64)
            return decode_brotli_b64(b64) or "«brotli decode failed»"
        end)
    s = s:gsub(
        "[%w_]*%.?brotli_decompress%s*%(%s*[%w_]*%.?FROM_BASE64%s*%(%s*'([^']+)'[^)]*%)%s*%)",
        function(b64)
            return decode_brotli_b64(b64) or "«brotli decode failed»"
        end)
    s = s:gsub(
        "[%w_]*%.?BROTLI_DECOMPRESS%s*%(%s*[%w_]*%.?BASE64DECODEBINARY%s*%(%s*'([^']+)'[^)]*%)%s*%)",
        function(b64)
            return decode_brotli_b64(b64) or "«brotli decode failed»"
        end)
    s = s:gsub(
        "[%w_]*%.?brotli_decompress%s*%(%s*[%w_]*%.?DECODE%s*%(%s*'([^']+)'[^)]*%)%s*%)",
        function(b64)
            return decode_brotli_b64(b64) or "«brotli decode failed»"
        end)
    s = s:gsub(
        "[%w_]*%.?CONVERT_FROM%s*%(%s*[%w_]*%.?DECODE%s*%(%s*'([^']+)'[^)]*%)%s*,[^)]*%)",
        function(b64)
            local raw = base64_decode(b64)
            if raw == "" then
                return "«base64 decode failed»"
            end
            return raw
        end)
    s = s:gsub(
        "CRYPTO_DECODE%s*%(%s*'([^']+)'[^)]*%)",
        function(b64)
            local raw = base64_decode(b64)
            if raw == "" then
                return "«base64 decode failed»"
            end
            return raw
        end)
    s = s:gsub(
        "[%w_]*%.?BASE64DECODE%s*%(%s*'([^']+)'[^)]*%)",
        function(b64)
            local raw = base64_decode(b64)
            if raw == "" then
                return "«base64 decode failed»"
            end
            return raw
        end)
    s = s:gsub(
        "[%w_]*%.?FROM_BASE64%s*%(%s*'([^']+)'[^)]*%)",
        function(b64)
            local raw = base64_decode(b64)
            if raw == "" then
                return "«base64 decode failed»"
            end
            return raw
        end)
    return s
end

function M.has_embed(s)
    return U.has_embed(s)
end

function M.build_line_decode_view(left_line, right_line)
    local l_ok = U.has_embed(left_line)
    local r_ok = U.has_embed(right_line)
    if not l_ok and not r_ok then
        return nil
    end
    local lv = l_ok and M.decode_embedded(left_line) or ""
    local rv = r_ok and M.decode_embedded(right_line) or ""
    local layout = "both"
    if l_ok and not r_ok then
        layout = "left"
    elseif r_ok and not l_ok then
        layout = "right"
    end
    local a = U.split_lines(lv)
    local b = U.split_lines(rv)
    if layout == "left" then
        b = {}
    elseif layout == "right" then
        a = {}
    end
    local maxn = math.max(#a, #b)
    local nchg = 0
    local first_diff = 0
    local rows = {}
    for n = 1, maxn do
        local same = (a[n] or "") == (b[n] or "")
        if not same then
            nchg = nchg + 1
            if first_diff == 0 then
                first_diff = n
            end
        end
        rows[#rows + 1] = {
            kind = "pair",
            n = n,
            left = a[n] or "",
            right = b[n] or "",
            same = same,
        }
    end
    local facts = { "decoded  BROTLI / CRYPTO" }
    if layout == "left" then
        facts[#facts + 1] = "Migration only"
    elseif layout == "right" then
        facts[#facts + 1] = "Database only"
    else
        facts[#facts + 1] = "Migration vs Database"
    end
    if maxn == 0 then
        rows[#rows + 1] = {
            kind = "label",
            text = "decoded — empty",
        }
    else
        table.insert(rows, 1, {
            kind = "label",
            text = string.format("decoded — %d of %d lines differ", nchg, maxn),
        })
    end
    return {
        facts = facts,
        rows = rows,
        first_diff = first_diff,
        layout = layout,
        decoded = true,
    }
end

return M
