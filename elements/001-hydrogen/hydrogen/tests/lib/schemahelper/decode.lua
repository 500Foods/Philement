local env = require('schemahelper_test_env')
package.cpath = package.cpath .. ';/usr/local/lib/lua/5.5/?.so;/home/asimard/.luarocks/lib/lua/5.5/?.so'
local queue = require('schemahelper_queue')

local test_plain = 'hello world'
local b64_blob = 'DwWAaGVsbG8gd29ybGQD'
local plain_b64 = 'dGVzdA=='
local errs = {}

local r = queue.decode_embedded("brotli_decompress(FROM_BASE64('" .. b64_blob .. "'))")
if r ~= test_plain then errs[#errs + 1] = 'MySQL lowercase brotli: got ' .. tostring(r) end

r = queue.decode_embedded("BROTLI_DECOMPRESS(FROM_BASE64('" .. b64_blob .. "'))")
if r ~= test_plain then errs[#errs + 1] = 'MySQL uppercase brotli failed' end

r = queue.decode_embedded("BROTLI_DECOMPRESS(CRYPTO_DECODE('" .. b64_blob .. "'))")
if r ~= test_plain then errs[#errs + 1] = 'SQLite brotli failed' end

r = queue.decode_embedded("myschema.BROTLI_DECOMPRESS(myschema.BASE64DECODEBINARY('" .. b64_blob .. "'))")
if r ~= test_plain then errs[#errs + 1] = 'DB2 brotli failed' end

r = queue.decode_embedded("brotli_decompress(DECODE('" .. b64_blob .. "', 'base64'))")
if r ~= test_plain then errs[#errs + 1] = 'PostgreSQL brotli failed' end

r = queue.decode_embedded("CONVERT_FROM(DECODE('" .. plain_b64 .. "', 'base64'), 'UTF8')")
if r ~= 'test' then errs[#errs + 1] = 'PostgreSQL CONVERT_FROM failed' end

r = queue.decode_embedded("CRYPTO_DECODE('" .. plain_b64 .. "')")
if r ~= 'test' then errs[#errs + 1] = 'SQLite CRYPTO_DECODE failed' end
r = queue.decode_embedded("BASE64DECODE('" .. plain_b64 .. "')")
if r ~= 'test' then errs[#errs + 1] = 'DB2 BASE64DECODE failed' end
r = queue.decode_embedded("FROM_BASE64('" .. plain_b64 .. "')")
if r ~= 'test' then errs[#errs + 1] = 'MySQL FROM_BASE64 failed' end

if not queue.has_embed("brotli_decompress(FROM_BASE64('" .. b64_blob .. "'))") then
    errs[#errs + 1] = 'has_embed MySQL lowercase failed'
end
if not queue.has_embed("BROTLI_DECOMPRESS(BASE64DECODEBINARY('" .. b64_blob .. "'))") then
    errs[#errs + 1] = 'has_embed DB2 failed'
end

if queue.has_embed('SELECT 1 FROM users') then
    errs[#errs + 1] = 'has_embed false positive on plain SQL'
end

env.fail_if(errs)
print('OK: all decode patterns verified (SQLite, MySQL upper+lower, DB2, PostgreSQL)')
os.exit(0)
