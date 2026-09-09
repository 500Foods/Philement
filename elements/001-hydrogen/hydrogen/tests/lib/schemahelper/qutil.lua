local env = require('schemahelper_test_env')
local qutil = require('schemahelper_qutil')
local fixture = env.fixture
local errs = {}

local required = {
    'json_escape', 'file_exists', 'write_all', 'json_string_field',
    'json_num_field', 'json_subobj', 'listed_fields', 'payload_text',
    'payload_raw', 'payload_field_differs', 'has_embed', 'split_lines',
    'first_diff_at', 'pad_clip', 'wrap_hard', 'clip_text', 'jq_lines',
}
for _, e in ipairs(env.require_fns(qutil, 'qutil', required)) do
    errs[#errs + 1] = e
end

if qutil.has_embed('SELECT 1 FROM users') then
    errs[#errs + 1] = 'has_embed false positive on plain SQL'
end
if not qutil.has_embed("brotli_decompress(FROM_BASE64('abc'))") then
    errs[#errs + 1] = 'has_embed failed to detect brotli'
end

local esc = qutil.json_escape('hello "world"')
if not esc:find('\\"', 1, true) then
    errs[#errs + 1] = 'json_escape did not escape quotes: ' .. esc
end

if qutil.json_escape('a\\b') ~= 'a\\\\b' then
    errs[#errs + 1] = 'json_escape did not double-escape backslash'
end

local state_json = '{"design":"acuranzo","engine":"sqlite","cursor_id":"meta:drift:1148:1003:name"}'
local design = qutil.json_string_field(state_json, 'design')
if design ~= 'acuranzo' then
    errs[#errs + 1] = 'json_string_field design=' .. tostring(design)
end

local ver = qutil.json_num_field('{"version":1}', 'version')
if ver ~= 1 then
    errs[#errs + 1] = 'json_num_field version=' .. tostring(ver)
end

if not qutil.file_exists(fixture .. '/findings.json') then
    errs[#errs + 1] = 'file_exists should be true for findings.json'
end
if qutil.file_exists(fixture .. '/nonexistent.json') then
    errs[#errs + 1] = 'file_exists false positive'
end

local lines = qutil.split_lines('a\nb\nc')
if #lines ~= 3 or lines[1] ~= 'a' or lines[3] ~= 'c' then
    errs[#errs + 1] = 'split_lines failed'
end

if qutil.first_diff_at('ab', 'ac') ~= 2 then
    errs[#errs + 1] = 'first_diff_at expected 2'
end
if qutil.first_diff_at('abc', 'abc') ~= nil then
    errs[#errs + 1] = 'first_diff_at expected nil for equal'
end
if qutil.first_diff_at('abc', 'ab') ~= 3 then
    errs[#errs + 1] = 'first_diff_at expected 3 for length diff'
end

local raw = qutil.payload_raw('{"name":"x"}', 'name')
if raw ~= 'x' then
    errs[#errs + 1] = 'payload_raw name=' .. tostring(raw)
end

local fields = qutil.listed_fields('{"fields":["code","name"]}')
if #fields ~= 2 or fields[1] ~= 'code' then
    errs[#errs + 1] = 'listed_fields failed'
end

env.fail_if(errs)
print('OK: all qutil function checks passed (' .. #required .. ' functions)')
os.exit(0)
